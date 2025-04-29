#pragma once

#include <4dm.h>

using namespace fdm;

#include "EntityProjectile.h"

class ItemSlingshot : public Item {
public:
	enum Type : int
	{
		TYPE_WOOD,
		TYPE_DEADLY,
		TYPE_LAST = TYPE_DEADLY,
		TYPE_COUNT
	};
	inline static const std::unordered_map<fdm::stl::string, Type> slingshotTypes
	{
		{ "Slingshot", TYPE_WOOD },
		{ "Deadly Slingshot", TYPE_DEADLY },
	};
	inline static const std::map<Type, fdm::stl::string> slingshotTypeNames
	{
		{ TYPE_WOOD, "Slingshot" },
		{ TYPE_DEADLY, "Deadly Slingshot" },
	};

	EntityProjectile::BulletType selectedBulletType = EntityProjectile::BULLET_STONE;
	Type type = TYPE_WOOD;
	float drawFraction = 0;
	bool isDrawing = false;

	glm::vec4 savedOffset{ 0 };
	float savedRotation = 0;
	bool offhand = false;

	// Virtual function overrides
	bool isCompatible(const std::unique_ptr<Item>& other) override;
	stl::string getName() override;
	bool isDeadly() override;
	uint32_t getStackLimit() override;
	void render(const glm::ivec2& pos) override;
	void renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) override;
	std::unique_ptr<Item> clone() override;
	nlohmann::json saveAttributes() override;

	static MeshRenderer renderer;
	static const Shader* slingshotShader;
	static MeshRenderer stringRenderer;
	static const Shader* stringShader;
	static void renderInit();
};