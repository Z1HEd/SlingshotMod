//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "EntityProjectile.h"
#include "4DKeyBinds.h"
#include <glm/gtc/random.hpp>
#include "auilib/auilib.h"
#include "utils.h"
#include "ItemSlingshot.h"
#include "JSONData.h"
#include "FullPlayerInventory.h"
#include "JSONPacket.h"

using namespace fdm;

std::vector<std::string> itemNames{
	"Slingshot",
	"Deadly Slingshot",
	"Stone Bullet",
	"Iron Bullet",
	"Deadly Bullet",
	"Solenoid Bullet"
};

inline const float maxZoom = 0.2;

aui::BarIndicator drawIndicator;
QuadRenderer qr{};
TexRenderer bulletBackgroundRenderer;
TexRenderer bulletRenderer;
gui::Text bulletCountText;
gui::Interface ui;
FontRenderer font{};

// Initialize the DLLMain
initDLL

//Shooting a slingshot
int getItemCount(Player* player,const std::string& name) {
	int count = 0;
	for (int slot = 0;slot < FullPlayerInventory::getSlotCount(player); slot++) {
		Item* i = FullPlayerInventory::getSlot(player,slot).get();
		if (i != nullptr && i->getName() == name)
			count += i->count;
	}
	return count;
}
void subtractItem(Player* player, const std::string& name) {
	
	for (int slot = 0;slot < FullPlayerInventory::getSlotCount(player); slot++) {
		Item* i = FullPlayerInventory::getSlot(player, slot).get();
		if (i != nullptr && i->getName() == name) {
			i->count--;
			if (i->count < 1) FullPlayerInventory::getSlot(player, slot).reset();
			return;
		}
	}
}

void shoot(World* world,const nlohmann::json& data) {
	
	const glm::vec4 playerPos = m4::vec4FromJson<float>(data["playerPosition"]);
	const glm::vec4 playerDirection = m4::vec4FromJson<float>(data["playerDirection"]);
	const glm::vec4 playerVelocity = m4::vec4FromJson<float>(data["playerVelocity"]);
	const stl::uuid ownerPlayerId = stl::uuid()(data["ownerPlayerId"].get<std::string>());
	const float drawFraction = data["drawFraction"].get<float>();

	constexpr float randomRange = 0.015f;
	constexpr float randomRangeDeadly = 0.007f;

	Player* player = ((EntityPlayer*)world->getEntity(ownerPlayerId))->player;

	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get());
	if (!slingshot) slingshot = dynamic_cast<ItemSlingshot*>(player->equipment.getSlot(0).get());
	if (!slingshot) return;

	float range = slingshot->isDeadly() ? randomRangeDeadly : randomRange;

	glm::vec4 linearVelocity = glm::normalize(playerDirection + glm::vec4{
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range) });

	if (slingshot->isDeadly()) linearVelocity *= 150;
	else linearVelocity *= 100;
	//if (selectedBullet == 3) linearVelocity *= 0.7;

	float clampedDrawFration = std::max(0.3f, drawFraction);
	linearVelocity *= clampedDrawFration;

	linearVelocity += playerVelocity;
	

	nlohmann::json attributes;
	attributes["linearVelocity"] = m4::vec4ToJson(linearVelocity);

	switch (slingshot->selectedBulletType) {
	case ItemSlingshot::BULLET_STONE:
		attributes["damage"] = 7 * clampedDrawFration;
		attributes["type"] = 0;
		break;
	case ItemSlingshot::BULLET_IRON:
		attributes["damage"] = 13 * clampedDrawFration;
		attributes["type"] = 1;
		break;
	case ItemSlingshot::BULLET_DEADLY:
		attributes["damage"] = 23 * clampedDrawFration;
		attributes["type"] = 2;
		break;
	case ItemSlingshot::BULLET_SOLENOID:
		attributes["damage"] = 17 * clampedDrawFration;
		attributes["type"] = 3;
		break;
	}

	attributes["ownerPlayerId"] = stl::uuid::to_string(ownerPlayerId);

	std::unique_ptr<Entity> projectile = Entity::createWithAttributes("Projectile", playerPos, attributes);

	Chunk * chunk=world->getChunkFromCoords(playerPos.x, playerPos.z, playerPos.w);
	world->addEntityToChunk(projectile, chunk);

	AudioManager::playSound4D(EntityProjectile::slingshotSound, EntityProjectile::voiceGroup, playerPos, { 0,0,0,0 });

	subtractItem(player, itemNames[slingshot->selectedBulletType + 2]);
	//TODO: sync inventory
	// :)
}

