#pragma once

#include <4dm.h>

using namespace fdm;

class EntityProjectile : public Entity
{
public:
	enum BulletType : int
	{
		BULLET_STONE = 0,
		BULLET_IRON,
		BULLET_DEADLY,
		BULLET_SOLENOID,
		BULLET_TYPE_LAST = BULLET_SOLENOID,
		BULLET_TYPE_COUNT,
		BULLET_TYPE_DEFAULT = BULLET_STONE,
	};
	inline static const std::unordered_map<fdm::stl::string, BulletType> bulletTypes
	{
		{ "Stone Bullet", BULLET_STONE },
		{ "Iron Bullet", BULLET_IRON },
		{ "Deadly Bullet", BULLET_DEADLY },
		{ "Solenoid Bullet", BULLET_SOLENOID },
	};
	inline static const std::map<BulletType, fdm::stl::string> bulletTypeNames
	{
		{ BULLET_STONE, "Stone Bullet" },
		{ BULLET_IRON, "Iron Bullet" },
		{ BULLET_DEADLY, "Deadly Bullet" },
		{ BULLET_SOLENOID, "Solenoid Bullet" },
	};

	inline static constexpr float gravity = 50;
	glm::vec4 position;
	glm::vec4 velocity;
	stl::uuid ownerPlayerId;
	float damage = 0;
	float timeTillVisible = 0.02;
	BulletType type;
	bool playedHitSound = false;
	
	static stl::string stretchSound;
	static stl::string slingshotSound;
	static stl::string hitSound;
	inline static const char* voiceGroup = "ambience";

	static const Shader* projectileShader;
	static void soundInit();
	static void renderInit();
	static void renderBullet(BulletType type, const glm::vec4& position, const m4::Mat5& MV, bool glasses, const glm::vec4& lightDir);
	static void playHitSound(const glm::vec4& position);
	static void playSlingshotSound(const glm::vec4& position);
	static void playStretchSound(const glm::vec4& position);
	stl::string getName() override;
	void update(World* world, double dt) override;
	void render(const World* world, const m4::Mat5& MV, bool glasses) override;
	nlohmann::json saveAttributes() override;
	nlohmann::json getServerUpdateAttributes() override;
	void applyServerUpdate(const nlohmann::json& j, World* world) override;
	glm::vec4 getPos() override;
	void setPos(const glm::vec4& pos) override;
	bool action(World* world, Entity* actor, int action, const nlohmann::json& details) override;
};
