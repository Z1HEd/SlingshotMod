//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "4DKeyBinds.h"
#include <glm/gtc/random.hpp>
#include "auilib/auilib.h"
#include "utils.h"
#include "ItemSlingshot.h"
#include "JSONData.h"
#include "FullPlayerInventory.h"
#include "JSONPacket.h"

#include "EntityProjectile.h"

using namespace fdm;

inline constexpr float maxZoom = 0.2;

aui::BarIndicator drawIndicator;
QuadRenderer qr{};
TexRenderer bulletBackgroundRenderer;
TexRenderer bulletRenderer;
gui::Text bulletCountText;
gui::Interface ui;
FontRenderer font{};

// Initialize the DLLMain
initDLL

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

ItemSlingshot* getSlingshot(Player* player)
{
	ItemSlingshot* slingshot
		= dynamic_cast<ItemSlingshot*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get());
	if (slingshot)
	{
		slingshot->offhand = false;
		return slingshot;
	}
	else
	{
		slingshot = dynamic_cast<ItemSlingshot*>(player->equipment.getSlot(0).get());
		if (slingshot)
		{
			slingshot->offhand = true;
			return slingshot;
		}
	}
	return nullptr;
}

void localPlayerEventJson(World* world, const stl::string& event, const nlohmann::json& data, Player* player);

void handleShoot(World* world, const nlohmann::json& data)
{
	const glm::vec4 playerPos = m4::vec4FromJson<float>(data["playerPosition"]);
	const glm::vec4 playerDirection = m4::vec4FromJson<float>(data["playerDirection"]);
	const glm::vec4 playerVelocity = m4::vec4FromJson<float>(data["playerVelocity"]);
	const stl::uuid ownerPlayerId = stl::uuid()(data["ownerPlayerId"].get<std::string>());
	const float drawFraction = data["drawFraction"].get<float>();

	constexpr float randomRange = 0.012f;
	constexpr float randomRangeDeadly = 0.006f;

	EntityPlayer* ownerPlayerEntity = (EntityPlayer*)world->getEntity(ownerPlayerId);
	if (!ownerPlayerEntity) return;

	Player* player = ownerPlayerEntity->player;

	ItemSlingshot* slingshot = getSlingshot(player);
	if (!slingshot) return;

	const float range = slingshot->isDeadly() ? randomRangeDeadly : randomRange;

	glm::vec4 velocity = glm::normalize(playerDirection + glm::vec4{
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range) });

	if (slingshot->isDeadly()) velocity *= 150;
	else velocity *= 100;
	//if (selectedBullet == 3) velocity *= 0.7;

	float clampedDrawFration = glm::clamp(drawFraction, 0.2f, 1.0f);
	velocity *= clampedDrawFration;

	velocity += playerVelocity;

	nlohmann::json attributes;
	attributes["velocity"] = m4::vec4ToJson(velocity);

	switch (slingshot->selectedBulletType) {
	case EntityProjectile::BULLET_STONE:
		attributes["damage"] = 10 * clampedDrawFration;
		break;
	case EntityProjectile::BULLET_IRON:
		attributes["damage"] = 15 * clampedDrawFration;
		break;
	case EntityProjectile::BULLET_DEADLY:
		attributes["damage"] = 20 * clampedDrawFration;
		break;
	case EntityProjectile::BULLET_SOLENOID:
		attributes["damage"] = 15 * clampedDrawFration;
		break;
	}
	attributes["type"] = (int)slingshot->selectedBulletType;

	attributes["ownerPlayerId"] = stl::uuid::to_string(ownerPlayerId);

	auto projectile = Entity::createWithAttributes("Projectile", playerPos, attributes);

	Chunk* chunk = world->getChunkFromCoords(playerPos.x, playerPos.z, playerPos.w);
	if (chunk)
		world->addEntityToChunk(projectile, chunk);

	subtractItem(player, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType));

	// Sync Inventory
	if (world->getType() == World::TYPE_SERVER)
	{
		auto* server = (WorldServer*)world;
		nlohmann::json invJson = player->saveInventory();
		if (server->entityPlayerIDs.contains(player->EntityPlayerID))
			server->sendMessagePlayer({ Packet::S_INVENTORY_UPDATE, invJson.dump() }, server->entityPlayerIDs.at(player->EntityPlayerID), true);
	}

	slingshot->isDrawing = false;
	slingshot->drawFraction = 0;
}

