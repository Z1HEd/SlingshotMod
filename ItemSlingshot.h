#pragma once

#include <4dm.h>

using namespace fdm;

class ItemSlingshot : public Item {
public:
	enum BulletType {
		BULLET_STONE,
		BULLET_IRON,
		BULLET_DEADLY,
		BULLET_SOLENOID
	};

	enum Type { 
		TYPE_WOOD,
		TYPE_DEADLY
	};

	BulletType selectedBulletType = BULLET_STONE;
	Type type = TYPE_WOOD;
	float drawFraction = 0;
	bool isDrawing = false;

	// Virtual functions overrides
	bool isCompatible(const std::unique_ptr<Item>& other) override;
	stl::string getName() override;
	bool isDeadly() override;
	uint32_t getStackLimit() override;
	void render(const glm::ivec2& pos) override;
	void renderEntity(const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir) override;
	std::unique_ptr<Item> clone() override;
	nlohmann::json saveAttributes() override;

};