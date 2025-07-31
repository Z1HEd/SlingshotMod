#include <4dm.h>
#include "4DKeyBinds.h"
#include <glm/gtc/random.hpp>
#include "auilib/auilib.h"
#include "utils.h"
#include "ItemSlingshot.h"
#include "JSONData.h"
#include "FullPlayerInventory.h"
#include "JSONPacket.h"
#include "4DVR.h"

#include "EntityProjectile.h"

using namespace fdm;

inline constexpr float maxZoom = 0.2;

aui::BarIndicator drawIndicator;
gui::Text shotBulletNumText{};
QuadRenderer qr{};
TexRenderer bulletBackgroundRenderer;
TexRenderer bulletRenderer;
gui::Text bulletCountText{};
gui::Interface ui;
FontRenderer font{};

struct
{
	VR::ActionHandle nextBullet = 0;
	VR::ActionHandle prevBullet = 0;
	VR::ActionHandle drawSlingshot = 0;
	VR::ActionHandle addBullet = 0;
} vrActions;

// Initialize the DLLMain
initDLL

void changeBulletType(Player* player, int change = 1);

int getItemCount(Player* player, const stl::string& name)
{
	int count = 0;
	for (int slot = 0;slot < FullPlayerInventory::getSlotCount(player); slot++) {
		Item* i = FullPlayerInventory::getSlot(player,slot).get();
		if (i != nullptr && i->getName() == name)
			count += i->count;
	}
	return count;
}

void subtractItem(Player* player, const stl::string& name)
{
	for (int slot = 0;slot < FullPlayerInventory::getSlotCount(player); slot++) {
		Item* i = FullPlayerInventory::getSlot(player, slot).get();
		if (i != nullptr && i->getName() == name) {
			i->count--;
			if (i->count < 1) FullPlayerInventory::getSlot(player, slot).reset();
			return;
		}
	}
}

void localPlayerEventJson(World* world, const stl::string& event, const nlohmann::json& data, Player* player);

