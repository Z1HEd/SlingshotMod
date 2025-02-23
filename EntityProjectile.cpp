#include "EntityProjectile.h"

int EntityProjectile::nextId = 0;
int EntityProjectile::getNextId() { return nextId++; }

stl::string EntityProjectile::getName()
{
	return "Projectile";
}
void EntityProjectile::update(World* world, double dt)
{
	if (timeTillVisible > 0) {
		timeTillVisible -= dt;
	}
	linearVelocity -= glm::vec4{ 0,1,0,0 }*gravity * (float)dt;
	//if (glm::length(linearVelocity) < 0.001) return; // sqrt(x*x + y*y + z*z + w*w) is slow
	if (glm::dot(linearVelocity, linearVelocity) < 0.001f * 0.001f) return;   // x*x + y*y + z*z + w*w is fast (which is what the dot(a,a) does here)
	glm::vec4 newPos = position + linearVelocity * (float)dt;

	Entity* intersect = world->getEntityIntersection(position, newPos, ownerPlayer->EntityPlayerID);

	glm::ivec4 blockPos = position;
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
void EntityProjectile::render(const World* world, const m4::Mat5& MV, bool glasses)
{

	if (timeTillVisible > 0) {
		return;
	}

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

	ItemTool::rockRenderer->render();
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
	linearVelocity = m4::vec4FromJson(j["linearVelocity"]);
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
