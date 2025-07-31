#pragma once

#include <4dm.h>

using namespace fdm;

#include "EntityProjectile.h"

class ItemSlingshot : public Item
{
public:
	static bool entityPlayerRender;

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

	inline static constexpr int maxBulletsPerShot = 5;
	inline static constexpr float stringStiffness = 2650.0f;
	inline static constexpr float stringDamping = 26.0f;

	EntityProjectile::BulletType selectedBulletType = EntityProjectile::BULLET_STONE;
	Type type = TYPE_WOOD;
	float drawFraction = 0;
	bool isDrawing = false;

	glm::vec4 savedOffset{ 0 };
	float savedRotation = 0;
	bool offhand = false;

	int numBullets = 1;

	glm::vec4 stringTargetPos{ 0 };
	glm::vec4 stringPos{ 0 };
	glm::vec4 stringVel{ 0 };

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

	inline static ItemSlingshot* getSlingshot(Player* player)
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
};