const float drawFractionThreshold = 0.1f;
void handleShoot(World* world, const nlohmann::json& data)
{
	const glm::vec4 playerPos = m4::vec4FromJson<float>(data["playerPosition"]);
	const glm::vec4 playerDirection = glm::normalize(m4::vec4FromJson<float>(data["playerDirection"]));
	const glm::vec4 playerVelocity = m4::vec4FromJson<float>(data["playerVelocity"]);
	const stl::uuid ownerPlayerId = stl::uuid()(data["ownerPlayerId"].get<std::string>());
	const float drawFraction = data["drawFraction"].get<float>();
	const float numBullets = glm::clamp(data["numBullets"].get<int>(), 1, ItemSlingshot::maxBulletsPerShot);

	if (drawFraction < drawFractionThreshold) return;

	constexpr float randomRange = 0.012f;
	constexpr float randomRangeDeadly = 0.006f;

	constexpr float woodenSpeed = 100.0f;
	constexpr float deadlySpeed = 150.0f;

	const float nNormalized = (numBullets - 1.0f) / (ItemSlingshot::maxBulletsPerShot - 1.0f); // [0.0;1.0]
	const float nDiv = nNormalized * (2.25f - 1.0f) + 1.0f; // [1.0;2.25]
	const float nMult = nNormalized * (4.0f - 1.0f) + 1.0f; // [1.0;4.0]

	EntityPlayer* ownerPlayerEntity = (EntityPlayer*)world->getEntity(ownerPlayerId);
	if (!ownerPlayerEntity) return;

	Player* player = ownerPlayerEntity->player;

	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
	if (!slingshot) return;

	EntityProjectile::BulletType selectedBulletType = slingshot->selectedBulletType;

	// if holding a bullet in the other hand, use that bullet type instead of the selected one

	auto& item = slingshot->offhand ? 
		player->getSelectedHotbarSlot() : player->equipment.getSlot(0);
	if (item)
	{
		auto name = item->getName();
		if (EntityProjectile::bulletTypes.contains(name))
		{
			selectedBulletType = EntityProjectile::bulletTypes.at(name);
		}
	}

	if (slingshot->isDrawing && getItemCount(player, EntityProjectile::bulletTypeNames.at(selectedBulletType)) == 0)
	{
		localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, player);
		return;
	}

	float clampedDrawFraction = glm::clamp(drawFraction, 0.175f, 1.0f);

	// rescale player vel (to clamp it to a max speed, in this case 32)
	float pVelL = glm::length(playerVelocity);
	glm::vec4 scaledPlayerVel = playerVelocity;
	if (pVelL >= 0.01f)
	{
		scaledPlayerVel *= 1.0f / pVelL;
		scaledPlayerVel *= glm::min(pVelL, 32.0f);
	}

	float damage = 0.0f;
	switch (selectedBulletType)
	{
	case EntityProjectile::BULLET_STONE:
		damage = 10.0f;
		break;
	case EntityProjectile::BULLET_IRON:
		damage = 15.0f;
		break;
	case EntityProjectile::BULLET_DEADLY:
		damage = 20.0f;
		break;
	case EntityProjectile::BULLET_SOLENOID:
		damage = 15.0f;
		break;
	}
	damage *= clampedDrawFraction;
	damage /= nDiv;

	float speed = slingshot->isDeadly() ? deadlySpeed : woodenSpeed;
	speed *= clampedDrawFraction;
	speed /= nDiv;

	for (int i = 0; i < numBullets; ++i)
	{
		if (getItemCount(player, EntityProjectile::bulletTypeNames.at(selectedBulletType)) == 0)
		{
			break;
		}

		float rndRange = slingshot->isDeadly() ? randomRangeDeadly : randomRange;
		rndRange *= nMult;

		glm::vec4 velocity = glm::normalize(playerDirection + glm::vec4{
			glm::linearRand(-rndRange, rndRange),
			glm::linearRand(-rndRange, rndRange),
			glm::linearRand(-rndRange, rndRange),
			glm::linearRand(-rndRange, rndRange) }) * speed;

		velocity += scaledPlayerVel;

		nlohmann::json attributes;
		attributes["velocity"] = m4::vec4ToJson(velocity);

		attributes["damage"] = damage;
		attributes["type"] = (int)selectedBulletType;

		attributes["ownerPlayerId"] = stl::uuid::to_string(ownerPlayerId);

		auto projectile = Entity::createWithAttributes("Projectile", playerPos, attributes);

		Chunk* chunk = world->getChunkFromCoords(playerPos.x, playerPos.z, playerPos.w);
		if (chunk)
			world->addEntityToChunk(projectile, chunk);

		subtractItem(player, EntityProjectile::bulletTypeNames.at(selectedBulletType));
	}

	// Sync Inventory
	if (world->getType() == World::TYPE_SERVER)
	{
		auto* server = (WorldServer*)world;
		nlohmann::json invJson = player->saveInventory();
		if (server->entityPlayerIDs.contains(player->EntityPlayerID))
			server->sendMessagePlayer({ Packet::S_INVENTORY_UPDATE, invJson.dump() }, server->entityPlayerIDs.at(player->EntityPlayerID), true);
	}

	slingshot->isDrawing = false;
	slingshot->drawFraction = 0.0f;
	slingshot->numBullets = 1;
}

