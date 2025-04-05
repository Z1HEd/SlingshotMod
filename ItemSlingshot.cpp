#include "ItemSlingshot.h"
#include "utils.h"

using namespace utils;

bool ItemSlingshot::isCompatible(const std::unique_ptr<Item>& other)
{
	return dynamic_cast<ItemSlingshot*>(other.get());
}

stl::string ItemSlingshot::getName() {
	return isDeadly() ? "Deadly Slingshot" : "Slingshot";
}

bool ItemSlingshot::isDeadly() {
	return type==TYPE_DEADLY;
}
uint32_t ItemSlingshot::getStackLimit() {
	return 1;
}

void ItemSlingshot::render(const glm::ivec2& pos) {
	TexRenderer& tr = ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/Items.png", true); // set to custom texture
	tr.setClip(type==TYPE_WOOD? 0:36, 0, 36, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

void ItemSlingshot::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) {
	static glm::vec4 savedOffset;
	static float savedRotation;
	
	glm::vec4 goalOffset;
	float goalRotation;

	if (inHand) {
		goalRotation = -glm::pi<float>() / 8.0f;
		goalOffset = glm::vec4{ -1.0f, 0, 0, 0 };
	}
	else {
		goalRotation = glm::pi<float>() / 8.0f;
		goalOffset = glm::vec4{ 1.0f, 0, 0, 0 };
	}

	BlockInfo::TYPE handleType;
	if (isDeadly())
		handleType = BlockInfo::MIDNIGHT_LEAF; // Looks bad, but thats the best texture i found
	else
		handleType = BlockInfo::WOOD;

	static double lastTime = glfwGetTime() - 0.01;
	double dt = glfwGetTime() - lastTime;
	lastTime = glfwGetTime();

	glm::vec4 offset{ 0 };
	float rot = 0;

	if (isDrawing) {
		offset = savedOffset = lerp(glm::vec4{ 0 }, goalOffset, easeInOutQuad(drawFraction));
		rot = savedRotation = lerp(0.0f, goalRotation, easeInOutQuad(drawFraction));
	}
	else {
		offset = savedOffset = ilerp(savedOffset, glm::vec4{ 0 }, 0.3f, dt);
		rot = savedRotation = ilerp(savedRotation, 0.0f, 0.35f, dt);
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

	m4::Mat5 handleMat = MV;
	handleMat *= rotor;
	handleMat.translate(glm::vec4{ 0, -0.2f, 0, 0 });
	handleMat.translate(offset);
	handleMat.scale(glm::vec4(0.2f, 1.3f, 0.2f, 0.2f));
	handleMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 connectorMat = MV;
	connectorMat *= rotor;
	connectorMat.translate(glm::vec4{ 0, .55, 0, 0 });
	connectorMat.translate(offset);
	connectorMat.scale(glm::vec4(.6, 0.2f, 0.2f, 0.2f));
	connectorMat *= m4::Mat5(m4::Rotor({ m4::wedge({1, 0, 0, 0}, {0, 1, 0, 0}), glm::pi<float>() / 2 }));
	connectorMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftMat = MV;
	leftMat *= rotor;
	leftMat.translate(glm::vec4{ .4, .95, 0, 0 });
	leftMat.translate(offset);
	leftMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	leftMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightMat = MV;
	rightMat *= rotor;
	rightMat.translate(glm::vec4{ -.4, .95, 0, 0 });
	rightMat.translate(offset);
	rightMat.scale(glm::vec4(.2, 1, 0.2f, 0.2f));
	rightMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 leftStringMat = MV;
	leftStringMat *= rotor;
	leftStringMat.translate(glm::vec4{ .4, 1.37, 0, 0 });
	leftStringMat.translate(offset);
	leftStringMat.scale(glm::vec4(.22, .06f, 0.22f, 0.06f));
	leftStringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 rightStringMat = MV;
	rightStringMat *= rotor;
	rightStringMat.translate(glm::vec4{ -.4, 1.37, 0, 0 });
	rightStringMat.translate(offset);
	rightStringMat.scale(glm::vec4(.22, 0.06f, 0.22f, 0.06f));
	rightStringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	m4::Mat5 stringMat = MV;
	stringMat *= rotor;
	stringMat.translate(glm::vec4{ 0, 1.37, 0, 0 });
	stringMat.translate(offset);
	stringMat.scale(glm::vec4(1, 0.03f, 0.03f, 0.03f));
	stringMat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5f });

	const Tex2D* tex = ResourceManager::get("tiles.png", false);
	const Shader* shaderWood = ShaderManager::get("blockNormalShader");
	const Shader* slingshotShader = ShaderManager::get("tetSolidColorNormalShader");
	tex->use();

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

	ItemMaterial::barRenderer.render();

	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(rightStringMat) / sizeof(float), &rightStringMat[0][0]);

	ItemMaterial::barRenderer.render();

	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(stringMat) / sizeof(float), &stringMat[0][0]);

	ItemMaterial::barRenderer.render();

}

nlohmann::json ItemSlingshot::saveAttributes() {
	return { {"selectedBulletType",(int)selectedBulletType} };
}

// Cloning item
std::unique_ptr<Item> ItemSlingshot::clone() {
	auto result = std::make_unique<ItemSlingshot>();

	result->selectedBulletType = selectedBulletType;
	result->type = type;

	return result;
}

// Instantiating item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes) {

	if (type != "slingshot") return original(itemName, count, type, attributes);

	auto result = std::make_unique<ItemSlingshot>();
	try {
		result->selectedBulletType = (ItemSlingshot::BulletType)(attributes["selectedBulletType"].get<int>());
	}
	catch(std::exception e){} // Compatibility with older versions

	result->type = itemName== "Slingshot" ? ItemSlingshot::TYPE_WOOD : ItemSlingshot::TYPE_DEADLY;

	result->count = count;
	return result;
}
