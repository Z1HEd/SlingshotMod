#include <4dm.h>
#include "4DVR.h"
#include "ItemSlingshot.h"

using namespace fdm;

$hook(void, EntityPlayer, update, World* world, double dt)
{
	if (self->ownedPlayer && self->ownedPlayer.get() != &StateGame::instanceObj.player && self->id != StateGame::instanceObj.player.EntityPlayerID)
	{
		ItemSlingshot* slingshot = ItemSlingshot::getSlingshot(self->ownedPlayer.get());
		ItemSlingshot* localSlingshot = ItemSlingshot::getSlingshot(&StateGame::instanceObj.player);
		if (slingshot && localSlingshot != slingshot)
		{
			if (slingshot->isDrawing)
			{
				slingshot->drawFraction =
					std::min(1.0, slingshot->drawFraction + dt / ((slingshot->isDeadly() ? 0.5 : 0.9) + ((slingshot->numBullets - 1.0f) / (ItemSlingshot::maxBulletsPerShot - 1.0f) * 0.4f)));
			}
			else
			{
				slingshot->drawFraction = 0.0f;
			}

			glm::vec4 vel = self->ownedPlayer->vel;
			{
				glm::vec4 stringVelAdd = vel * 20.0f;
				float velL = glm::length(stringVelAdd);
				if (velL >= 0.01f)
				{
					stringVelAdd *= 1.0f / velL;
					stringVelAdd *= glm::min(velL, 20.0f);
				}

				slingshot->stringVel -= stringVelAdd * (float)dt;
			}

			slingshot->stringTargetPos = glm::vec4{ 0 };
			if (!VR::isEntityPlayerInVR(self))
			{
				slingshot->stringTargetPos = -self->ownedPlayer->forward * slingshot->drawFraction * 0.5f;
				(slingshot->offhand ? self->skinRenderer.upperArmAnimL : self->skinRenderer.upperArmAnimR) =
					(glm::asin(self->ownedPlayer->forward.y) + glm::pi<float>() * 0.5f) * 2.0f;
			}
			else
			{
				if (!slingshot->offhand && slingshot->isDrawing)
				{
					m4::Mat5 lMat = VR::getEntityPlayerHandMat(self, VR::CONTROLLER_LEFT);
					lMat.translate(glm::vec4{ 0, 0, 0.04f, 0 });

					m4::Mat5 rMat = VR::getEntityPlayerHandMat(self, VR::CONTROLLER_RIGHT);
					std::swap(*(glm::vec4*)rMat[1], *(glm::vec4*)rMat[2]);
					rMat.scale(glm::vec4{ 0.65f });
					rMat *= m4::Rotor{ m4::BiVector4{ 0, 0, 0, 1, 0, 0 }, glm::radians(-20.0f) };
					rMat.translate(glm::vec4{ 0, -0.275f, 0.25f, 0 });
					rMat.scale(glm::vec4{ 0.4f });
					rMat.translate(glm::vec4{ 0, 0.2f, -0.6f, 0 });

					glm::vec4 lPos = *(glm::vec4*)lMat[4];
					glm::vec4 rPos = *(glm::vec4*)rMat[4];
					glm::vec4 d = rPos - lPos;

					slingshot->stringTargetPos = -d;
				}
			}

			slingshot->stringVel += (slingshot->stringTargetPos - slingshot->stringPos) * ItemSlingshot::stringStiffness * (float)dt;
			slingshot->stringVel *= (float)(1.0 - ItemSlingshot::stringDamping * dt);
			slingshot->stringPos += slingshot->stringVel * (float)dt;
		}
	}
	return original(self, world, dt);
}

$hook(void, EntityPlayer, render, const World* world, const m4::Mat5& MV, bool glasses)
{
	ItemSlingshot::entityPlayerRender = true;
	
	original(self, world, MV, glasses);

	ItemSlingshot::entityPlayerRender = false;
}
