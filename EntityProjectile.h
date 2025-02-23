#include <4dm.h>

using namespace fdm;



class EntityProjectile : public Entity
{
public:
	glm::vec4 position;
	glm::vec4 linearVelocity; // In case in the future there would also be angular velocity
	Player* ownerPlayer;
	float damage;
	float gravity = 50;
	float timeTillVisible = 0.02;
	int type;
	
	static stl::string stretchSound;
	static stl::string slingshotSound;
	static stl::string hitSound;
	inline static const char* voiceGroup = "ambience";

	static int nextId;
	static int getNextId();

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
