//#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "EntityProjectile.h"
#include "4DKeyBinds.h"
#include <glm/gtc/random.hpp>

using namespace fdm;

// framerate independent a=lerp(a,b,t)
float deltaRatio(float ratio, double dt, double targetDelta)
{
	const double rDelta = dt / (1.0 / (1.0 / targetDelta));
	const double s = 1.0 - ratio;

	return (float)(1.0 - pow(s, rDelta));
}
float deltaRatio(float ratio, double dt)
{
	return deltaRatio(ratio, dt, 1.0 / 100.0);
}
float lerp(float a, float b, float ratio, bool clampRatio = true)
{
	if (clampRatio)
		ratio = glm::clamp(ratio, 0.f, 1.f);
	return a + (b - a) * ratio;
}
glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float ratio, bool clampRatio = true)
{
	return glm::vec4{ lerp(a.x, b.x, ratio, clampRatio),lerp(a.y, b.y, ratio, clampRatio),lerp(a.z, b.z, ratio, clampRatio),lerp(a.w, b.w, ratio, clampRatio) };
}
float ilerp(float a, float b, float ratio, double dt, bool clampRatio = true)
{
	return lerp(a, b, deltaRatio(ratio, dt), clampRatio);
}
glm::vec4 ilerp(const glm::vec4& a, const glm::vec4& b, float ratio, double dt, bool clampRatio = true)
{
	return lerp(a, b, deltaRatio(ratio, dt), clampRatio);
}
float easeInOutQuad(float x)
{
	return x < 0.5f ? 2.0f * x * x : 1.0f - powf(-2.0f * x + 2.0f, 2.0f) / 2.0f;
}

std::vector<std::string> itemNames{
	"Slingshot",
	"Deadly Slingshot",
	"Stone Bullet",
	"Iron Bullet",
	"Deadly Bullet",
	"Solenoid Bullet"
};

inline const float maxZoom = 0.2;

float drawFraction = 0;
bool isRightButtonPressed = false;
int selectedBullet = 0;
int bulletCount = 0;
bool isHoldingSlingshot = false;
Player* player;

QuadRenderer* simpleRenderer = Item::qr;
QuadRenderer qr{};
TexRenderer bulletBackgroundRenderer;
TexRenderer bulletRenderer;
gui::Text bulletCountText;
gui::Interface ui;
FontRenderer font{};

// Initialize the DLLMain
initDLL

int getSelectedBulletCount(InventoryPlayer& inventory) {
	int count = 0;
	for (int slot = 0;slot < inventory.getSlotCount(); slot++) {
		Item* i = inventory.getSlot(slot)->get();
		if (i != nullptr && i->getName() == itemNames[2 + selectedBullet]) 
			count += i->count;
		
	}
	return count;
}

void subtractBullet(InventoryPlayer& inventory) {
	for (int slot = 0;slot < inventory.getSlotCount(); slot++) {
		Item* i = inventory.getSlot(slot)->get();
		if (i != nullptr && i->getName() == itemNames[2 + selectedBullet]) {
			i->count--;
			if (i->count < 1) inventory.getSlot(slot)->reset();
		}
	}
}

