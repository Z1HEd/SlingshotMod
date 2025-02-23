#include <4dm.h>

namespace fdm
{
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

		inline static int nextId=0;
		static int getNextId() { return nextId++; }

		stl::string getName() override
		{
			return "EntityProjectile";
		}
		void update(World* world, double dt) override
		{
			if (timeTillVisible > 0) {
				timeTillVisible -= dt;
			}
			linearVelocity -= glm::vec4{ 0,1,0,0 }*gravity* (float)dt;
			if (glm::length(linearVelocity)<0.001) return;
			glm::vec4 newPos = position + linearVelocity * (float) dt;
			
			Entity* intersect = world->getEntityIntersection(position,newPos,ownerPlayer->EntityPlayerID);

			glm::ivec4 blockPos=position;
			glm::ivec4 endPos;

			if (intersect != nullptr) {
				intersect->takeDamage(damage, world);
				Chunk* chunk = world->getChunkFromCoords(position.x, position.z, position.w);
				dead = true;
				return;
			}
			else if (world->castRay(position, blockPos, endPos, newPos)) {
				dead = BlockInfo::Blocks.at(world->getBlock(endPos)).solid;
			}
			position = newPos;
		}
		void render(const World* world, const m4::Mat5& MV, bool glasses) override
		{
			
			if (timeTillVisible > 0) {
				return;
			}

			glm::vec3 color;
			switch (type) {
			case 0:
				color =  glm::vec3{ 0.7,0.7,0.7 };
				break;
			case 1:
				color =  glm::vec3{ 0.75, 0.8, 0.7 };
				break;
			case 2:
				color =  glm::vec3{0.5,0.2,0.5};
				break;
			}

			m4::Mat5 material = MV;
			material.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
			material.translate(position);
			material.scale(glm::vec4{ 0.03f });

			Console::printLine("Before get: ", glGetError()); // 1282
			const Shader* slingshotShader = ShaderManager::get("projectileShader");

			slingshotShader->use();

			glUniform4f(glGetUniformLocation(slingshotShader->id(), "lightDir"), 0, 1, 0, 0);
			glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), color.r, color.g, color.b, 1);
			glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

			ItemTool::rockRenderer->render();
		}
		nlohmann::json saveAttributes() override
		{
			nlohmann::json attributes;
			attributes["linearVelocity"] = m4::vec4ToJson(linearVelocity);
			attributes["damage"] = damage;
			attributes["type"] = type;
			return attributes;
		}
		nlohmann::json getServerUpdateAttributes() override
		{
			nlohmann::json attributes;
			attributes["linearVelocity"] = m4::vec4ToJson(linearVelocity);
			attributes["damage"] = damage;
			attributes["type"] = type;
			return attributes;
		}
		void applyServerUpdate(const nlohmann::json& j, World* world) override
		{
			linearVelocity= m4::vec4FromJson(j["linearVelocity"]);
			damage = j["damage"].get<float>();
			type = j["type"].get<int>();
		}
		glm::vec4 getPos() override
		{
			return position;
		}
		void setPos(const glm::vec4& pos) override
		{
			position = pos;
		}
		bool action(World* world, Entity* actor, int action, const nlohmann::json& details) override
		{
			//Console::printLine("Action! actor: ", actor->getName()); //Never actually prints anything, not needed i guess
			return false;
		}
	};
}