// Handle player inputs
$hook(void, Player, update, World* world, double dt, EntityPlayer* entityPlayer)
{
	original(self, world, dt, entityPlayer);
	
	if (world->getType() == World::TYPE_SERVER) return; // run on client and singleplayer
	if (self != &StateGame::instanceObj.player) return; // only on the local player
	if (self->inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot = getSlingshot(self);
	if (!slingshot) return;

	if (getItemCount(self, EntityProjectile::bulletTypeNames.at(slingshot->selectedBulletType)) == 0)
	{
		slingshot->drawFraction = 0;
		slingshot->isDrawing = false;
		return;
	}

	if (self->keys.rightMouseDown)
	{
		if (!slingshot->isDrawing)
		{
			EntityProjectile::playStretchSound(self->cameraPos);

			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_START_DRAWING, {}, self);
		}

		slingshot->drawFraction = std::min(1.0, slingshot->drawFraction + dt / (slingshot->isDeadly() ? 0.5 : 0.9));
		slingshot->isDrawing = true;
	}
	else if (slingshot->isDrawing)
	{
		if (slingshot->drawFraction >= 0.1)
		{
			nlohmann::json data = {
				{ "drawFraction", slingshot->drawFraction + 0.01 },
				{ "ownerPlayerId", stl::uuid::to_string(self->EntityPlayerID) },
				{ "playerPosition", m4::vec4ToJson(self->cameraPos) },
				{ "playerDirection", m4::vec4ToJson(self->forward) },
				{ "playerVelocity", m4::vec4ToJson(self->vel) }
			};
			EntityProjectile::playSlingshotSound(self->cameraPos);
			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_SHOOT, data, self);

			slingshot->drawFraction = 0;
			slingshot->isDrawing = false;
		}
		else
		{
			localPlayerEventJson(world, JSONPacket::C_SLINGSHOT_CANCEL_DRAWING, {}, self);

			slingshot->drawFraction = 0;
			slingshot->isDrawing = false;
		}
	}
}

// Render UI
$hook(void, Player, renderHud, GLFWwindow* window)
{
	original(self, window);

	ItemSlingshot* slingshot = getSlingshot(self);
	if (!slingshot) return;
	if (self->inventoryManager.isOpen()) return;

	glDisable(GL_DEPTH_TEST);
		
	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	drawIndicator.offsetX(width / 2 - 30);
	drawIndicator.offsetY(height / 2 - 50);
	drawIndicator.setFill(slingshot->drawFraction);
	drawIndicator.visible = slingshot->drawFraction > 0;

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

	glEnable(GL_DEPTH_TEST);
}