//Shooting a slingshot
void shoot(Player* player, World* world,bool isDeadly) {
	
	constexpr float randomRange = 0.015f;
	constexpr float randomRangeDeadly = 0.007f;
	float range = isDeadly ? randomRangeDeadly : randomRange;

	glm::vec4 linearVelocity = glm::normalize(player->forward + glm::vec4{
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range),
		glm::linearRand(-range, range) });

	if (isDeadly) linearVelocity *= 150;
	else linearVelocity *= 100;
	//if (selectedBullet == 3) linearVelocity *= 0.7;

	float clampedDrawFration = std::max(0.3f, drawFraction);
	linearVelocity *= clampedDrawFration;

	linearVelocity += player->vel;
	

	nlohmann::json attributes;
	attributes["linearVelocity"] = m4::ivec4ToJson(linearVelocity);

	switch (selectedBullet) {
	case 0:
		attributes["damage"] = 7 * clampedDrawFration;
		attributes["type"] = 0;
		break;
	case 1:
		attributes["damage"] = 13 * clampedDrawFration;
		attributes["type"] = 1;
		break;
	case 2:
		attributes["damage"] = 23 * clampedDrawFration;
		attributes["type"] = 2;
		break;
	case 3:
		attributes["damage"] = 17 * clampedDrawFration;
		attributes["type"] = 3;
		break;
	}

	std::unique_ptr<Entity> projectile = Entity::createWithAttributes("Projectile", player->cameraPos, attributes);

	(dynamic_cast<EntityProjectile*>(projectile.get()))->ownerPlayer = player;

	Chunk * chunk=world->getChunkFromCoords(player->cameraPos.x, player->cameraPos.z, player->cameraPos.w);
	world->addEntityToChunk(projectile, chunk);

	AudioManager::playSound4D(EntityProjectile::slingshotSound, EntityProjectile::voiceGroup, player->cameraPos, { 0,0,0,0 });

	subtractBullet(player->inventoryAndEquipment);
}
$hookStatic(std::unique_ptr<Entity>, Entity, instantiateEntity, const stl::string& entityName, const stl::uuid& id, const glm::vec4& pos, const stl::string& type, const nlohmann::json& attributes)
{
	if (type == "projectile")
	{
		auto result = std::make_unique<EntityProjectile>();

		result->id = id;
		result->setPos(pos);
		result->linearVelocity = m4::vec4FromJson(attributes["linearVelocity"]);
		result->damage = attributes["damage"];
		result->type = attributes["type"];

		return result;
	}
	return original(entityName, id, pos, type, attributes);
}

//Right click handling
$hook(void, Player, update, World* world, double dt, EntityPlayer* entityPlayer){
	original(self, world,dt, entityPlayer);

	bulletCount = getSelectedBulletCount(self->inventoryAndEquipment);
	Item* i = self->hotbar.getSlot(self->hotbar.selectedIndex)->get();
	bool isDeadly= (i!=nullptr && i->getName() == "Deadly Slingshot");
	isHoldingSlingshot = (i != nullptr && i->getName()=="Slingshot" || isDeadly);

	if (self->keys.rightMouseDown &&isHoldingSlingshot && bulletCount>0) {
		if (drawFraction == 0) {
			AudioManager::playSound4D(EntityProjectile::stretchSound, EntityProjectile::voiceGroup, self->cameraPos, { 0,0,0,0 });
		}
		if (isDeadly)
			drawFraction = std::min(1.0, drawFraction+dt*1.4);
		else
			drawFraction = std::min(1.0, drawFraction + dt);
	}
	else if (isHoldingSlingshot && drawFraction>0 && bulletCount > 0){
		shoot(self,world,isDeadly);
		drawFraction = 0;
	}
	else {
		drawFraction = 0;
	}
}


