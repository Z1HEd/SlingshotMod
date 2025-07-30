#include "ItemSlingshot.h"
#include "utils.h"
#include <4do/4do.h>
#include "4DVR.h"

using namespace utils;

MeshRenderer ItemSlingshot::renderer{};
const Shader* ItemSlingshot::slingshotShader = nullptr;
MeshRenderer ItemSlingshot::stringRenderer{};
const Shader* ItemSlingshot::stringShader = nullptr;
bool ItemSlingshot::entityPlayerRender = false;

bool ItemSlingshot::isCompatible(const std::unique_ptr<Item>& other)
{
	auto* otherSlingshot = dynamic_cast<ItemSlingshot*>(other.get());
	return otherSlingshot && otherSlingshot->type == type;
}

stl::string ItemSlingshot::getName()
{
	return slingshotTypeNames.at(type);
}

bool ItemSlingshot::isDeadly()
{
	return type == TYPE_DEADLY;
}
uint32_t ItemSlingshot::getStackLimit()
{
	return 1;
}

void ItemSlingshot::render(const glm::ivec2& pos)
{
	TexRenderer& tr = ItemTool::tr;
	const Tex2D* ogTex = tr.texture; // remember the original texture

	tr.texture = ResourceManager::get("assets/textures/items.png", true); // set to custom texture
	tr.setClip(type * 35, 0, 35, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex; // return to the original texture
}

constexpr int stringSegmentCount = 32;
constexpr int stringSegmentCountZ = 3; // idk
void ItemSlingshot::renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir)
{	
	glm::vec4 goalOffset{ 0 };
	float goalRotation = 0;

	if (!VR::inVR())
	{
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
	}

	glm::vec4 offset{ 0 };
	float rot = 0;

	if (inHand)
	{
		if (isDrawing)
		{
			offset = savedOffset = lerp(glm::vec4{ 0 }, goalOffset, easeInOutQuad(drawFraction));
			rot = savedRotation = lerp(0.0f, goalRotation, easeInOutQuad(drawFraction));
		}
		else
		{
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
			-rot
		}
	);

	auto vrMatAdjust = [](m4::Mat5& mat)
		{
			mat.scale(glm::vec4{ 0.65f });
			mat *= m4::Rotor{ m4::BiVector4{ 0, 0, 0, 1, 0, 0 }, glm::radians(-20.0f) };
			mat.translate(glm::vec4{ 0, 0.275f, 0.25f, 0 });
		};

	{
		m4::Mat5 mat = MV;
		if (inHand && !entityPlayerRender)
		{
			if (VR::inVR())
			{
				vrMatAdjust(mat);
			}
			else
			{
				mat *= rotor;
				mat.translate(offset);
			}
		}
		//mat *= m4::Rotor{ m4::BiVector4{ 0,0,0,0,0,1 }, glm::pi<float>() * 0.5f }; // xyw test
		mat.scale(glm::vec4{ inHand && !entityPlayerRender ? 0.4f : 0.3f });
		mat.translate(glm::vec4{ 0.0001f, (inHand || entityPlayerRender ? -2.5001f : 0.0001f), 0.0001f, 0.0001f});

		slingshotShader->use();
		glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1, &lightDir[0]);
		glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);
		glUniform1i(glGetUniformLocation(slingshotShader->id(), "type"), type);

		renderer.render();
	}

	{
		m4::Mat5 view = m4::createCamera(
			{ 0, 0, 0, 0 },
			-StateGame::instanceObj.player.forward,
			StateGame::instanceObj.player.up,
			-StateGame::instanceObj.player.left,
			StateGame::instanceObj.player.over);
		m4::Mat5 MVbutM = m4::Mat5::inverse(view) * MV;
		m4::Mat5 mat = MVbutM;
		if (inHand && !entityPlayerRender)
		{
			if (VR::inVR())
			{
				vrMatAdjust(mat);
			}
			else
			{
				mat *= rotor;
				mat.translate(offset);
			}
		}
		//mat *= m4::Rotor{ m4::BiVector4{ 0,0,0,0,0,1 }, glm::pi<float>() * 0.5f }; // xyw test
		mat *= m4::Rotor{ m4::BiVector4{ 0,0,1,0,0,0 }, glm::pi<float>() * 0.25f }; // 45deg XW
		mat.scale(glm::vec4{ inHand && !entityPlayerRender ? 0.4f : 0.3f });
		mat.translate(glm::vec4{ 0, (inHand || entityPlayerRender ? -2.5f : 0) + 5.45f, 0, 0 });
		mat.scale(glm::vec4{
			2.0f / (float)stringSegmentCount,
			0.2f,
			0.1f / (float)stringSegmentCountZ,
			2.0f / (float)stringSegmentCount });
		mat.translate(glm::vec4{
			(float)stringSegmentCount * -0.5f + 0.001f,
			-0.501f,
			(float)stringSegmentCountZ * -0.5f + 0.001f,
			(float)stringSegmentCount * -0.5f + 0.001f });

		stringShader->use();
		glUniform4fv(glGetUniformLocation(stringShader->id(), "targetPos"), 1, &stringPos[0]);
		glUniform1fv(glGetUniformLocation(stringShader->id(), "model"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);
		glUniform1fv(glGetUniformLocation(stringShader->id(), "view"), sizeof(m4::Mat5) / sizeof(float), &view[0][0]);
		glUniform4f(glGetUniformLocation(stringShader->id(), "inColor"), colors[type].x, colors[type].y, colors[type].z, 0.9f);

		stringRenderer.render();
	}
}