// Handle player inputs
$hook(void, Player, update, World* world, double dt, EntityPlayer* entityPlayer)
{
	original(self, world, dt, entityPlayer);
	
	if (world->getType() == World::TYPE_SERVER) return; // run on client and singleplayer
	if (self != &StateGame::instanceObj.player) return; // only on the local player
	if (self->inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(self);
	if (!slingshot) return;

	glm::vec4 vel = self->vel;
	if (VR::inVR())
	{
		vel += VR::getControllerVel4D(!slingshot->offhand ? VR::CONTROLLER_RIGHT : VR::CONTROLLER_LEFT);
	}

	{
		glm::vec4 stringVelAdd = vel * 20.0f;
		float velL = glm::length(stringVelAdd);
		if (velL >= 0.01f)
		{
			stringVelAdd *= 1.0f / velL;
			stringVelAdd *= glm::min(velL, 20.0f);
		}

		slingshot->stringVel -= stringVelAdd * (float)dt;
	}

	slingshot->stringVel += (slingshot->stringTargetPos - slingshot->stringPos) * ItemSlingshot::stringStiffness * (float)dt;
	slingshot->stringVel *= (float)(1.0 - ItemSlingshot::stringDamping * dt);
	slingshot->stringPos += slingshot->stringVel * (float)dt;

	if (getItemCount(self, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType)) == 0)
	{
		slingshot->isDrawing = false;
		slingshot->drawFraction = 0.0f;
		slingshot->numBullets = 1;
		return;
	}

	if (!VR::inVR())
	{
		slingshot->stringTargetPos = glm::vec4{ 0 };
		if (self->keys.rightMouseDown)
		{
			if (!slingshot->isDrawing)
			{
				EntityProjectile::playStretchSound(self->cameraPos);

				slingshot->numBullets = 1;
				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_START_DRAWING, {}, self);

				shotBulletNumText.setText("1");
			}

			slingshot->isDrawing = true;
			slingshot->drawFraction = std::min(1.0, slingshot->drawFraction + dt / ((slingshot->isDeadly() ? 0.5 : 0.9) + ((slingshot->numBullets - 1.0f) / (ItemSlingshot::maxBulletsPerShot - 1.0f) * 0.4f)));

			slingshot->stringTargetPos = -self->forward * slingshot->drawFraction * 0.6f;
		}
		else if (slingshot->isDrawing)
		{
			if (slingshot->drawFraction >= drawFractionThreshold)
			{
				nlohmann::json data = {
					{ "drawFraction", slingshot->drawFraction + 0.01 },
					{ "ownerPlayerId", stl::uuid::to_string(self->EntityPlayerID) },
					{ "playerPosition", m4::vec4ToJson(self->cameraPos) },
					{ "playerDirection", m4::vec4ToJson(self->forward) },
					{ "playerVelocity", m4::vec4ToJson(vel) },
					{ "numBullets", slingshot->numBullets },
				};
				EntityProjectile::playSlingshotSound(self->cameraPos);
				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_SHOOT, data, self);

				slingshot->isDrawing = false;
				slingshot->drawFraction = 0.0f;
				slingshot->numBullets = 1;
			}
			else
			{
				slingshot->isDrawing = false;
				slingshot->drawFraction = 0.0f;
				slingshot->numBullets = 1;

				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, self);
			}
		}
	}
	else // vr interactive drawing or smth ig
	{
		bool drawingBtn = false;
		static bool lastDrawingBtn = false;
		VR::getInput(vrActions.drawSlingshot, drawingBtn);
		bool addBulletBtn = false;
		static bool lastAddBulletBtn = false;
		VR::getInput(vrActions.addBullet, addBulletBtn);
		bool prevBtn = false;
		static bool lastPrevBtn = false;
		VR::getInput(vrActions.prevBullet, prevBtn);
		bool nextBtn = false;
		static bool lastNextBtn = false;
		VR::getInput(vrActions.nextBullet, nextBtn);

		if (addBulletBtn && !lastAddBulletBtn)
		{
			++slingshot->numBullets;
			if (slingshot->numBullets > ItemSlingshot::maxBulletsPerShot)
				slingshot->numBullets = 1;
		}
		if (prevBtn && !lastPrevBtn)
		{
			changeBulletType(self, -1);
		}
		if (nextBtn && !lastNextBtn)
		{
			changeBulletType(self, 1);
		}
		lastAddBulletBtn = addBulletBtn;
		lastPrevBtn = prevBtn;
		lastNextBtn = nextBtn;

		glm::vec4 hmdPos = VR::getHeadPos4D();

		m4::Mat5 lMat = VR::getController4D(VR::CONTROLLER_LEFT);
		lMat.translate(glm::vec4{ 0, 0, 0.04f, 0 });
		glm::vec4 lPos = *(glm::vec4*)lMat[4] - hmdPos;
		
		m4::Mat5 rMat = VR::getController4D(VR::CONTROLLER_RIGHT);
		std::swap(*(glm::vec4*)rMat[1], *(glm::vec4*)rMat[2]);
		rMat.scale(glm::vec4{ 0.65f });
		rMat *= m4::Rotor{ m4::BiVector4{ 0, 0, 0, 1, 0, 0 }, glm::radians(-20.0f) };
		rMat.translate(glm::vec4{ 0, -0.275f, 0.25f, 0 });
		rMat.scale(glm::vec4{ 0.4f });
		rMat.translate(glm::vec4{ 0, 0.2f, -0.6f, 0 });
		glm::vec4 rPos = *(glm::vec4*)rMat[4] - hmdPos;
		slingshot->stringTargetPos = glm::vec4{ 0 };

		//Debug::box(glm::vec4{ 0.05f }, glm::vec4{ 1, 0, 0, 1 }, self->pos + lPos, true);

		constexpr float boxSize = 0.15f;
		bool in =
			lPos.x >= rPos.x - boxSize && lPos.x <= rPos.x + boxSize &&
			lPos.y >= rPos.y - boxSize && lPos.y <= rPos.y + boxSize &&
			lPos.z >= rPos.z - boxSize && lPos.z <= rPos.z + boxSize &&
			lPos.w >= rPos.w - boxSize && lPos.w <= rPos.w + boxSize;

		//Debug::box(glm::vec4{ boxSize }, glm::vec4{0, in, !in, 1}, self->pos + rPos, true);

		if ((!drawingBtn || slingshot->offhand) && slingshot->isDrawing)
		{
			if (slingshot->drawFraction >= drawFractionThreshold)
			{
				glm::vec4 pos = self->pos + rPos;
				glm::vec4 dir = glm::normalize(rPos - lPos);

				nlohmann::json data
				{
					{ "drawFraction", slingshot->drawFraction },
					{ "ownerPlayerId", stl::uuid::to_string(self->EntityPlayerID) },
					{ "playerPosition", m4::vec4ToJson(pos) },
					{ "playerDirection", m4::vec4ToJson(dir) },
					{ "playerVelocity", m4::vec4ToJson(vel) },
					{ "numBullets", slingshot->numBullets },
				};
				EntityProjectile::playSlingshotSound(pos);
				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_SHOOT, data, self);

				slingshot->isDrawing = false;
				slingshot->drawFraction = 0.0f;
				slingshot->numBullets = 1;
			}
			else
			{
				slingshot->isDrawing = false;
				slingshot->drawFraction = 0.0f;
				slingshot->numBullets = 1;

				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, self);
			}
		}

		if (slingshot->offhand)
		{
			lastDrawingBtn = drawingBtn;
			return;
		}

		if (drawingBtn)
		{
			glm::vec4 d = rPos - lPos;
			//std::swap(d.z, d.w); // xyw test
			float dist = glm::length(d);

			if (dist > 0.1f && abs(glm::dot(glm::normalize(d), glm::normalize(*(glm::vec4*)rMat[2]))) < 0.3f)
			{
				if (slingshot->isDrawing)
				{
					slingshot->isDrawing = false;
					slingshot->drawFraction = 0.0f;
					slingshot->numBullets = 1;

					localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, self);
				}
				lastDrawingBtn = drawingBtn;
				return;
			}

			if (!slingshot->isDrawing)
			{
				if (lastDrawingBtn || !in)
				{
					lastDrawingBtn = drawingBtn;
					return;
				}
				EntityProjectile::playStretchSound(rPos);

				slingshot->numBullets = 1;
				localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_START_DRAWING, {}, self);
			}

			slingshot->isDrawing = true;
			slingshot->drawFraction = std::min(1.0f, dist / (0.6f + slingshot->numBullets * 0.05f));
			slingshot->stringTargetPos = -d;
		}
		lastDrawingBtn = drawingBtn;
	}
}