// Render UI
$hook(void, Player, renderHud,GLFWwindow* window){
	glDisable(GL_DEPTH_TEST);
	if (isHoldingSlingshot && !(self->inventoryManager.secondary)) {
		
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		if (drawFraction > 0) {
			simpleRenderer->setPos(width / 2 - 30, height / 2 - 50 * drawFraction + 25, 10, 50 * drawFraction);
			simpleRenderer->setQuadRendererMode(GL_TRIANGLES);
			simpleRenderer->setColor(1, 1, 1, 0.5);
			simpleRenderer->render();

			simpleRenderer->setPos(width / 2 - 30, height / 2 - 25, 10, 49);
			simpleRenderer->setQuadRendererMode(GL_LINE_LOOP);
			simpleRenderer->setColor(1, 1, 1, 0.5);
			simpleRenderer->render();

		}
		
		int posX = self->hotbar.renderPos.x + 70 + 34 * ((self->hotbar.selectedIndex + 1) % 2);
		int posY = self->hotbar.renderPos.y + (self->hotbar.selectedIndex * 56);


		bulletBackgroundRenderer.setPos(posX, posY, 72, 72);
		bulletBackgroundRenderer.setClip(0, 0, 36, 36);
		bulletBackgroundRenderer.setColor(1, 1, 1, 0.8);
		bulletBackgroundRenderer.render();
		
		bulletRenderer.setPos(posX, posY, 72, 72);
		bulletRenderer.setClip((selectedBullet + 2) * 36, 0, 36, 36);
		if (bulletCount>0)
			bulletRenderer.setColor(1, 1, 1, 1);
		else
			bulletRenderer.setColor(.2, .2, .2, 1);
		bulletRenderer.render();

		bulletCountText.text = std::to_string(bulletCount);
		bulletCountText.offsetX(posX + 40);
		bulletCountText.offsetY(posY + 45);

		ui.render();
	}
	glEnable(GL_DEPTH_TEST);
	original(self, window);
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

	(*Entity::blueprints)["Projectile"] =
	{
		{ "type", "projectile" },
		{ "baseAttributes",
			{
				{"type", 0},
				{"linearVelocity", {0.0f, 0.0f, 0.0f, 0.0f}},
				{"damage", 0.0f}
			}
		}
	};

	player = &self->player;

	font = { ResourceManager::get("pixelFont.png"), ShaderManager::get("textShader") };
	
	qr.shader = ShaderManager::get("quadShader");
	qr.init();

	bulletBackgroundRenderer.texture = ItemBlock::tr->texture;
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

}
// Update FOV for zoom when aiming
$hook(void, StateGame, render, StateManager& s) {
	static double lastTime = glfwGetTime() - 0.01;
	double dt = glfwGetTime() - lastTime;
	lastTime = glfwGetTime();
	if (drawFraction > 0) {
		self->FOV = lerp(StateSettings::instanceObj->currentFOV+30, (StateSettings::instanceObj->currentFOV + 30) - (StateSettings::instanceObj->currentFOV + 30) * maxZoom, easeInOutQuad(drawFraction));
		int width, height;
		glfwGetWindowSize(s.window,&width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
	else {
		self->FOV = ilerp(self->FOV, StateSettings::instanceObj->currentFOV + 30, 0.38f, dt);
		int width, height;
		glfwGetWindowSize(s.window, &width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
	original(self, s);
}

// entity (on ground or in hand)
$hook(void, ItemTool, renderEntity, const m4::Mat5& mat, bool inHand, const glm::vec4& lightDir)
{
	if (self->name != "Slingshot" && self->name != "Deadly Slingshot")
		return original(self, mat, inHand, lightDir);


	BlockInfo::TYPE handleType;
	if (self->name == "Deadly Slingshot")
		handleType = BlockInfo::MIDNIGHT_LEAF; // Looks bad, but thats the best texture i found
	else
		handleType = BlockInfo::WOOD;

	static double lastTime = glfwGetTime() - 0.01;
	double dt = glfwGetTime() - lastTime;
	lastTime = glfwGetTime();

	static glm::vec4 offset{ 0 };
	static float rot = 0;
	if (drawFraction > 0) {
		offset = lerp(glm::vec4{ 0 }, glm::vec4{ -1.0f, 0, 0, 0 }, easeInOutQuad(drawFraction));
		rot = lerp(0.0f, -glm::pi<float>() / 8.0f, easeInOutQuad(drawFraction));
	}
	else {
		offset = ilerp(offset, glm::vec4{ 0 }, 0.3f, dt);
		rot = ilerp(rot, 0.0f, 0.35f, dt);
	}
	glm::vec3 colorDeadly;
	glm::vec3 colorString;
	colorDeadly = glm::vec3{ 0.7f };
	colorString = glm::vec3{ 0.9f,0.9f,1, };
	m4::Rotor rotor = m4::Rotor
	(
		{
			m4::wedge({1, 0, 0, 0}, {0, 1, 0, 0}), // XY
			rot
		}
	);

	m4::Mat5 handleMat = mat;
	handleMat *= rotor;
	handleMat.translate(glm::vec4{ 0, -0.2f, 0, 0 });
	handleMat.translate(offset);
	handleMat.scale(glm::vec4(0.2f, 1.3f, 0.2f, 0.2f));
	handleMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 connectorMat = mat;
	connectorMat *= rotor;
	connectorMat.translate(glm::vec4{ 0, .55, 0, 0 });
	connectorMat.translate(offset);
	connectorMat.scale(glm::vec4(.6, 0.2f, 0.2f, 0.2f));
	connectorMat*= m4::Mat5(m4::Rotor({ m4::wedge({1, 0, 0, 0}, {0, 1, 0, 0}), glm::pi<float>() / 2 }));
	connectorMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftMat = mat;
	leftMat *= rotor;
	leftMat.translate(glm::vec4{ .4, .95, 0, 0 });
	leftMat.translate(offset);
	leftMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	leftMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightMat = mat;
	rightMat *= rotor;
	rightMat.translate(glm::vec4{ -.4, .95, 0, 0 });
	rightMat.translate(offset);
	rightMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	rightMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftStringMat = mat;
	leftStringMat *= rotor;
	leftStringMat.translate(glm::vec4{ .4, 1.37, 0, 0 });
	leftStringMat.translate(offset);
	leftStringMat.scale(glm::vec4(.22, .06f, 0.22f, 0.06f));
	leftStringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });
 
	m4::Mat5 rightStringMat = mat;
	rightStringMat *= rotor;
	rightStringMat.translate(glm::vec4{ -.4, 1.37, 0, 0 });
	rightStringMat.translate(offset);
	rightStringMat.scale(glm::vec4(.22, 0.06f, 0.22f, 0.06f));
	rightStringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 stringMat = mat;
	stringMat *= rotor;
	stringMat.translate(glm::vec4{ 0, 1.37, 0, 0 });
	stringMat.translate(offset);
	stringMat.scale(glm::vec4(1, 0.03f, 0.03f, 0.03f));
	stringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Tex2D* tex = ResourceManager::get("tiles.png", false);
	const Shader* shaderWood = ShaderManager::get("blockNormalShader");
	const Shader* slingshotShader = ShaderManager::get("tetSolidColorNormalShader");
	glBindTexture(tex->target, tex->ID); // tr1ngle still didn't add Tex2D::use() or Tex2D::id() :skull:

	shaderWood->use();
	glUniform4fv(glGetUniformLocation(shaderWood->id(), "lightDir"), 1, &lightDir.x);
	glUniform2ui(glGetUniformLocation(shaderWood->id(), "texSize"), 96, 16);
	
	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(handleMat) / sizeof(float), &handleMat[0][0]);
	
	BlockInfo::renderItemMesh(handleType);

	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(connectorMat) / sizeof(float), &connectorMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(leftMat) / sizeof(float), &leftMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(rightMat) / sizeof(float), &rightMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	slingshotShader->use();
	glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1, &lightDir.x);
	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorString.r, colorString.g, colorString.b, 1);

	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(leftStringMat) / sizeof(float), &leftStringMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(rightStringMat) / sizeof(float), &rightStringMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(stringMat) / sizeof(float), &stringMat[0][0]);

	ItemMaterial::barRenderer->render();
	
}