nlohmann::json ItemSlingshot::saveAttributes()
{
	return { {"selectedBulletType",(int)selectedBulletType} };
}

// Cloning item
std::unique_ptr<Item> ItemSlingshot::clone()
{
	auto result = std::make_unique<ItemSlingshot>();

	result->selectedBulletType = selectedBulletType;
	result->type = type;

	return result;
}

// Instantiating item
$hookStatic(std::unique_ptr<Item>, Item, instantiateItem, const stl::string& itemName, uint32_t count, const stl::string& type, const nlohmann::json& attributes)
{
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
		const auto add = [](auto& where, const auto& what, auto count, auto offset)
			{
				for (size_t i = 0; i < count; ++i)
				{
					where.emplace_back(what[i] + offset);
				}
			};

		std::vector<glm::u8vec4> verts{};
		std::vector<float> ts{};
		std::vector<uint32_t> indices{};

		// -y
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int w = 0; w < stringSegmentCount; ++w)
			{
				for (int z = 0; z < stringSegmentCountZ; ++z)
				{
					add(indices, BlockInfo::cube_indices, 20, verts.size());
					add(verts, BlockInfo::cube_verts_y, 8, glm::u8vec4{ x, 0, z, w });
				}
			}
		}
		// +y
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int w = 0; w < stringSegmentCount; ++w)
			{
				for (int z = 0; z < stringSegmentCountZ; ++z)
				{
					add(indices, BlockInfo::cube_indices, 20, verts.size());
					add(verts, BlockInfo::cube_verts_y, 8, glm::u8vec4{ x, 1, z, w });
				}
			}
		}
		// -z
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int w = 0; w < stringSegmentCount; ++w)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_z, 8, glm::u8vec4{ x, 0, 0, w });
			}
		}
		// +z
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int w = 0; w < stringSegmentCount; ++w)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_z, 8, glm::u8vec4{ x, 0, stringSegmentCountZ, w });
			}
		}
		// -w
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int z = 0; z < stringSegmentCountZ; ++z)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_w, 8, glm::u8vec4{ x, 0, z, 0 });
			}
		}
		// +w
		for (int x = 0; x < stringSegmentCount; ++x)
		{
			for (int z = 0; z < stringSegmentCountZ; ++z)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_w, 8, glm::u8vec4{ x, 0, z, stringSegmentCount });
			}
		}
		// -x
		for (int w = 0; w < stringSegmentCount; ++w)
		{
			for (int z = 0; z < stringSegmentCountZ; ++z)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_x, 8, glm::u8vec4{ 0, 0, z, w });
			}
		}
		// +x
		for (int w = 0; w < stringSegmentCount; ++w)
		{
			for (int z = 0; z < stringSegmentCountZ; ++z)
			{
				add(indices, BlockInfo::cube_indices, 20, verts.size());
				add(verts, BlockInfo::cube_verts_x, 8, glm::u8vec4{ stringSegmentCount, 0, z, w });
			}
		}
		for (int i = 0; i < verts.size(); ++i)
		{
			glm::dvec2 xwA
			{
				glm::abs(((double)verts[i].x / (double)stringSegmentCount) * 2.0 - 1.0),
				glm::abs(((double)verts[i].w / (double)stringSegmentCount) * 2.0 - 1.0),
			};
			glm::dvec2 xw = glm::dvec2{ 1.0 } - xwA;

			if (verts[i].x <= 0 + 0 || verts[i].x >= stringSegmentCount - 0)
				xw.x = 0.0;
			if (verts[i].w <= 0 + 0 || verts[i].w >= stringSegmentCount - 0)
				xw.y = 0.0;
			
			float t = glm::mix(
				glm::length(xw) / glm::sqrt(2.0),
				glm::sqrt(xw.x * xw.y),
				glm::max(glm::sqrt(xwA.x * xwA.y), 0.2));
			t =
				glm::clamp(
					(float)glm::pow(t, 0.85)
					, 0.0f, 1.0f
				);
			
			ts.emplace_back(t);
		}

		MeshBuilder stringMesh{ (int)indices.size() };

		// pos
		stringMesh.addBuff(verts.data(), verts.size() * sizeof(glm::u8vec4));
		stringMesh.addAttr(GL_UNSIGNED_BYTE, 4, sizeof(glm::u8vec4));
		// t
		stringMesh.addBuff(ts.data(), ts.size() * sizeof(float));
		stringMesh.addAttr(GL_FLOAT, 1, sizeof(float));

		stringMesh.setIndexBuff(indices.data(), indices.size() * sizeof(uint32_t));

		stringRenderer = MeshRenderer{};
		stringRenderer.setMesh(&stringMesh);

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