// Render UI
$hook(void, Player, renderHud, GLFWwindow* window)
{
	original(self, window);
	
	if (VR::inVR())
	{
		return;
	}

	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(self);
	if (!slingshot) return;
	if (self->inventoryManager.isOpen()) return;

	glDisable(GL_DEPTH_TEST);
		
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	drawIndicator.offsetX(width / 2 - 30);
	drawIndicator.offsetY(height / 2 - 50);
	drawIndicator.setFill(slingshot->drawFraction);
	drawIndicator.visible = slingshot->drawFraction > 0;

	shotBulletNumText.offsetX(drawIndicator.xOffset - 12);
	shotBulletNumText.offsetY(height / 2 - 10 / 2);

	int posX, posY;
	if (!slingshot->offhand) {
		posX = self->hotbar.renderPos.x + 68 /*imperfect reality*/ + 34 * ((self->hotbar.selectedIndex + 1) % 2);
		posY = self->hotbar.renderPos.y + (self->hotbar.selectedIndex * 56);
	}
	else {
		posX = self->hotbar.renderPos.x + 78 + 34 * ((9) % 2);
		posY = self->hotbar.renderPos.y + (8 * 56) + 6;
	}

	bulletBackgroundRenderer.setPos(posX, posY, 72, 72);
	bulletBackgroundRenderer.setClip(0, 0, 36, 36);
	bulletBackgroundRenderer.setColor(1, 1, 1, 0.4);
	bulletBackgroundRenderer.render();

	bulletRenderer.setPos(posX, posY, 72, 72);
	bulletRenderer.setClip(slingshot->selectedBulletType * 35, 36, 35, 36);
	if (getItemCount(self, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType)) > 0)
		bulletRenderer.setColor(1, 1, 1, 1);
	else
		bulletRenderer.setColor(.2, .2, .2, 1);
	bulletRenderer.render();

	bulletCountText.text = std::to_string(getItemCount(self, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType)));
	bulletCountText.offsetX(posX + 40);
	bulletCountText.offsetY(posY + 45);

	ui.render();
	if (slingshot->isDrawing)
		shotBulletNumText.render(&ui);

	glEnable(GL_DEPTH_TEST);
}