//Initialize UI  when entering world
void viewportCallback(void* user, const glm::ivec4& pos, const glm::ivec2& scroll)
{
	GLFWwindow* window = (GLFWwindow*)user;

	// update the render viewport
	int wWidth, wHeight;
	glfwGetFramebufferSize(window, &wWidth, &wHeight);
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

	font = { ResourceManager::get("pixelFont.png"), ShaderManager::get("textShader") };

	qr.shader = ShaderManager::get("quadShader");
	qr.init();

	drawIndicator.isHorizontal = false;
	drawIndicator.setFillColor({ 1,1,1,0.5f });
	drawIndicator.setOutlineColor({ 1,1,1,0.5f });
	drawIndicator.setSize(10,50);

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

	original(self, s);

	if (self->player.inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot = getSlingshot(&self->player);
	if (!slingshot) return;

	if (slingshot->isDrawing) {
		self->FOV = utils::lerp(StateSettings::instanceObj.currentFOV+30, (StateSettings::instanceObj.currentFOV + 30) - (StateSettings::instanceObj.currentFOV + 30) * maxZoom, utils::easeInOutQuad(slingshot->drawFraction));
		int width, height;
		glfwGetFramebufferSize(s.window,&width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
	else {
		self->FOV = utils::ilerp(self->FOV, StateSettings::instanceObj.currentFOV + 30, 0.38f, utils::render_dt);
		int width, height;
		glfwGetFramebufferSize(s.window, &width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
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
	original(self, s);

	glewExperimental = true;
	glewInit();
	glfwInit();

	EntityProjectile::soundInit();
	EntityProjectile::renderInit();
	ItemSlingshot::renderInit();

	// register networking callbacks
	JSONData::SCaddPacketCallback(JSONPacket::S_SLINGSHOT_START_DRAWING, [](WorldClient* world, Player* player, const nlohmann::json& data)
		{
			Entity* e = world->getEntity(stl::uuid()(data["player"].get<std::string>()));
			if (!e) return;
			if (e->getName() != "Player") return;

			ItemSlingshot* slingshot = getSlingshot(((EntityPlayer*)e)->player);
			if (!slingshot) return;

			slingshot->isDrawing = true;

			EntityProjectile::playStretchSound(e->getPos());
		});

	JSONData::SCaddPacketCallback(JSONPacket::S_SLINGSHOT_STOP_DRAWING, [](WorldClient* world, Player* player, const nlohmann::json& data)
		{
			Entity* e = world->getEntity(stl::uuid()(data["player"].get<std::string>()));
			if (!e) return;
			if (e->getName() != "Player") return;

			ItemSlingshot* slingshot = getSlingshot(((EntityPlayer*)e)->player);
			if (!slingshot) return;

			slingshot->isDrawing = false;
			slingshot->drawFraction = 0;

			if (data.contains("cancel"))
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
			ItemSlingshot* slingshot = getSlingshot(player);
			if (!slingshot) return;

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
			ItemSlingshot* slingshot = getSlingshot(player);
			if (!slingshot) return;

			slingshot->selectedBulletType = (EntityProjectile::BulletType)(data["selectedBulletType"].get<int>());
		}
		}
	};

	if (eventHandlers.contains(event))
		eventHandlers.at(event)(world, data, player);
}

void changeBulletType(Player* player, int change = 1)
{
	if (!player) return;

	ItemSlingshot* slingshot = getSlingshot(player);
	if (!slingshot) return;

	slingshot->selectedBulletType = (EntityProjectile::BulletType)(((uint8_t)(slingshot->selectedBulletType + change)) % EntityProjectile::BULLET_TYPE_COUNT);

	localPlayerEventJson(StateGame::instanceObj.world.get(), JSONPacket::C_SLINGSHOT_UPDATE,
		{
			{ "selectedBulletType", slingshot->selectedBulletType }
		},
		player);
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

// reset drawing on hotbar slot switch
void localPlayerEvent(World* world, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	if (eventType == Packet::ClientPacket::C_HOTBAR_SLOT_SELECT)
	{
		ItemSlingshot* slingshot = getSlingshot(player);
		if (slingshot)
		{
			slingshot->isDrawing = false;
			slingshot->drawFraction = 0;

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
	}
}
$hook(void, WorldClient, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	original(self, player, eventType, eventValue, data);
	localPlayerEvent(self, player, eventType, eventValue, data);
}
$hook(void, WorldServer, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	original(self, player, eventType, eventValue, data);
	localPlayerEvent(self, player, eventType, eventValue, data);
}
$hook(void, WorldSingleplayer, localPlayerEvent, Player* player, Packet::ClientPacket eventType, int64_t eventValue, void* data)
{
	original(self, player, eventType, eventValue, data);
	localPlayerEvent(self, player, eventType, eventValue, data);
}

$exec
{
	KeyBinds::addBind("Slingshots", "Next Bullet", glfw::Keys::R, KeyBindsScope::PLAYER, pickNextBullet);
	KeyBinds::addBind("Slingshots", "Previous Bullet", glfw::Keys::F, KeyBindsScope::PLAYER, pickPreviousBullet);
}
