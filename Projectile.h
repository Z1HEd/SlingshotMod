#include <4dm.h>

namespace fdm
{
	class Projectile : public Entity
	{
	public:
		glm::vec4 position;
		glm::vec4 direction;
		float velocity;
		stl::string getName() override
		{
			return "Projectile";
		}
		void update(World* world, double dt) override
		{
			position += direction * (float)(velocity * dt);
			
		}
		void render(const World* world, const m4::Mat5& MV, bool glasses) override
		{
			Console::printLine("x: ", position.x, "y: ", position.y, "z: ", position.z, "w: ", position.w);
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
			attributes["direction.x"] = direction.x;
			attributes["direction.y"] = direction.y;
			attributes["direction.z"] = direction.z;
			attributes["direction.w"] = direction.w;

			attributes["velocity"] = velocity;
			return attributes;
		}
		nlohmann::json getServerUpdateAttributes() override
		{
			nlohmann::json attributes;
			attributes["direction.x"] = direction.x;
			attributes["direction.y"] = direction.y;
			attributes["direction.z"] = direction.z;
			attributes["direction.w"] = direction.w;

			attributes["velocity"] = velocity;
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