//Handle player inputs
$hook(void, Player, update, World* world, double _, EntityPlayer* entityPlayer) { // dt is useless bcs its hardcoded to be 0.01
	original(self, world, _, entityPlayer);
	
	if (world->getType() != World::TYPE_CLIENT && world->getType() != World::TYPE_SINGLEPLAYER) return;
	
	if (self->inventoryManager.isOpen()) return;

	static double lastTime = glfwGetTime() - 0.01;
	double curTime = glfwGetTime();
	double dt = curTime - lastTime;
	lastTime = curTime;

	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(self->hotbar.getSlot(self->hotbar.selectedIndex).get());
	if (!slingshot) slingshot = dynamic_cast<ItemSlingshot*>(self->equipment.getSlot(0).get());
	if (!slingshot) return;

	if (getItemCount(self, itemNames[slingshot->selectedBulletType + 2]) < 1) {
		slingshot->drawFraction = 0;
		return;
	}

	if (self->keys.rightMouseDown) {
		if (slingshot->drawFraction == 0) {
			AudioManager::playSound4D(EntityProjectile::stretchSound, EntityProjectile::voiceGroup, self->cameraPos, { 0,0,0,0 });
		}
		slingshot->drawFraction =std::min(1.0, slingshot->drawFraction + dt * (slingshot->isDeadly() ? 1.4 : 1));
		slingshot->isDrawing = true;
	}
	else if (!self->keys.rightMouseDown && slingshot->drawFraction > 0 ) {
		nlohmann::json data = { 
			{"ownerPlayerId",stl::uuid::to_string(self->EntityPlayerID)},
			{"drawFraction",slingshot->drawFraction},
			{"playerPosition",m4::vec4ToJson(self->cameraPos)},
			{"playerDirection",m4::vec4ToJson(self->forward)},
			{"playerVelocity",m4::vec4ToJson(self->vel)}
		};
		if (world->getType() == World::TYPE_SINGLEPLAYER)
			shoot(world, data);
		else
			JSONData::sendPacketServer((WorldClient*)world, JSONPacket::C_SLINGSHOT_SHOOT, data);
		slingshot->drawFraction = 0;
	}
}

