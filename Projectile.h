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

		stl::string getName() override
		{
			return "Projectile";
		}
		void update(World* world, double dt) override
		{
			speed -= glm::vec4{ 0,1,0,0 }*gravity* (float)dt;
			glm::vec4 newPos = position + speed * (float) dt;
			
			Entity* intersect = world->getEntityIntersection(position,newPos,ownerPlayer->EntityPlayerID);

			glm::ivec4 blockPos=position;
			glm::ivec4 endPos;

			if (intersect != nullptr) {
				Console::printLine("Hit entity: ", intersect->getName());
				if (auto* spider = dynamic_cast<EntitySpider*> (intersect)) Console::printLine("Spider had hp: ", spider->health);
				intersect->takeDamage(damage, world);
				if (auto* spider = dynamic_cast<EntitySpider*> (intersect)) Console::printLine("Spider now has hp: ", spider->health);
				Chunk* chunk = world->getChunkFromCoords(position.x, position.z, position.w);
				dead = true;
				return;
			}
			else if (world->castRay(position, blockPos, endPos, newPos)) {
				Console::printLine("BLOCK: x: ", endPos.x, " y: ", endPos.y, " z: ", endPos.z, " w: ", endPos.w);
				dead = BlockInfo::Blocks.at(world->getBlock(endPos)).solid;
			}
			
			Console::printLine("x: ", position.x, " y: ", position.y, " z: ", position.z, " w: ", position.w);
			position = newPos;
		}
		void render(const World* world, const m4::Mat5& MV, bool glasses) override
		{
			glm::vec3 color;
			color = glm::vec3{ 0.7f };

			float lightSmth = 1;

			m4::Mat5 material = MV;
			//material.translate(position);
			material.scale(glm::vec4(10,10,10,10));

			const Shader* slingshotShader = ShaderManager::get("tetSolidColorNormalShader");
			slingshotShader->use();

			glUniform4fv(glGetUniformLocation(slingshotShader->id(), "lightDir"), 1,&lightSmth);

			glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), color.r, color.g, color.b, 1);
			glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

			ItemMaterial::barRenderer->render();
		}
		nlohmann::json saveAttributes() override
		{
			nlohmann::json attributes;
			attributes["speed.x"] = speed.x;
			attributes["speed.y"] = speed.y;
			attributes["speed.z"] = speed.z;
			attributes["speed.w"] = speed.w;

			attributes["damage"] = damage;
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
			return attributes;
		}
		void applyServerUpdate(const nlohmann::json& j, World* world) override
		{
			//Idk whats that
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