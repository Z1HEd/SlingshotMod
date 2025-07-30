#include "EntityProjectile.h"
#include <glm/gtc/random.hpp>
#include "utils.h"

stl::string EntityProjectile::stretchSound = "";
stl::string EntityProjectile::slingshotSound = "";
stl::string EntityProjectile::hitSound = "";
const Shader* EntityProjectile::projectileShader = nullptr;

stl::string EntityProjectile::getName()
{
	return "Projectile";
}
void EntityProjectile::update(World* world, double dt)
{
	if (world->getType() != World::TYPE_CLIENT && type == EntityProjectile::BULLET_SOLENOID)
	{
		Entity* target = nullptr;
		float targetDist = FLT_MAX;
		glm::vec4 towardsTarget{ 0 };

		{
			std::lock_guard mutexLock{ world->entitiesMutex };
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
							if (entity->getName() == "Projectile") continue;
							if (entity->dead) continue;

							glm::vec4 ePos = entity->getPos();
							if (entity->getName() == "Player")
							{
								ePos += glm::vec4{ 0.0f, Player::HEIGHT * 0.5f, 0.0f, 0.0f };
							}

							glm::vec4 towardsCurrent = ePos - getPos();
							float distSqr = glm::dot(towardsCurrent, towardsCurrent);
							if (distSqr < targetDist * targetDist)
							{
								targetDist = glm::sqrt(distSqr);
								target = entity;
								towardsTarget = towardsCurrent;
							}
						}
					}
		}

		if (target && targetDist < 7)
		{
			velocity -= velocity * ((float)dt * 13);
			velocity += towardsTarget * (float)dt / (targetDist * targetDist / 450);
		}
	}
	if (timeTillVisible > 0)
		timeTillVisible -= dt;

	if (world->getType() == World::TYPE_CLIENT && type != EntityProjectile::BULLET_SOLENOID ||
		world->getType() != World::TYPE_CLIENT)
		velocity -= glm::vec4{ 0,1,0,0 } * gravity * (float)dt;

	if (glm::dot(velocity, velocity) < 0.001f * 0.001f) return;

	glm::vec4 oldPos = position;
	position += velocity * (float)dt;

	if (world->getType() != World::TYPE_CLIENT)
	{
		glm::vec4 a = oldPos - velocity * (float)dt * 0.5f;
		glm::vec4 b = position + velocity * (float)dt * 0.5f;
		Entity* intersect = world->getEntityIntersection(a, b, ownerPlayerId);

		glm::ivec4 blockPos = position;
		glm::ivec4 endPos;

		if (intersect != nullptr)
		{
			// reset hit timeout timers because those are unfun
			if (intersect->getName() == "Player")
			{
				((EntityPlayer*)intersect)->player->damageTime = 0.0;
			}
			else if (intersect->getName() == "Spider")
			{
				((EntitySpider*)intersect)->hitTime = 0.0;
			}
			else if (intersect->getName() == "Butterfly")
			{
				((EntityButterfly*)intersect)->hitTime = 0.0;
			}
			intersect->takeDamage(damage, world);

			dead = true;
			playHitSound(position);
		}
		else if (world->castRay(a, blockPos, endPos, b))
		{
			dead = BlockInfo::Blocks.at(world->getBlock(endPos)).solid;
			if (dead)
				playHitSound(position);
		}
	}
}

