#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>
#include "Projectile.h"

using namespace fdm;

constexpr std::string NAME = "Slingshot";
std::vector<Projectile*> projectiles;

inline const float maxZoom = 0.2;

float drawFraction = 0;
float savedFOV;
bool saveFOV = true;
bool isRightButtonPressed = false;

// Initialize the DLLMain
initDLL

$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	// initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();
}

void shoot(Player* player, World* world) {
	
	glm::vec4 speed = (player->reachEndpoint- player->cameraPos);
	float length = glm::length(speed);
	speed /= length;
	speed *= 100;

	std::string c = std::to_string(speed.x);
	
	stl::uuid id= stl::uuid()(c.c_str());
	Console::printLine(stl::uuid::to_string(id));

	nlohmann::json attributes;
	attributes["speed.x"] = speed.x;
	attributes["speed.y"] = speed.y;
	attributes["speed.z"] = speed.z;
	attributes["speed.w"] = speed.w;

	attributes["damage"] = 7;

	std::unique_ptr<Entity> projectile = Entity::instantiateEntity("Projectile", id, player->cameraPos, "Projectile", attributes);

	(dynamic_cast<Projectile*>(projectile.get()))->ownerPlayer = player;

	Chunk * chunk=world->getChunkFromCoords(player->cameraPos.x, player->cameraPos.z, player->cameraPos.w);
	world->addEntityToChunk(projectile, chunk);
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

		return result;
	}
	return original(entityName, id, pos, type, attributes);
}

//Doing this here manually because Item::action works, but Item::postAction doesnt- no way to know when button is released
$hook(void, Player, update, World* world, double dt, EntityPlayer* entityPlayer){
	original(self, world,dt, entityPlayer);
	bool isHoldingSlingshot = self->hotbar.getSlot(self->hotbar.selectedIndex)->get()->getName() == "Slingshot";
	if (self->keys.rightMouseDown &&isHoldingSlingshot) {
		if (drawFraction == 0) saveFOV = true;
		drawFraction = std::min(1.0, drawFraction+dt);
	}
	else if (isHoldingSlingshot && drawFraction>0){
		drawFraction = 0;
		shoot(self,world);
	}
	else {
		drawFraction = 0;
	}
	for (auto* p : projectiles) {
		p->update(world, dt);
	}
}

// Update FOV for zoom when aiming
$hook(void, StateGame, render, StateManager& s) {
	
	if (saveFOV) { 
		savedFOV = self->FOV;
		Console::printLine("Saved FOV: ", savedFOV);
		saveFOV = false; 
	}
	if (drawFraction > 0) {
		Console::printLine(self->FOV);
		self->FOV = savedFOV - savedFOV * maxZoom * drawFraction; 
	}
	else { 
		self->FOV = savedFOV;
	}
	original(self, s);
}

// entity (on ground or in hand)
$hook(void, ItemTool, renderEntity, const m4::Mat5& mat, bool inHand, const glm::vec4& lightDir)
{
	if (self->name != NAME)
		return original(self, mat, inHand, lightDir);

	glm::vec4 offset;
	if (drawFraction > 0) {
		offset = { -1.5, 0, 0, 0 };
	}
	else {
		offset={ 0,0,0,0 };
	}

	glm::vec3 colorWood;
	glm::vec3 colorDeadly;
	glm::vec3 colorString;
	colorWood = glm::vec3{ 0.8f,0.4f ,0.25f };
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
	connectorMat.scale(glm::vec4(1, 0.2f, 0.2f, 0.2f));
	connectorMat *= transform;

	m4::Mat5 leftMat = mat;
	leftMat.translate(glm::vec4{ .4, 1.05, 0, 0 } + offset);
	leftMat.scale(glm::vec4(.2, .8f, 0.2f, 0.2f));
	leftMat *= transform;

	m4::Mat5 rightMat = mat;
	rightMat.translate(glm::vec4{ -.4, 1.05, 0, 0 } + offset);
	rightMat.scale(glm::vec4(.2, 0.8f, 0.2f, 0.2f));
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

	const Shader* slingshotShader = ShaderManager::get("tetSolidColorNormalShader");
	slingshotShader->use();
	glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1, &lightDir[0]);

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorWood.r, colorWood.g, colorWood.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(handleMat) / sizeof(float), &handleMat[0][0]);
	
	ItemMaterial::barRenderer->render();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorWood.r, colorWood.g, colorWood.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(connectorMat) / sizeof(float), &connectorMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorWood.r, colorWood.g, colorWood.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(leftMat) / sizeof(float), &leftMat[0][0]);

	ItemMaterial::barRenderer->render();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), colorWood.r, colorWood.g, colorWood.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(rightMat) / sizeof(float), &rightMat[0][0]);

	ItemMaterial::barRenderer->render();

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

// item slot
$hook(void, ItemTool, render, const glm::ivec2& pos)
{
	if (self->name != NAME)
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture
	tr.texture = ResourceManager::get("Res/SlingshotItem.png", true); // set to custom texture
	// this is for a single item texture, not for a texture set like the ogTex
	tr.setClip(0, 0, 36, 36); // no offset, size=texture size
	tr.setPos(pos.x, pos.y, 70, 72); // the render position
	tr.render();
	tr.texture = ogTex; // return to the original texture
}

// add recipe
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Stick"}, {"count", 2}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", NAME}, {"count", 1}}}
		}
	);
}


void initItemNAME()
{
	// add the custom item
	(*Item::blueprints)[NAME] =
	{
	{ "type", "tool" }, // based on ItemMaterial
	{ "baseAttributes", nlohmann::json::object() } // no attributes
	};
}

$hook(void, StateIntro, init, StateManager& s)
{
	// ...
	original(self, s);
	// ...
	initItemNAME();
	// ...
}