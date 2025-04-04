#include "EntityProjectile.h"

stl::string EntityProjectile::stretchSound="";
stl::string EntityProjectile::slingshotSound="";
stl::string EntityProjectile::hitSound="";

stl::string EntityProjectile::getName()
{
	return "Projectile";
}
void EntityProjectile::update(World* world, double dt)
{
	if (type == 3) // solenoid
	{
		Entity* target = nullptr;
		float targetDist = FLT_MAX;
		glm::vec4 towardsTarget{ 0 };

		//world->entitiesMutex.lock();
		for (int x = -1; x <= 1; ++x)
			for (int z = -1; z <= 1; ++z)
				for (int w = -1; w <= 1; ++w)
				{
					Chunk* chunk = world->getChunkFromCoords(position.x + 8 * x, position.z + 8 * z, position.w + 8 * w);

					if (!chunk) continue;

					for (auto& entity : chunk->entities)
					{
						if (entity == this) continue;
						if (entity->id == ownerPlayerId) continue;
						if (entity->isBlockEntity()) continue;
						if (entity->getName() == "Item") continue;
						if (entity->dead) continue; //in case

						glm::vec4 towardsCurrent = entity->getPos() - getPos();
						float distSqr = glm::dot(towardsCurrent, towardsCurrent);
						if (distSqr < targetDist * targetDist)
						{
							targetDist = glm::sqrt(distSqr);
							target = entity;
							towardsTarget = towardsCurrent;
						}
					}
				}
		//world->entitiesMutex.unlock();

		if (target && targetDist < 7)
		{
			linearVelocity -= linearVelocity * ((float)dt * 13);
			linearVelocity += towardsTarget * (float)dt / (targetDist * targetDist / 450);
		}
	}
	if (timeTillVisible > 0)
		timeTillVisible -= dt;

	linearVelocity -= glm::vec4{ 0,1,0,0 }*gravity * (float)dt;
	//if (glm::length(linearVelocity) < 0.001) return; // sqrt(x*x + y*y + z*z + w*w) is slow
	if (glm::dot(linearVelocity, linearVelocity) < 0.001f * 0.001f) return;   // x*x + y*y + z*z + w*w is fast (which is what the dot(a,a) does here)
	glm::vec4 newPos = position + linearVelocity * (float)dt;

	Entity* intersect = world->getEntityIntersection(position, newPos, ownerPlayerId);

	glm::ivec4 blockPos = position;
	glm::ivec4 endPos;

	if (intersect != nullptr) {
		intersect->takeDamage(damage, world);
		dead = true;
		AudioManager::playSound4D(hitSound, voiceGroup, newPos, { 0,0,0,0 });
		return;
	}
	else if (world->castRay(position, blockPos, endPos, newPos)) {
		dead = BlockInfo::Blocks.at(world->getBlock(endPos)).solid;
		AudioManager::playSound4D(hitSound, voiceGroup, newPos, { 0,0,0,0 });
	}
	position = newPos;
}
void EntityProjectile::render(const World* world, const m4::Mat5& MV, bool glasses)
{
	if (timeTillVisible > 0)
		return;

	glm::vec3 color{ 1 };
	switch (type) {
	case 0: // stone
		color = glm::vec3{ 0.6, 0.6, 0.6 };
		break;
	case 1: // iron
		color = glm::vec3{ 242.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f };
		break;
	case 2: // deadly
		color = glm::vec3{ 232.0f / 255.0f, 77.0f / 255.0f, 193.0f / 255.0f } * 1.4f;
		break;
	case 3: // solenoid
		color = glm::vec3{ 196.0f / 255.0f, 90.0f / 255.0f, 112.0f / 255.0f };
		break;
	}

	m4::Mat5 material = MV;
	material.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	material.translate(position);
	material.scale(glm::vec4{ 0.03f });

	const Shader* slingshotShader = ShaderManager::get("projectileShader");

	slingshotShader->use();

	glUniform4f(glGetUniformLocation(slingshotShader->id(), "lightDir"), 0, 1, 0, 0);
	glUniform4f(glGetUniformLocation(slingshotShader->id(), "inColor"), color.r, color.g, color.b, 1);
	glUniform1fv(glGetUniformLocation(slingshotShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

	ItemTool::rockRenderer.render();

	if (glasses)
	{
		const Shader* wireframeGlassesShader = ShaderManager::get("wireframeGlassesShader");

		wireframeGlassesShader->use();

		glUniform1f(glGetUniformLocation(wireframeGlassesShader->id(), "wFar"), 8.0f);
		glUniform4f(glGetUniformLocation(wireframeGlassesShader->id(), "inColor"), color.r, color.g, color.b, 1);
		glUniform1fv(glGetUniformLocation(wireframeGlassesShader->id(), "MV"), sizeof(material) / sizeof(float), &material[0][0]);

		EntitySpider::wireframeRenderer.render();
	}
}
nlohmann::json EntityProjectile::saveAttributes()
{
	nlohmann::json attributes;
	attributes["linearVelocity"] = m4::vec4ToJson(linearVelocity);
	attributes["damage"] = damage;
	attributes["type"] = type;
	return attributes;
}
nlohmann::json EntityProjectile::getServerUpdateAttributes()
{
	nlohmann::json attributes;
	attributes["linearVelocity"] = m4::vec4ToJson(linearVelocity);
	attributes["damage"] = damage;
	attributes["type"] = type;
	return attributes;
}
void EntityProjectile::applyServerUpdate(const nlohmann::json& j, World* world)
{
	linearVelocity = m4::vec4FromJson<float>(j["linearVelocity"]);
	damage = j["damage"].get<float>();
	type = j["type"].get<int>();
}
glm::vec4 EntityProjectile::getPos()
{
	return position;
}
void EntityProjectile::setPos(const glm::vec4& pos)
{
	position = pos;
}
bool EntityProjectile::action(World* world, Entity* actor, int action, const nlohmann::json& details)
{
	//Console::printLine("Action! actor: ", actor->getName()); //Never actually prints anything, not needed i guess
	return false;
}

//Instantiating a projectile
$hookStatic(std::unique_ptr<Entity>, Entity, instantiateEntity, const stl::string& entityName, const stl::uuid& id, const glm::vec4& pos, const stl::string& type, const nlohmann::json& attributes)
{
	if (type == "projectile")
	{
		auto result = std::make_unique<EntityProjectile>();

		result->id = id;
		result->setPos(pos);
		result->linearVelocity = m4::vec4FromJson<float>(attributes["linearVelocity"]);
		result->damage = attributes["damage"];
		result->type = attributes["type"];
		result->ownerPlayerId = stl::uuid()(attributes["ownerPlayerId"].get<std::string>());
		return result;
	}
	return original(entityName, id, pos, type, attributes);
}