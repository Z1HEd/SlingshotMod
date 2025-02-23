#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "Projectile.h"
#include "4DKeyBinds.h"

using namespace fdm;

std::vector<std::string> itemNames{
	"Slingshot",
	"Deadly Slingshot",
	"Stone bullet",
	"Iron bullet",
	"Deadly bullet"
};
std::vector<Projectile*> projectiles;

inline const float maxZoom = 0.2;

float drawFraction = 0;
float savedFOV;
bool saveFOV = true;
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

AudioManager stretchSound;

// Initialize the DLLMain
initDLL

int getSelectedBulletCount(InventoryPlayer inventory) {
	int count = 0;
	for (int slot = 0;slot < inventory.getSlotCount(); slot++) {
		Item* i = inventory.getSlot(slot)->get();
		if (i != nullptr && i->getName() == itemNames[2 + selectedBullet]) 
			count += i->count;
		
	}
	return count;
}

void subtractBullet(InventoryPlayer inventory) {
	for (int slot = 0;slot < inventory.getSlotCount(); slot++) {
		Item* i = inventory.getSlot(slot)->get();
		if (i != nullptr && i->getName() == itemNames[2 + selectedBullet]) {
			i->count--;
			if (i->count < 1) inventory.getSlot(slot)->reset();
		}
	}
}

//Initialize opengl stuff
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	glewExperimental = true;
	glewInit();
	glfwInit();
}

//Shooting a slingshot
void shoot(Player* player, World* world,bool isDeadly) {
	
	glm::vec4 speed = (player->reachEndpoint- player->cameraPos);
	float length = glm::length(speed);
	speed /= length;
	
	if (isDeadly) speed *= 150;
	else speed *= 100;


	float clampedDrawFration = std::max(0.3f, drawFraction);
	speed *= clampedDrawFration;

	stl::uuid id= stl::uuid()(std::format("Projectile{}",Projectile::getNextId()));

	

	nlohmann::json attributes;
	attributes["speed.x"] = speed.x;
	attributes["speed.y"] = speed.y;
	attributes["speed.z"] = speed.z;
	attributes["speed.w"] = speed.w;

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
	}

	std::unique_ptr<Entity> projectile = Entity::instantiateEntity("Projectile", id, player->cameraPos, "Projectile", attributes);

	(dynamic_cast<Projectile*>(projectile.get()))->ownerPlayer = player;

	Chunk * chunk=world->getChunkFromCoords(player->cameraPos.x, player->cameraPos.z, player->cameraPos.w);
	world->addEntityToChunk(projectile, chunk);

	subtractBullet(player->inventoryAndEquipment);
}
$hookStatic(std::unique_ptr<Entity>, Entity, instantiateEntity, const stl::string& entityName, const stl::uuid& id, const glm::vec4& pos, const stl::string& type, const nlohmann::json& attributes)
{
	if (type == "Projectile")
	{
		auto result = std::make_unique<Projectile>();

		result->id = id;
		result->setPos(pos);
		result->speed.x = attributes["speed.x"].get<float>();
		result->speed.y = attributes["speed.y"].get<float>();
		result->speed.z = attributes["speed.z"].get<float>();
		result->speed.w = attributes["speed.w"].get<float>();

		result->damage = attributes["damage"].get<float>();
		result->type = attributes["type"].get<int>();

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
			saveFOV = true;
			stl::string sound = "mods/Slingshot/Res/StretchSound.mp3";
			stl::string voiceGroup = "ambience";
			stretchSound.playSound4D(sound, voiceGroup, self->cameraPos, { 0,0,0,0 });
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
	for (auto* p : projectiles) {
		p->update(world, dt);
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

	player = &self->player;

	font = { ResourceManager::get("pixelFont.png"), ShaderManager::get("textShader") };
	
	qr.shader = ShaderManager::get("quadShader");
	qr.init();

	bulletBackgroundRenderer.texture = ItemBlock::tr->texture;
	bulletBackgroundRenderer.shader = ShaderManager::get("tex2DShader");
	bulletBackgroundRenderer.init();
	
	bulletRenderer.texture = ResourceManager::get("Res/Items.png", true);
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

	stretchSound.loadSound("mods/Slingshot/Res/StretchSound.mp3");
	stretchSound.updateBGM();
}
// Update FOV for zoom when aiming
$hook(void, StateGame, render, StateManager& s) {
	
	if (saveFOV) { 
		savedFOV = self->FOV;
		saveFOV = false; 
	}
	if (drawFraction > 0) {
		self->FOV = savedFOV - savedFOV * maxZoom * drawFraction; 
		int width, height;
		glfwGetWindowSize(s.window,&width, &height);
		self->updateProjection(glm::max(width, 1), glm::max(height, 1));
	}
	else { 
		self->FOV = savedFOV;
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



	glm::vec4 offset;
	if (drawFraction > 0) {
		offset = { -1.5, 0, 0, 0 };
	}
	else {
		offset={ 0,0,0,0 };
	}
	glm::vec3 colorDeadly;
	glm::vec3 colorString;
	colorDeadly = glm::vec3{ 0.7f };
	colorString = glm::vec3{ 0.9f,0.9f,1, };

	m4::Mat5 transform{ 1 };
	//transform *= m4::Mat5(m4::Rotor({ m4::wedge({1, 0, 0, 0}, {0, 1, 0, 0}), glm::pi<float>() / 4 }));
	transform.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });
	

	m4::Mat5 handleMat = mat;
	handleMat.translate(glm::vec4{ 0, -0.2f, 0, 0 }+ offset);
	handleMat.scale(glm::vec4(0.2f, 1.3f, 0.2f, 0.2f));
	handleMat *= transform;

	m4::Mat5 connectorMat = mat;
	connectorMat.translate(glm::vec4{ 0, .55, 0, 0 } + offset);
	connectorMat.scale(glm::vec4(.6, 0.2f, 0.2f, 0.2f));
	connectorMat*= m4::Mat5(m4::Rotor({ m4::wedge({1, 0, 0, 0}, {0, 1, 0, 0}), glm::pi<float>() / 2 }));
	connectorMat *= transform;

	m4::Mat5 leftMat = mat;
	leftMat.translate(glm::vec4{ .4, .95, 0, 0 } + offset);
	leftMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	leftMat *= transform;

	m4::Mat5 rightMat = mat;
	rightMat.translate(glm::vec4{ -.4, .95, 0, 0 } + offset);
	rightMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	rightMat *= transform;

	m4::Mat5 leftStringMat = mat;
	leftStringMat.translate(glm::vec4{ .4, 1.37, 0, 0 } + offset);
	leftStringMat.scale(glm::vec4(.22, .06f, 0.22f, 0.06f));
	leftStringMat *= transform;

	m4::Mat5 rightStringMat = mat;
	rightStringMat.translate(glm::vec4{ -.4, 1.37, 0, 0 } + offset);
	rightStringMat.scale(glm::vec4(.22, 0.06f, 0.22f, 0.06f));
	rightStringMat *= transform;

	m4::Mat5 stringMat = mat;
	stringMat.translate(glm::vec4{ 0, 1.37, 0, 0 } + offset);
	stringMat.scale(glm::vec4(1, 0.03f, 0.03f, 0.03f));
	stringMat *= transform;

	const Tex2D* tex = ResourceManager::get("tiles.png", false);
	const Shader* shaderWood = ShaderManager::get("blockNormalShader");
	const Shader* slingshotShader = ShaderManager::get("tetSolidColorNormalShader");
	glBindTexture(tex->target, tex->ID); // i still didn't add Tex2D::use() or Tex2D::id() lollll
	shaderWood->use();

	
	glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1, &lightDir[0]);

	glUniform4fv(glGetUniformLocation(shaderWood->id(), "lightDir"), 1, &lightDir.x);
	glUniform2ui(glGetUniformLocation(shaderWood->id(), "texSize"), 96, 16);
	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(handleMat) / sizeof(float), &handleMat[0][0]);
	
	BlockInfo::renderItemMesh(handleType);

	glUniform4fv(glGetUniformLocation(shaderWood->id(), "lightDir"), 1, &lightDir.x);
	glUniform2ui(glGetUniformLocation(shaderWood->id(), "texSize"), 96, 16);
	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(connectorMat) / sizeof(float), &connectorMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	glUniform4fv(glGetUniformLocation(shaderWood->id(), "lightDir"), 1, &lightDir.x);
	glUniform2ui(glGetUniformLocation(shaderWood->id(), "texSize"), 96, 16);
	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(leftMat) / sizeof(float), &leftMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	glUniform4fv(glGetUniformLocation(shaderWood->id(), "lightDir"), 1, &lightDir.x);
	glUniform2ui(glGetUniformLocation(shaderWood->id(), "texSize"), 96, 16);
	glUniform1fv(glGetUniformLocation(shaderWood->id(), "MV"), sizeof(rightMat) / sizeof(float), &rightMat[0][0]);

	BlockInfo::renderItemMesh(handleType);

	slingshotShader->use();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorString.r, colorString.g, colorString.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(leftStringMat) / sizeof(float), &leftStringMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorString.r, colorString.g, colorString.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(rightStringMat) / sizeof(float), &rightStringMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorString.r, colorString.g, colorString.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(stringMat) / sizeof(float), &stringMat[0][0]);

	ItemMaterial::barRenderer->render();
}