bool item = false;
void EntityProjectile::renderBullet(EntityProjectile::BulletType type, const glm::vec4& position, const m4::Mat5& MV, bool glasses, const glm::vec4& lightDir)
{
	glm::vec3 color{ 1 };
	switch (type)
	{
	case 0: // stone
		color = glm::vec3{ 0.6, 0.6, 0.6 };
		break;
	case 1: // iron
		color = glm::vec3{ 242.0f / 255.0f, 240.0f / 255.0f, 240.0f / 255.0f };
		break;
	case 2: // deadly
		color = glm::vec3{ 232.0f / 255.0f, 77.0f / 255.0f, 193.0f / 255.0f } * 1.3f;
		break;
	case 3: // solenoid
		color = glm::vec3{ 196.0f / 255.0f, 90.0f / 255.0f, 112.0f / 255.0f };
		break;
	}

	m4::Mat5 mat = MV;
	mat.translate(glm::vec4{ 0.0f, 0.0f, 0.0f, 0.001f });
	mat.translate(position);
	mat.scale(glm::vec4{ size * (item ? 4.0f : 1.0f)});

	projectileShader->use();

	glUniform4fv(glGetUniformLocation(projectileShader->id(), "lightDir"), 1, &lightDir[0]);
	glUniform4f(glGetUniformLocation(projectileShader->id(), "inColor"), color.r, color.g, color.b, 1);
	glUniform1fv(glGetUniformLocation(projectileShader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);

	ItemTool::rockRenderer.render();

	if (glasses)
	{
		const Shader* wireframeGlassesShader = ShaderManager::get("wireframeGlassesShader");

		wireframeGlassesShader->use();

		glUniform1f(glGetUniformLocation(wireframeGlassesShader->id(), "wFar"), 8.0f);
		glUniform4f(glGetUniformLocation(wireframeGlassesShader->id(), "inColor"), color.r, color.g, color.b, 1);
		glUniform1fv(glGetUniformLocation(wireframeGlassesShader->id(), "MV"), sizeof(m4::Mat5) / sizeof(float), &mat[0][0]);

		EntitySpider::wireframeRenderer.render();
	}
	item = false;
}

void EntityProjectile::render(const World* world, const m4::Mat5& MV, bool glasses)
{
	if (timeTillVisible > 0)
		return;

	renderBullet(type, position, MV, glasses, { 0,1,0,0 });
}
nlohmann::json EntityProjectile::saveAttributes()
{
	nlohmann::json attributes;
	attributes["position"] = m4::vec4ToJson(position);
	attributes["velocity"] = m4::vec4ToJson(velocity);
	attributes["damage"] = damage;
	attributes["type"] = (int)type;
	attributes["ownerPlayerId"] = stl::uuid::to_string(ownerPlayerId);
	return attributes;
}
nlohmann::json EntityProjectile::getServerUpdateAttributes()
{
	nlohmann::json serverUpdate = saveAttributes();
	serverUpdate["hit"] = dead;
	return serverUpdate;
}
void EntityProjectile::applyServerUpdate(const nlohmann::json& j, World* world)
{
	position = m4::vec4FromJson<float>(j["position"]);
	velocity = m4::vec4FromJson<float>(j["velocity"]);
	damage = j["damage"].get<float>();
	type = (EntityProjectile::BulletType)j["type"].get<int>();
	ownerPlayerId = stl::uuid()(j["ownerPlayerId"].get<std::string>());

	if (j["hit"])
	{
		playHitSound(position);
		dead = true;
	}
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
	return false;
}

$hookStatic(std::unique_ptr<Entity>, Entity, instantiateEntity, const stl::string& entityName, const stl::uuid& id, const glm::vec4& pos, const stl::string& type, const nlohmann::json& attributes)
{
	if (type == "projectile")
	{
		auto result = std::make_unique<EntityProjectile>();

		result->id = id;
		result->setPos(pos);
		result->velocity = m4::vec4FromJson<float>(attributes["velocity"]);
		result->damage = attributes["damage"];
		result->type = (EntityProjectile::BulletType)attributes["type"].get<int>();
		if (!attributes["ownerPlayerId"].get<std::string>().empty())
			result->ownerPlayerId = stl::uuid()(attributes["ownerPlayerId"].get<std::string>());
		return result;
	}
	return original(entityName, id, pos, type, attributes);
}

void EntityProjectile::renderInit()
{
	projectileShader = ShaderManager::load("zihed.slingshot.projectileShader",
		"../../assets/shaders/tetNormal.vs", "assets/shaders/projectile.fs", "../../assets/shaders/tetNormal.gs");
}

void EntityProjectile::soundInit()
{
	stretchSound = std::format("../../{}/assets/audio/stretch.ogg", fdm::getModPath(fdm::modID));
	slingshotSound = std::format("../../{}/assets/audio/slingshot.ogg", fdm::getModPath(fdm::modID));
	hitSound = std::format("../../{}/assets/audio/hit.ogg", fdm::getModPath(fdm::modID));

	if (!AudioManager::loadSound(stretchSound)) Console::printLine("Cannot load sound \"", stretchSound, "\" (skill issue)");
	if (!AudioManager::loadSound(slingshotSound)) Console::printLine("Cannot load sound \"", slingshotSound, "\" (skill issue)");
	if (!AudioManager::loadSound(hitSound)) Console::printLine("Cannot load sound \"", hitSound, "\" (skill issue)");
}

$hook(void, StateGame, updateProjection, int width, int height)
{
	original(self, width, height);

	EntityProjectile::projectileShader->use();
	glUniformMatrix4fv(glGetUniformLocation(EntityProjectile::projectileShader->id(), "P"), 1, false, &self->projection3D[0][0]);
}

void EntityProjectile::playHitSound(const glm::vec4& position)
{
	if (fdm::isServer()) return;
	SoLoud::handle soundHandle
		= AudioManager::playSound4D(EntityProjectile::hitSound, EntityProjectile::voiceGroup, position, { 0,0,0,0 });

	AudioManager::soloud.setRelativePlaySpeed(soundHandle, glm::linearRand(0.9f, 1.1f));
}
void EntityProjectile::playSlingshotSound(const glm::vec4& position)
{
	if (fdm::isServer()) return;
	SoLoud::handle soundHandle
		= AudioManager::playSound4D(EntityProjectile::slingshotSound, EntityProjectile::voiceGroup, position, { 0,0,0,0 });

	AudioManager::soloud.setRelativePlaySpeed(soundHandle, glm::linearRand(0.9f, 1.1f));
}
void EntityProjectile::playStretchSound(const glm::vec4& position)
{
	if (fdm::isServer()) return;
	SoLoud::handle soundHandle
		= AudioManager::playSound4D(EntityProjectile::stretchSound, EntityProjectile::voiceGroup, position, { 0,0,0,0 });

	AudioManager::soloud.setRelativePlaySpeed(soundHandle, glm::linearRand(0.9f, 1.1f));
}

$hookStatic(bool, Entity, loadEntityInfo)
{
	bool result = original();

	static bool loaded = false;
	if (loaded) return result;
	loaded = true;

	Entity::blueprints["Projectile"] =
	{
		{ "type", "projectile" },
		{ "baseAttributes",
			{
				{ "type", 0 },
				{ "velocity", std::array<float, 4>{ 0.0f, 0.0f, 0.0f, 0.0f } },
				{ "damage", 0.0f },
				{ "ownerPlayerId","" }
			}
		}
	};

	return result;
}


$hookStatic(bool, Item, loadItemInfo)
{
	bool result = original();

	static bool loaded = false;
	if (loaded) return result;
	loaded = true;

	for (auto& item : EntityProjectile::bulletTypeNames)
	{
		Item::blueprints[item.second] =
		{
		{ "type", "material" }, // ItemMaterial
		{ "baseAttributes", nlohmann::json::object() }
		};
	}

	return result;
}

$hookStatic(bool, CraftingMenu, loadRecipes)
{
	bool result = original();

	utils::addRecipe("Stone Bullet", 6,		{ {{"name", "Rock"}, {"count", 1}} });
	utils::addRecipe("Iron Bullet", 18,		{ {{"name", "Iron Bars"}, {"count", 1}} });
	utils::addRecipe("Deadly Bullet", 12,	{ {{"name", "Deadly Bars"}, {"count", 1}} });
	utils::addRecipe("Solenoid Bullet", 12,	{ {{"name", "Solenoid Bars"}, {"count", 1}} });

	return result;
}

$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	if (!EntityProjectile::bulletTypes.contains(self->name))
	{
		return original(self, pos);
	}

	TexRenderer& tr = ItemTool::tr;
	const Tex2D* ogTex = tr.texture;

	tr.texture = ResourceManager::get("assets/textures/items.png", true);
	tr.setClip(EntityProjectile::bulletTypes.at(self->name) * 35, 36, 35, 36);
	tr.setPos(pos.x, pos.y, 70, 72);
	tr.render();

	tr.texture = ogTex;
}

$hook(bool, ItemMaterial, isDeadly)
{
	if (EntityProjectile::bulletTypes.contains(self->name) &&
		EntityProjectile::bulletTypes.at(self->name) == EntityProjectile::BULLET_DEADLY)
	{
		return true;
	}
	return original(self);
}

$hook(void, ItemMaterial, renderEntity, const m4::Mat5& MV, bool inHand, const glm::vec4& lightDir)
{
	if (EntityProjectile::bulletTypes.contains(self->name))
	{
		::item = true;
		return EntityProjectile::renderBullet(EntityProjectile::bulletTypes.at(self->name), { 0,0,-0.1f,0 }, MV, false, lightDir);
	}
	return original(self, MV, inHand, lightDir);
}