// Render UI
$hook(void, Player, renderHud,GLFWwindow* window){
	original(self, window);

	bool isSlingshotOffhand = false;
	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(self->hotbar.getSlot(self->hotbar.selectedIndex).get());
	if (!slingshot) {
		slingshot = dynamic_cast<ItemSlingshot*>(self->equipment.getSlot(0).get());
		isSlingshotOffhand = true;
	}
	if (!slingshot || self->inventoryManager.isOpen()) return;

	glDisable(GL_DEPTH_TEST);
		
	int width, height;
	glfwGetWindowSize(window, &width, &height);
	drawIndicator.offsetX(width / 2 - 30);
	drawIndicator.offsetY(height / 2 - 50);
	drawIndicator.setFill(slingshot->drawFraction);
	drawIndicator.visible = slingshot->drawFraction > 0;

	int posX, posY;
	if (!isSlingshotOffhand) {
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
	bulletRenderer.setClip((slingshot->selectedBulletType + 2) * 36, 0, 36, 36);
	if (getItemCount(self,itemNames[slingshot->selectedBulletType + 2]) > 0)
		bulletRenderer.setColor(1, 1, 1, 1);
	else
		bulletRenderer.setColor(.2, .2, .2, 1);
	bulletRenderer.render();

	bulletCountText.text = std::to_string(getItemCount(self, itemNames[slingshot->selectedBulletType + 2]));
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
	
	bulletRenderer.texture = ResourceManager::get("assets/Items.png", true);
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
// Update FOV for zoom when aiming
$hook(void, StateGame, render, StateManager& s) {
	original(self, s);
	static double lastTime = glfwGetTime() - 0.01;
	double dt = glfwGetTime() - lastTime;
	lastTime = glfwGetTime();

	if (self->player.inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(self->player.hotbar.getSlot(self->player.hotbar.selectedIndex).get());
	if (!slingshot) slingshot = dynamic_cast<ItemSlingshot*>(self->player.equipment.getSlot(0).get());
	if (!slingshot) return;

	if (slingshot->drawFraction > 0) {
		self->FOV = utils::lerp(StateSettings::instanceObj.currentFOV+30, (StateSettings::instanceObj.currentFOV + 30) - (StateSettings::instanceObj.currentFOV + 30) * maxZoom, utils::easeInOutQuad(slingshot->drawFraction));
		int width, height;
		glfwGetWindowSize(s.window,&width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
	else {
		self->FOV = utils::ilerp(self->FOV, StateSettings::instanceObj.currentFOV + 30, 0.38f, dt);
		int width, height;
		glfwGetWindowSize(s.window, &width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
}

// item slot material (probably shouldve made a seperate array of names for it but im too lazy to change it)
// Im still too lazy to change it
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	int index = std::find(itemNames.begin(), itemNames.end(), self->name) - itemNames.begin();
	if (index == itemNames.size())
		return original(self, pos);

	TexRenderer& tr = ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Items.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

$hook(bool, ItemMaterial, isDeadly)
{
	if (self->name == "Deadly Bullet")
		return true;
	return original(self);
}

// Add recipes
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Stick"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", itemNames[0]}, {"count", 1}}}
		}
	);
	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Bars"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", itemNames[1]}, {"count", 1}}}
		}
	);
	
	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Rock"}, {"count", 1}}}},
		{"result", {{"name", itemNames[2]}, {"count", 3}}}
		}
	);
	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Iron Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[3]}, {"count", 9}}}
		}
	);
	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[4]}, {"count", 6}}}
		}
	);
	CraftingMenu::recipes.push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Solenoid Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[5]}, {"count", 6}}}
		}
	);
	
}

void initEntityBlueprints() {
	(Entity::blueprints)["Projectile"] =
	{
		{ "type", "projectile" },
		{ "baseAttributes",
			{
				{"type", 0},
				{"linearVelocity", {0.0f, 0.0f, 0.0f, 0.0f}},
				{"damage", 0.0f},
				{"ownerPlayerId",""}
			}
		}
	};
}

void initItemBlueprints()
{
	for (int i = 0;i < 2; i++) {
		// add the custom item
		(Item::blueprints)[itemNames[i]] =
		{
		{ "type", "slingshot" },
		{ "baseAttributes", {{"isSelectedBulletDeadly",false}}}
		};
	}
	for (int i = 2;i < itemNames.size(); i++) {
		// add the custom item
		(Item::blueprints)[itemNames[i]] =
		{
		{ "type", "material" }, // based on ItemMaterial
		{ "baseAttributes", nlohmann::json::object() } // no attributes
		};
	}
}

// Init stuff - server

void onPlayerShoot(fdm::WorldServer* world, double dt, const nlohmann::json& data, uint32_t client) {
	auto& playerInfo = world->players.at(client);
	shoot(world, data);
}