//Initialize UI  when entering world
void viewportCallback(void* user, const glm::ivec4& pos, const glm::ivec2& scroll)
{
	GLFWwindow* window = (GLFWwindow*)user;

	// update the render viewport
	int wWidth, wHeight;
	glfwGetWindowSize(window, &wWidth, &wHeight);
	glViewport(pos.x, wHeight - pos.y - pos.w, pos.z, pos.w);

	// create a 2D projection matrix from the specified dimensions and scroll position
	glm::mat4 projection2D = glm::ortho(0.0f, (float)pos.z, (float)pos.w, 0.0f, -1.0f, 1.0f);
	projection2D = glm::translate(projection2D, { scroll.x, scroll.y, 0 });

	// update all 2D shaders
	const Shader* textShader = ShaderManager::get("textShader");
	textShader->use();
	glUniformMatrix4fv(glGetUniformLocation(textShader->id(), "P"), 1, GL_FALSE, &projection2D[0][0]);

	const Shader* tex2DShader = ShaderManager::get("tex2DShader");
	tex2DShader->use();
	glUniformMatrix4fv(glGetUniformLocation(tex2DShader->id(), "P"), 1, GL_FALSE, &projection2D[0][0]);

	const Shader* quadShader = ShaderManager::get("quadShader");
	quadShader->use();
	glUniformMatrix4fv(glGetUniformLocation(quadShader->id(), "P"), 1, GL_FALSE, &projection2D[0][0]);
}
$hook(void, StateGame, init, StateManager& s)
{
	original(self, s);

	if (VR::inVR())
	{
		return;
	}
	font = { ResourceManager::get("pixelFont.png"), ShaderManager::get("textShader") };

	qr.shader = ShaderManager::get("quadShader");
	qr.init();

	drawIndicator.isHorizontal = false;
	drawIndicator.setFillColor({ 1,1,1,0.5f });
	drawIndicator.setOutlineColor({ 1,1,1,0.5f });
	drawIndicator.setSize(10,50);

	shotBulletNumText.setText("1");
	shotBulletNumText.shadow = true;

	bulletBackgroundRenderer.texture = ItemBlock::tr.texture;
	bulletBackgroundRenderer.shader = ShaderManager::get("tex2DShader");
	bulletBackgroundRenderer.init();
	
	bulletRenderer.texture = ResourceManager::get("assets/textures/items.png", true);
	bulletRenderer.shader = ShaderManager::get("tex2DShader");
	bulletRenderer.init();

	bulletCountText.size = 2;
	bulletCountText.text = "0";
	bulletCountText.shadow = true;

	// initialize the Interface
	ui = gui::Interface{ s.window };
	ui.viewportCallback = viewportCallback;
	ui.viewportUser = s.window;
	ui.font = &font;
	ui.qr = &qr;

	ui.addElement(&bulletCountText);
	ui.addElement(&drawIndicator);
}

$hook(void, StateGame, update, StateManager& s, double dt)
{
	utils::update_time = glfwGetTime();
	utils::update_dt = glm::min(utils::update_time - utils::update_lastTime, 0.1);
	utils::update_lastTime = utils::update_time;

	original(self, s, dt);
}

