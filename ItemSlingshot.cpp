#include "ItemSlingshot.h"
#include "utils.h"
#include <4do/4do.h>

using namespace utils;

MeshRenderer ItemSlingshot::renderer{};
const Shader* ItemSlingshot::slingshotShader = nullptr;
MeshRenderer ItemSlingshot::stringRenderer{};
const Shader* ItemSlingshot::stringShader = nullptr;

bool ItemSlingshot::isCompatible(const std::unique_ptr<Item>& other)
{
	auto* otherSlingshot = dynamic_cast<ItemSlingshot*>(other.get());
	return otherSlingshot && otherSlingshot->type == type;
}

stl::string ItemSlingshot::getName() {
	return slingshotTypeNames.at(type);
}

bool ItemSlingshot::isDeadly() {
	return type == TYPE_DEADLY;
}
uint32_t ItemSlingshot::getStackLimit() {
	return 1;
}

void ItemSlingshot::render(const glm::ivec2& pos) {
	TexRenderer& tr = ItemTool::tr;
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/textures/items.png", true); // set to custom texture
	tr.setClip(type * 35, 0, 35, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

void ItemSlingshot::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir)
{	
	glm::vec4 goalOffset{ 0 };
	float goalRotation = 0;

	if (!offhand)
	{
		goalRotation = -glm::pi<float>() / 8.0f;
		goalOffset = glm::vec4{ -1.0f, 0, 0, 0 };
	}
	else
	{
		goalRotation = glm::pi<float>() / 8.0f;
		goalOffset = glm::vec4{ 1.0f, 0, 0, 0 };
	}

	glm::vec4 offset{ 0 };
	float rot = 0;

	if (inHand)
	{
		if (isDrawing) {
			offset = savedOffset = lerp(glm::vec4{ 0 }, goalOffset, easeInOutQuad(drawFraction));
			rot = savedRotation = lerp(0.0f, goalRotation, easeInOutQuad(drawFraction));
		}
		else {
			offset = savedOffset = ilerp(savedOffset, glm::vec4{ 0 }, 0.3f, utils::render_dt);
			rot = savedRotation = ilerp(savedRotation, 0.0f, 0.35f, utils::render_dt);
		}
	}

	constexpr glm::vec3 colors[EntityProjectile::BULLET_TYPE_COUNT]
	{
		glm::vec3{ 240 / 255.0f, 253 / 255.0f, 255 / 255.0f },
		glm::vec3{ 0.7f }
	};
	m4::Rotor rotor = m4::Rotor
	(
		{
			m4::BiVector4{ 1,0,0,0,0,0 }, // XY
			rot
		}
	);

	{
		m4::Mat5 mat = MV;
		mat *= rotor;
		mat.translate(offset);
		//mat *= m4::Rotor{ m4::BiVector4{ 0,0,0,0,0,1 }, glm::pi<float>() * 0.5f }; // xyw test
		mat.scale(glm::vec4{ 0.4f });
		mat.translate(glm::vec4{ 0, (inHand ? -2.5f : 0), 0, 0.0001f });

		slingshotShader->use();
		glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1, &lightDir[0]);
		glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);
		glUniform1i(glGetUniformLocation(slingshotShader->id(), "type"), type);

		renderer.render();
	}

	{
		m4::Mat5 mat = MV;
		mat *= rotor;
		mat.translate(offset);
		//mat *= m4::Rotor{ m4::BiVector4{ 0,0,0,0,0,1 }, glm::pi<float>() * 0.5f }; // xyw test
		mat *= m4::Rotor{ m4::BiVector4{ 0,0,1,0,0,0 }, glm::pi<float>() * 0.25f }; // 45deg XW
		mat.scale(glm::vec4{ 0.4f });
		mat.translate(glm::vec4{ 0, (inHand ? -2.5f : 0) + 5.45f, 0, 0 });
		mat.scale(glm::vec4{ 2.0f, 0.2f, 0.1f, 2.0f });
		mat.translate(glm::vec4{ -0.5f, -0.5f, -0.5f, -0.5001f });

		stringShader->use();
		glUniform1fv(glGetUniformLocation(stringShader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);
		glUniform4f(glGetUniformLocation(stringShader->id(), "inColor"), colors[type].x, colors[type].y, colors[type].z, 0.9f);

		stringRenderer.render();
	}
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
	if (attributes.contains("selectedBulletType"))
		result->selectedBulletType = (EntityProjectile::BulletType)(attributes["selectedBulletType"].get<int>());

	result->type = itemName == "Slingshot" ? ItemSlingshot::TYPE_WOOD : ItemSlingshot::TYPE_DEADLY;

	result->count = count;
	return result;
}