// item slot tool
$hook(void, ItemTool, render, const glm::ivec2& pos)
{
	int index = std::find(itemNames.begin(), itemNames.end(), self->name)- itemNames.begin();

	if (index == itemNames.size())
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture
	
	tr.texture = ResourceManager::get("assets/Items.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();
	
	tr.texture = ogTex; // return to the original texture
}
// item slot material (probably shouldve made a seperate array of names for it but im too lazy to change it)
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	int index = std::find(itemNames.begin(), itemNames.end(), self->name) - itemNames.begin();
	if (index == itemNames.size())
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Items.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

$hook(bool, ItemTool, isDeadly)
{
	if (self->name == "Deadly Slingshot")
		return true;
	return original(self);
}

$hook(bool, ItemMaterial, isDeadly)
{
	if (self->name == "Deadly Bullet")
		return true;
	return original(self);
}

// add recipes
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Stick"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", itemNames[0]}, {"count", 1}}}
		}
	);
	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Bars"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", itemNames[1]}, {"count", 1}}}
		}
	);
	
	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Rock"}, {"count", 1}}}},
		{"result", {{"name", itemNames[2]}, {"count", 3}}}
		}
	);
	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Iron Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[3]}, {"count", 9}}}
		}
	);
	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Deadly Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[4]}, {"count", 6}}}
		}
	);
	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Solenoid Bars"}, {"count", 1}}}},
		{"result", {{"name", itemNames[5]}, {"count", 6}}}
		}
	);
	
}