$hook(void, StateGame, render, StateManager& s)
{
	utils::render_time = glfwGetTime();
	utils::render_dt = glm::min(utils::render_time - utils::render_lastTime, 0.1);
	utils::render_lastTime = utils::render_time;
	static double notDrawingTime = 0.0;
	static float lastDrawFOV = StateSettings::instanceObj.currentFOV + 30;

	original(self, s);

	if (self->player.inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(&self->player);

	if (slingshot && slingshot->isDrawing)
	{
		self->FOV = utils::lerp(StateSettings::instanceObj.currentFOV + 30, (StateSettings::instanceObj.currentFOV + 30) - (StateSettings::instanceObj.currentFOV + 30) * maxZoom, utils::easeInOutQuad(slingshot->drawFraction));
		int width, height;
		glfwGetWindowSize(s.window,&width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
		notDrawingTime = 0.0;
		lastDrawFOV = self->FOV;
	}
	else if (notDrawingTime <= 0.4)
	{
		self->FOV = utils::lerp(lastDrawFOV, StateSettings::instanceObj.currentFOV + 30, utils::easeOutExpo(glm::clamp(notDrawingTime / 0.4, 0.0, 1.0)));
		int width, height;
		glfwGetWindowSize(s.window, &width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
		notDrawingTime += utils::render_dt;
	}
}

$hook(void, WorldServer, WorldServer, const stl::path& worldsPath, const stl::path& settingsPath, const stl::path& biomeInfoPath)
{
	// register networking callbacks
	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_SHOOT, [](WorldServer* world, double dt, const nlohmann::json& data, uint32_t client)
		{
			if (!world->players.contains(client)) return;

			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_SHOOT, data, world->players.at(client).player.get());
		});

	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_UPDATE, [](WorldServer* world, double dt, const nlohmann::json& data, uint32_t client)
		{
			if (!world->players.contains(client)) return;

			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_UPDATE, data, world->players.at(client).player.get());
		});

	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_START_DRAWING, [](WorldServer* world, double dt, const nlohmann::json& data, uint32_t client)
		{
			if (!world->players.contains(client)) return;

			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_START_DRAWING, data, world->players.at(client).player.get());
		});

	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, [](WorldServer* world, double dt, const nlohmann::json& data, uint32_t client)
		{
			if (!world->players.contains(client)) return;

			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, data, world->players.at(client).player.get());
		});

	original(self, worldsPath, settingsPath, biomeInfoPath);
}

$hook(void, StateIntro, init, StateManager& s)
{
	if (VR::isLoaded())
	{
		vrActions.nextBullet = VR::addAction("nextBullet", "Next Bullet", VR::ACTION_BOOL, VR::ACTION_SET_INGAME);
		if (!vrActions.nextBullet)
		{
			printf("[Slingshots] - Failed to init 4DVR \"Next Bullet\" action!\n");
		}

		vrActions.prevBullet = VR::addAction("prevBullet", "Previous Bullet", VR::ACTION_BOOL, VR::ACTION_SET_INGAME);
		if (!vrActions.prevBullet)
		{
			printf("[Slingshots] - Failed to init 4DVR \"Previous Bullet\" action!\n");
		}

		vrActions.drawSlingshot = VR::addAction("drawSlingshot", "Draw Slingshot", VR::ACTION_BOOL, VR::ACTION_SET_INGAME, VR::ACTION_SUGGESTED);
		if (vrActions.drawSlingshot)
		{
			VR::addDefaultBindButton(vrActions.drawSlingshot, VR::CONTROLLER_LEFT, VR::BTN_TRIGGER);
		}
		else
		{
			printf("[Slingshots] - Failed to init 4DVR \"Draw Slingshot\" action!\n");
		}

		vrActions.addBullet = VR::addAction("addBullet", "Add Bullet", VR::ACTION_BOOL, VR::ACTION_SET_INGAME, VR::ACTION_SUGGESTED);
		if (vrActions.addBullet)
		{
			VR::addDefaultBindButton(vrActions.addBullet, VR::CONTROLLER_RIGHT, VR::BTN_TRIGGER);
		}
		else
		{
			printf("[Slingshots] - Failed to init 4DVR \"Add Bullet\" action!\n");
		}
	}

	original(self, s);

	glewExperimental = true;
	glewInit();
	glfwInit();

	EntityProjectile::soundInit();
	EntityProjectile::renderInit();
	ItemSlingshot::renderInit();

	ShaderManager::load("debugShader", "assets/shaders/debug.vs", "assets/shaders/debug.fs");

	// register networking callbacks
	JSONData::SCaddPacketCallback(JSONPacket::S_SLINGSHOT_START_DRAWING, [](WorldClient* world, Player* player, const nlohmann::json& data)
		{
			Entity* e = world->getEntity(stl::uuid()(data["player"].get<std::string>()));
			if (!e) return;
			if (e->getName() != "Player") return;

			ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(((EntityPlayer*)e)->player);
			if (!slingshot) return;

			slingshot->isDrawing = true;
			slingshot->drawFraction = 0.0f;
			slingshot->numBullets = 1;

			EntityProjectile::playStretchSound(e->getPos());
		});

	JSONData::SCaddPacketCallback(JSONPacket::S_SLINGSHOT_STOP_DRAWING, [](WorldClient* world, Player* player, const nlohmann::json& data)
		{
			Entity* e = world->getEntity(stl::uuid()(data["player"].get<std::string>()));
			if (!e) return;
			if (e->getName() != "Player") return;

			ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(((EntityPlayer*)e)->player);
			if (!slingshot) return;

			slingshot->isDrawing = false;
			slingshot->drawFraction = 0.0f;
			slingshot->numBullets = 1;

			if (!data.contains("cancel"))
				EntityProjectile::playSlingshotSound(e->getPos());
		});
}