// item slot tool
$hook(void, ItemTool, render, const glm::ivec2& pos)
{
	
	int index = std::find(itemNames.begin(), itemNames.end(), self->name)- itemNames.begin();
	if (index == 5)
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture
	
	tr.texture = ResourceManager::get("Res/Items.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();
	
	tr.texture = ogTex; // return to the original texture
}
// item slot material (probably shouldve made a seperate array of names for it but im too lazy to change it)
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	int index = std::find(itemNames.begin(), itemNames.end(), self->name) - itemNames.begin();
	if (index == 5)
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("Res/Items.png", true); // set to custom texture
	tr.setClip(index * 36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture

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
	for (int i = 2;i < 5; i++) {
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
	// ...
	original(self, s);
	// ...
	initItemNAME();
	// ...
}

void pickNextBullet(GLFWwindow* window, int action, int mods)
{
	if (action == GLFW_PRESS && player!=nullptr && !player->inventoryManager.secondary)
		selectedBullet = (selectedBullet + 1) % 3;
}
void pickPreviousBullet(GLFWwindow* window, int action, int mods)
{
	if (action == GLFW_PRESS && player != nullptr && !player->inventoryManager.secondary && --selectedBullet<0) selectedBullet = 2;
}

$exec
{
	KeyBinds::addBind("Slingshot", "Next Bullet", glfw::Keys::R, KeyBindsScope::PLAYER, pickNextBullet);
	KeyBinds::addBind("Slingshot", "Previous Bullet", glfw::Keys::F, KeyBindsScope::PLAYER, pickPreviousBullet);
}