$hook(void, WorldServer, WorldServer, const stl::path& worldsPath, const stl::path& settingsPath, const stl::path& biomeInfoPath) {
	original(self, worldsPath, settingsPath, biomeInfoPath);
	
	initItemBlueprints();
	initEntityBlueprints();

	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_SHOOT, onPlayerShoot);
}

// Init stuff - client

$hook(void, StateGame, init,StateManager& s) {
	original(self, s);

	initEntityBlueprints();

	JSONData::CSaddPacketCallback(JSONPacket::C_SLINGSHOT_SHOOT, onPlayerShoot);
}

$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	glewExperimental = true;
	glewInit();
	glfwInit();

	initItemBlueprints();

	EntityProjectile::stretchSound = std::format("../../{}/assets/StretchSound.ogg", fdm::getModPath(fdm::modID));
	EntityProjectile::slingshotSound = std::format("../../{}/assets/Slingshot.ogg", fdm::getModPath(fdm::modID));
	EntityProjectile::hitSound = std::format("../../{}/assets/Hit.ogg", fdm::getModPath(fdm::modID));

	if (!AudioManager::loadSound(EntityProjectile::stretchSound)) Console::printLine("Cannot load sound (skill issue)");
	if (!AudioManager::loadSound(EntityProjectile::slingshotSound)) Console::printLine("Cannot load sound (skill issue)");
	if (!AudioManager::loadSound(EntityProjectile::hitSound)) Console::printLine("Cannot load sound (skill issue)");

	ShaderManager::load("projectileShader", "../../assets/shaders/tetNormal.vs", "assets/projectile.fs", "../../assets/shaders/tetNormal.gs");
}

// Shader doesnt work without it
$hook(void, StateGame, updateProjection, int width, int height)
{
	original(self, width, height);
	
	const Shader* sh = ShaderManager::get("projectileShader");
	sh->use();
	glUniformMatrix4fv(glGetUniformLocation(sh->id(), "P"), 1, false, &self->projection3D[0][0]);
}


//KeyBinds
void pickNextBullet(GLFWwindow* window, int action, int mods)
{
	Player* player = &StateGame::instanceObj.player;
	if (action != GLFW_PRESS || player == nullptr || player->inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get());
	if (!slingshot) slingshot = dynamic_cast<ItemSlingshot*>(player->equipment.getSlot(0).get());
	if (!slingshot) return;

	slingshot->selectedBulletType = (ItemSlingshot::BulletType) ((slingshot->selectedBulletType + 1) % 4);
}

void pickPreviousBullet(GLFWwindow* window, int action, int mods)
{
	Player* player = &StateGame::instanceObj.player;
	if (action != GLFW_PRESS || player == nullptr || player->inventoryManager.isOpen()) return;

	ItemSlingshot* slingshot;
	slingshot = dynamic_cast<ItemSlingshot*>(player->hotbar.getSlot(player->hotbar.selectedIndex).get());
	if (!slingshot) slingshot = dynamic_cast<ItemSlingshot*>(player->equipment.getSlot(0).get());
	if (!slingshot) return;

	slingshot->selectedBulletType = (ItemSlingshot::BulletType)(slingshot->selectedBulletType - 1);
	if (slingshot->selectedBulletType < 0) slingshot->selectedBulletType = ItemSlingshot::BULLET_SOLENOID;
}


$hook(bool, Player, keyInput, GLFWwindow* window, World* world, int key, int scancode, int action, int mods)
{
	if (!KeyBinds::isLoaded() && action == GLFW_PRESS && !self->inventoryManager.isOpen())
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

$exec
{
	KeyBinds::addBind("Slingshot Mod", "Next Bullet", glfw::Keys::R, KeyBindsScope::PLAYER, pickNextBullet);
	KeyBinds::addBind("Slingshot Mod", "Previous Bullet", glfw::Keys::F, KeyBindsScope::PLAYER, pickPreviousBullet);
}