void localPlayerEventJson(World* world, const stl::string& event, const nlohmann::json& data, Player* player)
{
	if (world->getType() == World::TYPE_CLIENT)
		return JSONData::sendPacketServer((WorldClient*)world, event, data);

	// "Can we have switch/case with strings?"
	// "We already have switch/case with strings at home"
	// switch/case with strings at home:
	static const std::unordered_map<stl::string, void(*)(World* world, const nlohmann::json& data, Player* player)> eventHandlers
	{
		{ JSONPacket::C_SLINGSHOT_SHOOT, [](World* world, const nlohmann::json& data, Player* player)
		{
			handleShoot(world, data);
			if (world->getType() == World::TYPE_SERVER)
			{
				auto* server = (WorldServer*)world;

				if (server->entityPlayerIDs.contains(player->EntityPlayerID))
				{
					auto* playerInfo = server->entityPlayerIDs.at(player->EntityPlayerID);

					for (auto& c : playerInfo->chunkList)
					{
						for (auto& e : c->entities)
						{
							if (e->id != player->EntityPlayerID && e->getName() == "Player" && server->entityPlayerIDs.contains(e->id))
							{
								JSONData::sendPacketClient(server, JSONPacket::S_SLINGSHOT_STOP_DRAWING,
									{
										{ "player", stl::uuid::to_string(player->EntityPlayerID) }
									},
									server->entityPlayerIDs.at(e->id)->handle);
							}
						}
					}
				}
			}
		}
		},
		{ JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, [](World* world, const nlohmann::json& data, Player* player)
		{
			ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
			if (slingshot)
			{
				slingshot->isDrawing = false;
				slingshot->drawFraction = 0.0f;
				slingshot->numBullets = 1;
			}

			if (world->getType() == World::TYPE_SERVER)
			{
				auto* server = (WorldServer*)world;

				if (server->entityPlayerIDs.contains(player->EntityPlayerID))
				{
					auto* playerInfo = server->entityPlayerIDs.at(player->EntityPlayerID);

					for (auto& c : playerInfo->chunkList)
					{
						for (auto& e : c->entities)
						{
							if (e->id != player->EntityPlayerID && e->getName() == "Player" && server->entityPlayerIDs.contains(e->id))
							{
								JSONData::sendPacketClient(server, JSONPacket::S_SLINGSHOT_STOP_DRAWING,
									{
										{ "player", stl::uuid::to_string(player->EntityPlayerID) },
										{ "cancel", true }
									},
									server->entityPlayerIDs.at(e->id)->handle);
							}
						}
					}
				}
			}
		}
		},
		{ JSONPacket::C_SLINGSHOT_START_DRAWING, [](World* world, const nlohmann::json& data, Player* player)
		{
			ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
			if (!slingshot) return;

			slingshot->numBullets = 1;
			slingshot->isDrawing = true;

			if (world->getType() == World::TYPE_SERVER)
			{
				auto* server = (WorldServer*)world;

				if (server->entityPlayerIDs.contains(player->EntityPlayerID))
				{
					auto* playerInfo = server->entityPlayerIDs.at(player->EntityPlayerID);

					for (auto& c : playerInfo->chunkList)
					{
						for (auto& e : c->entities)
						{
							if (e->id != player->EntityPlayerID && e->getName() == "Player" && server->entityPlayerIDs.contains(e->id))
							{
								JSONData::sendPacketClient(server, JSONPacket::S_SLINGSHOT_START_DRAWING,
									{
										{ "player", stl::uuid::to_string(player->EntityPlayerID) }
									},
									server->entityPlayerIDs.at(e->id)->handle);
							}
						}
					}
				}
			}
		}
		},
		{ JSONPacket::C_SLINGSHOT_UPDATE, [](World* world, const nlohmann::json& data, Player* player)
		{
			ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
			if (!slingshot) return;

			slingshot->selectedBulletType = (EntityProjectile::BulletType)(data["selectedBulletType"].get<int>());
		}
		}
	};

	if (eventHandlers.contains(event))
		eventHandlers.at(event)(world, data, player);
}

