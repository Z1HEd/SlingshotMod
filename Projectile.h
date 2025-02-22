#include <4dm.h>

namespace fdm
{
	class Projectile : public Entity
	{
	public:
		glm::vec4 position;
		glm::vec4 speed;
		Player* ownerPlayer;
		float damage;
		float gravity = 50;
		float timeTillVisible = 0.02;
		int type;

		inline static int nextId=0;
		static int getNextId() { return nextId++; }

		stl::string getName() override
		{
			return "Projectile";
		}
		void update(World* world, double dt) override
		{
			if (timeTillVisible > 0) {
				timeTillVisible -= dt;
			}
			speed -= glm::vec4{ 0,1,0,0 }*gravity* (float)dt;
			if (glm::length(speed)<0.001) return;
			glm::vec4 newPos = position + speed * (float) dt;
			
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
				color = { 0.7,0.7,0.7 };
				break;
			case 1:
				color = { 0.75, 0.8, 0.7 };
				break;
			case 2:
				color = {0.5,0.2,0.5};
				break;
			}

			m4::Mat5 material = MV;
			material.translate(position);
			material.scale(glm::vec4(0.03, 0.03, 0.03, 0.03));

			const Shader* slingshotShader = ShaderManager::get("tetColorNormalShader");
			slingshotShader->use();

			glUniform4f(glGetUniformLocation(slingshotShader->id(), "lightDir"), 0, 1, 0, 0);

			glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), color.r, color.g, color.b, 1);
			glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

			ItemTool::rockRenderer->render();
		}
		nlohmann::json saveAttributes() override
		{
			nlohmann::json attributes;
			attributes["speed.x"] = speed.x;
			attributes["speed.y"] = speed.y;
			attributes["speed.z"] = speed.z;
			attributes["speed.w"] = speed.w;

			attributes["damage"] = damage;
			attributes["type"] = type;
			return attributes;
		}
		nlohmann::json getServerUpdateAttributes() override
		{
			nlohmann::json attributes;
			attributes["speed.x"] = speed.x;
			attributes["speed.y"] = speed.y;
			attributes["speed.z"] = speed.z;
			attributes["speed.w"] = speed.w;

			attributes["damage"] = damage;
			attributes["type"] = type;
			return attributes;
		}
		void applyServerUpdate(const nlohmann::json& j, World* world) override
		{
			//Idk whats that i just copied everything
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
			Console::printLine("Action! actor: ", actor->getName());
			return false;
		}
	};
}