void ItemSlingshot::renderInit()
{
	// slingshot
	{
		fdo::Object obj = fdo::Object::load4DOFromFile((std::filesystem::path(fdm::getModPath(fdm::modID)) / "assets/models/slingshot.4do").string());
		if (obj.isInvalid()) throw std::runtime_error("Failed to open the slingshot.4do model!");

		std::vector<uint32_t> indexBuffer;
		std::vector<fdo::Point> positions;
		std::vector<fdo::Point> normals;
		std::vector<fdo::Color> colors;
		obj.tetrahedralize(indexBuffer, &positions, &normals, nullptr, &colors);

		MeshBuilder mesh{ (int)indexBuffer.size() };

		// pos
		mesh.addBuff(positions.data(), positions.size() * sizeof(fdo::Point));
		mesh.addAttr(GL_FLOAT, 4, sizeof(fdo::Point));
		// normal
		mesh.addBuff(normals.data(), normals.size() * sizeof(fdo::Point));
		mesh.addAttr(GL_FLOAT, 4, sizeof(fdo::Point));
		// color
		mesh.addBuff(colors.data(), colors.size() * sizeof(fdo::Color));
		mesh.addAttr(GL_UNSIGNED_BYTE, 4, sizeof(fdo::Color));

		mesh.setIndexBuff(indexBuffer.data(), indexBuffer.size() * sizeof(uint32_t));

		renderer = &mesh;

		slingshotShader = ShaderManager::load("zihed.slingshot.slingshotShader",
			"assets/shaders/slingshot.vs", "assets/shaders/slingshot.fs", "assets/shaders/slingshot.gs");
	}

	// string
	{
		MeshBuilder mesh{ BlockInfo::HYPERCUBE_FULL_INDEX_COUNT };

		// pos
		mesh.addBuff(BlockInfo::hypercube_full_verts, sizeof(BlockInfo::hypercube_full_verts));
		mesh.addAttr(GL_UNSIGNED_BYTE, 4, sizeof(glm::u8vec4));

		mesh.setIndexBuff(BlockInfo::hypercube_full_indices, sizeof(BlockInfo::hypercube_full_indices));

		stringRenderer = &mesh;

		stringShader = ShaderManager::load("zihed.slingshot.stringShader",
			"assets/shaders/string.vs", "assets/shaders/string.fs", "assets/shaders/string.gs");
	}
}

$hook(void, StateGame, updateProjection, int width, int height)
{
	original(self, width, height);

	ItemSlingshot::slingshotShader->use();
	glUniformMatrix4fv(glGetUniformLocation(ItemSlingshot::slingshotShader->id(), "P"), 1, false, &self->projection3D[0][0]);

	ItemSlingshot::stringShader->use();
	glUniformMatrix4fv(glGetUniformLocation(ItemSlingshot::stringShader->id(), "P"), 1, false, &self->projection3D[0][0]);
}

$hookStatic(bool, Item, loadItemInfo)
{
	bool result = original();

	static bool loaded = false;
	if (loaded) return result;
	loaded = true;

	for (auto& item : ItemSlingshot::slingshotTypeNames)
	{
		Item::blueprints[item.second] =
		{
		{ "type", "slingshot" },
		{ "baseAttributes", {{"selectedBulletType",(int)EntityProjectile::BULLET_TYPE_DEFAULT}}}
		};
	}

	return result;
}

$hookStatic(bool, CraftingMenu, loadRecipes)
{
	bool result = original();

	utils::addRecipe("Slingshot", 1,			{ {{"name", "Stick"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}} });
	utils::addRecipe("Deadly Slingshot", 1,		{ {{"name", "Deadly Bars"}, {"count", 3}},{{"name", "Hypersilk"}, {"count", 3}} });

	return result;
}