void changeBulletType(Player* player, int change)
{
	if (!player) return;

	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
	if (!slingshot) return;

	slingshot->selectedBulletType = (EntityProjectile::BulletType)(((uint8_t)(slingshot->selectedBulletType + change)) % EntityProjectile::BULLET_TYPE_COUNT);

	localPlayerEventJson(StateGame::instanceObj.world.get(), JSONPacket::C_SLINGSHOT_UPDATE,
		{
			{ "selectedBulletType", slingshot->selectedBulletType }
		},
		player);

	if (slingshot->isDrawing && getItemCount(player, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType)) == 0)
	{
		localPlayerEventJson(StateGame::instanceObj.world.get(), JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, player);
	}
}

void pickNextBullet(GLFWwindow* window, int action, int mods)
{
	Player* player = &StateGame::instanceObj.player;
	if (action != GLFW_PRESS || !player || player->inventoryManager.isOpen()) return;

	changeBulletType(player, 1);
}

void pickPreviousBullet(GLFWwindow* window, int action, int mods)
{
	Player* player = &StateGame::instanceObj.player;
	if (action != GLFW_PRESS || !player || player->inventoryManager.isOpen()) return;

	changeBulletType(player, -1);
}

$hook(bool, Player, keyInput, GLFWwindow* window, World* world, int key, int scancode, int action, int mods)
{
	if (!KeyBinds::isLoaded() && action == GLFW_PRESS)
	{
		switch (key)
		{
		case GLFW_KEY_R:
			pickNextBullet(window, action, mods);
			break;
		case GLFW_KEY_F:
			pickPreviousBullet(window, action, mods);
			break;
		}
	}

	return original(self, window, world, key, scancode, action, mods);
}

$hook(bool, Player, mouseButtonInput, GLFWwindow* window, World* world, int button, int action, int mods)
{
	if (!VR::inVR())
	{
		ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(self);
		if (slingshot)
		{
			if (slingshot->isDrawing)
			{
				if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
				{
					slingshot->drawFraction *= 0.75f;
					++slingshot->numBullets;
					if (slingshot->numBullets > ItemSlingshot::maxBulletsPerShot)
						slingshot->numBullets = 1;
					shotBulletNumText.setText(std::to_string(slingshot->numBullets));
				}
			}
		}
	}

	return original(self, window, world, button, action, mods);
}

$hook(void, StateGame, windowResize, StateManager& s, int w, int h)
{
	ui.changeViewport({ 0, 0, w, h }, { 0, 0 });
	original(self, s, w, h);
}

// reset drawing on hotbar slot switch
void localPlayerEvent(World* world, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
	if (slingshot && slingshot->isDrawing && eventType == Packet::ClientPacket::C_HOTBAR_SLOT_SELECT)
	{
		localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, player);
	}
}
$hook(void, WorldClient, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	original(self, player, eventType, eventValue, data);
	//localPlayerEvent(self, player, eventType, eventValue, data);
}
$hook(void, WorldSingleplayer, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	original(self, player, eventType, eventValue, data);
	localPlayerEvent(self, player, eventType, eventValue, data);
}

$hook(void, WorldClient, handleMessage, const Connection::InMessage& message, Player* player)
{
	if (message.getPacketType() == Packet::S_INVENTORY_UPDATE)
	{
		ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(player);
		if (!slingshot)
		{
			return original(self, message, player);
		}

		glm::vec4 stringTargetPos = slingshot->stringTargetPos;
		glm::vec4 stringPos = slingshot->stringPos;
		glm::vec4 stringVel = slingshot->stringVel;

		original(self, message, player);

		slingshot = ItemSlingshot::getSlingshot(player);
		if (slingshot)
		{
			slingshot->stringTargetPos = stringTargetPos;
			slingshot->stringPos = stringPos;
			slingshot->stringVel = stringVel;
		}
		return;
	}
	return original(self, message, player);
}

$exec
{
	KeyBinds::addBind("Slingshots", "Next Bullet", glfw::Keys::R, KeyBindsScope::PLAYER, pickNextBullet);
	KeyBinds::addBind("Slingshots", "Previous Bullet", glfw::Keys::F, KeyBindsScope::PLAYER, pickPreviousBullet);
}