void initItemNAME()
{
	for (int i = 0;i < 2; i++) {
		// add the custom item
		(*Item::blueprints)[itemNames[i]] =
		{
		{ "type", "tool" }, // based on ItemMaterial
		{ "baseAttributes", nlohmann::json::object() } // no attributes
		};
	}
	for (int i = 2;i < itemNames.size(); i++) {
		// add the custom item
		(*Item::blueprints)[itemNames[i]] =
		{
		{ "type", "material" }, // based on ItemMaterial
		{ "baseAttributes", nlohmann::json::object() } // no attributes
		};
	}
}

$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	glewExperimental = true;
	glewInit();
	glfwInit();

	initItemNAME();

	EntityProjectile::stretchSound = std::format("../../{}/assets/StretchSound.ogg", fdm::getModPath(fdm::modID));
	EntityProjectile::slingshotSound = std::format("../../{}/assets/Slingshot.ogg", fdm::getModPath(fdm::modID));
	EntityProjectile::hitSound = std::format("../../{}/assets/Hit.ogg", fdm::getModPath(fdm::modID));

	if (!AudioManager::loadSound(EntityProjectile::stretchSound)) Console::printLine("Cannot load sound (skill issue)");
	if (!AudioManager::loadSound(EntityProjectile::slingshotSound)) Console::printLine("Cannot load sound (skill issue)");
	if (!AudioManager::loadSound(EntityProjectile::hitSound)) Console::printLine("Cannot load sound (skill issue)");

	ShaderManager::load("projectileShader", "../../assets/shaders/tetNormal.vs", "assets/projectile.fs", "../../assets/shaders/tetNormal.gs");
}

void pickNextBullet(GLFWwindow* window, int action, int mods)
{
	if (action == GLFW_PRESS && player!=nullptr && !player->inventoryManager.secondary)
		selectedBullet = (selectedBullet + 1) % 4;
}
void pickPreviousBullet(GLFWwindow* window, int action, int mods)
{
	if (action == GLFW_PRESS && player != nullptr && !player->inventoryManager.secondary && --selectedBullet<0) selectedBullet = 3;
}

$hook(void, StateGame, updateProjection, int width, int height)
{
	original(self, width, height);
	
	const Shader* sh = ShaderManager::get("projectileShader");
	sh->use();
	glUniformMatrix4fv(glGetUniformLocation(sh->id(), "P"), 1, false, &self->projection3D[0][0]);
}

$exec
{
	KeyBinds::addBind("Slingshot", "Next Bullet", glfw::Keys::R, KeyBindsScope::PLAYER, pickNextBullet);
	KeyBinds::addBind("Slingshot", "Previous Bullet", glfw::Keys::F, KeyBindsScope::PLAYER, pickPreviousBullet);
}