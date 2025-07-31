#pragma once

#include <4dm.h>

namespace VR
{
	inline const fdm::stl::string id = "tr1ngledev.4dvr";

	inline bool isLoaded()
	{
		return fdm::isModLoaded(id);
	}

	enum Controller : int
	{
		CONTROLLER_LEFT,
		CONTROLLER_RIGHT,
		CONTROLLER_COUNT,
	};
	enum Eye : int
	{
		EYE_LEFT,
		EYE_RIGHT,
		EYE_DESKTOP,
		EYE_COUNT,
	};

	class Haptic
	{
	public:
		bool destroy = true;
		double duration = 0;
		double startTime = glfwGetTime();
		Haptic(double duration = 0)
			: duration(duration)
		{
		}
		virtual float lerpRatio(double time)
		{
			return glm::clamp((float)((time - startTime) / duration), 0.0f, 1.0f);
		}
		virtual float frequency(double time) = 0;
		virtual float amplitude(double time) = 0;
	};
	class HapticPulse : public Haptic
	{
	public:
		float startFrequency = 0;
		float startAmplitude = 0;
		float endFrequency = 0;
		float endAmplitude = 0;
		HapticPulse(float startFrequency, float endFrequency, float startAmplitude, float endAmplitude, double duration)
			: startFrequency(startFrequency), startAmplitude(startAmplitude), endFrequency(endFrequency), endAmplitude(endAmplitude), Haptic(duration)
		{
		}
		float frequency(double time) override
		{
			return glm::mix(startFrequency, endFrequency, lerpRatio(time));
		}
		float amplitude(double time) override
		{
			return glm::mix(startAmplitude, endAmplitude, lerpRatio(time));
		}
	};
	inline void addHapticEvent(Controller controller, Haptic* haptic)
	{
		if (!isLoaded()) return;
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, Haptic*)>
			(fdm::getModFuncPointer(id, "addHapticEvent"));
		return func(controller, haptic);
	}
	inline void stopAllHaptics(Controller controller)
	{
		if (!isLoaded()) return;
		static auto func = reinterpret_cast<void(__stdcall*)(Controller)>
			(fdm::getModFuncPointer(id, "stopAllHaptics"));
		return func(controller);
	}
	inline void stopAllHaptics()
	{
		stopAllHaptics(CONTROLLER_LEFT);
		stopAllHaptics(CONTROLLER_RIGHT);
	}
	inline bool isHapticActive(Controller controller, Haptic* haptic)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(Controller, Haptic*)>
			(fdm::getModFuncPointer(id, "isHapticActive"));
		return func(controller, haptic);
	}

	using ActionHandle = uint64_t;
	enum ActionType : int
	{
		ACTION_BOOL,
		ACTION_FLOAT,
		ACTION_VEC2,
		ACTION_TYPE_COUNT,
	};
	enum ActionSet : int
	{
		ACTION_SET_UI,
		ACTION_SET_INGAME,
		ACTION_SET_COUNT,
	};
	enum ActionRequirement : int
	{
		ACTION_MANDATORY,
		ACTION_SUGGESTED,
		ACTION_OPTIONAL,
		ACTION_REQ_COUNT,
	};
	/**
	 * Has to be called BEFORE StateIntro::init() (so, in an $exec for example)
	 *
	 * Adds an action for Steam VR Input to bind on controllers
	 * @param[in] id Internal identifier for the action.
	 * @param[in] name The Display Name shown to the user in the Steam VR Input Bindings Menu.
	 * @param[in] type The Action Type.
	 * @param[in] set The Action Set the Action will be added into.
	 * @returns A handle to pass to other functions.
	 */
	inline ActionHandle addAction(const fdm::stl::string& id, const fdm::stl::string& name, ActionType type, ActionSet set, ActionRequirement requirement = ACTION_OPTIONAL)
	{
		if (!isLoaded()) return 0;
		static auto func = reinterpret_cast<ActionHandle(__stdcall*)(const fdm::stl::string&, const fdm::stl::string&, const fdm::stl::string&, ActionType, ActionSet, ActionRequirement)>
			(fdm::getModFuncPointer(VR::id, "addAction"));
		return func(fdm::modID, id, name, type, set, requirement);
	}
	enum Button : int
	{
		BTN_A,
		BTN_B,
		BTN_X = BTN_A,
		BTN_Y = BTN_B,
		BTN_SYSTEM,
		BTN_TRIGGER,
		BTN_GRIP,
		BTN_JOYSTICK,
		BTN_COUNT,
	};
	enum ButtonInteraction : int
	{
		BTN_CLICK,
		BTN_TOUCH,
		BTN_LONG,
		BTN_HELD,
		BTN_DOUBLE_PRESS,
		BTN_INTERACTION_COUNT,
	};
	/**
	 * Has to be called BEFORE StateIntro::init() (so, in an $exec for example)
	 *
	 * Binds a bool ActionHandle to a Button Interaction.
	 * @param[in] action The action.
	 * @param[in] controller The controller.
	 * @param[in] button The button.
	 * @param[in] interaction The interaction type.
	 * @returns false if the action type is incorrect or the action is invalid.
	 */
	inline bool addDefaultBindButton(ActionHandle action, Controller controller, Button button, ButtonInteraction interaction = BTN_CLICK)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, Controller, Button, ButtonInteraction)>
			(fdm::getModFuncPointer(id, "addDefaultBindButton"));
		return func(action, controller, button, interaction);
	}
	/**
	 * Has to be called BEFORE StateIntro::init() (so, in an $exec for example)
	 *
	 * Binds a vec2/float ActionHandle to a Joystick.
	 * @param[in] action The action.
	 * @param[in] controller The controller.
	 * @returns false if the action type is incorrect or the action is invalid.
	 */
	inline bool addDefaultBindJoystick(ActionHandle action, Controller controller)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, Controller)>
			(fdm::getModFuncPointer(id, "addDefaultBindJoystick"));
		return func(action, controller);
	}
	enum DPadDirection
	{
		DPAD_EAST,
		DPAD_NORTH,
		DPAD_SOUTH,
		DPAD_WEST,

		DPAD_RIGHT = DPAD_EAST,
		DPAD_UP = DPAD_NORTH,
		DPAD_DOWN = DPAD_SOUTH,
		DPAD_LEFT = DPAD_WEST,

		DPAD_DIRECTION_COUNT,
	};
	/**
	 * Has to be called BEFORE StateIntro::init() (so, in an $exec for example)
	 *
	 * Binds 4 bool ActionHandles to a Joystick.
	 * @param[in] actions The actions.
	 * @param[in] controller The controller.
	 * @param[in] deadzone The deadzone.
	 * @param[in] overlap Overlap of adjacent sectors before switching direction, in radians (from 0 to pi/2).
	 * @returns false if the action type is incorrect or the action is invalid.
	 */
	inline bool addDefaultBindDPad(ActionHandle actions[DPAD_DIRECTION_COUNT], Controller controller, float deadzone = 0.15f, float overlap = 0.0f)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle[DPAD_DIRECTION_COUNT], Controller, float, float)>
			(fdm::getModFuncPointer(id, "addDefaultBindDPad"));
		return func(actions, controller, deadzone, overlap);
	}
	enum Trigger : int
	{
		TRIGGER,
		GRIP,
		TRIGGER_COUNT,
	};
	/**
	 * Has to be called BEFORE StateIntro::init() (so, in an $exec for example)
	 *
	 * Binds a float ActionHandle to a Button Interaction.
	 * @param[in] action The action.
	 * @param[in] controller The controller.
	 * @param[in] trigger The trigger type (trigger/grip).
	 * @returns false if the action type is incorrect or the action is invalid.
	 */
	inline bool addDefaultBindTrigger(ActionHandle action, Controller controller, Trigger trigger = TRIGGER)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, Controller, Trigger)>
			(fdm::getModFuncPointer(id, "addDefaultBindTrigger"));
		return func(action, controller, trigger);
	}

	/**
	 * @param[in] action The action to get the current value of.
	 * @param[out] output The value output.
	 * @returns false if the action type is incorrect or the action is invalid.
	 */
	inline bool getInput(ActionHandle action, bool& output)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, bool&)>
			(fdm::getModFuncPointer(id, "getInputBool"));
		return func(action, output);
	}
	inline bool getInput(ActionHandle action, float& output)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, float&)>
			(fdm::getModFuncPointer(id, "getInputFloat"));
		return func(action, output);
	}
	inline bool getInput(ActionHandle action, glm::vec2& output)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(ActionHandle, glm::vec2&)>
			(fdm::getModFuncPointer(id, "getInputVec2"));
		return func(action, output);
	}

	inline bool isControllerConnected(Controller controller)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(Controller)>
			(fdm::getModFuncPointer(id, "isControllerConnected"));
		return func(controller);
	}

	inline glm::mat4 getHead()
	{
		if (!isLoaded()) return glm::mat4{ 1 };
		glm::mat4 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::mat4&)>
			(fdm::getModFuncPointer(id, "getHead"));
		func(result);
		return result;
	}
	// same as getHead() unless in StateGame
	inline fdm::m4::Mat5 getHead4D()
	{
		if (!isLoaded()) return fdm::m4::Mat5{ 1 };
		fdm::m4::Mat5 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(fdm::m4::Mat5&)>
			(fdm::getModFuncPointer(id, "getHead4D"));
		func(result);
		return result;
	}

	inline glm::mat4 getController(Controller controller)
	{
		if (!isLoaded()) return glm::mat4{ 1 };
		glm::mat4 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, glm::mat4&)>
			(fdm::getModFuncPointer(id, "getController"));
		func(controller, result);
		return result;
	}
	// same as getController() unless in StateGame
	inline fdm::m4::Mat5 getController4D(Controller controller)
	{
		if (!isLoaded()) return fdm::m4::Mat5{ 1 };
		fdm::m4::Mat5 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, fdm::m4::Mat5&)>
			(fdm::getModFuncPointer(id, "getController4D"));
		func(controller, result);
		return result;
	}
	// includes the head position
	inline glm::vec3 getEyePos(Eye eye)
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Eye, glm::vec3&)>
			(fdm::getModFuncPointer(id, "getEyePos"));
		func(eye, result);
		return result;
	}
	// same as getEyePos() unless in StateGame
	inline glm::vec4 getEyePos4D(Eye eye)
	{
		if (!isLoaded()) return glm::vec4{ 0 };
		glm::vec4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Eye, glm::vec4&)>
			(fdm::getModFuncPointer(id, "getEyePos4D"));
		func(eye, result);
		return result;
	}

	inline glm::vec3 getHeadPos()
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::vec3&)>
			(fdm::getModFuncPointer(id, "getHeadPos"));
		func(result);
		return result;
	}
	// same as getHeadPos() unless in StateGame
	inline glm::vec4 getHeadPos4D()
	{
		if (!isLoaded()) return glm::vec4{ 0 };
		glm::vec4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::vec4&)>
			(fdm::getModFuncPointer(id, "getHeadPos4D"));
		func(result);
		return result;
	}
	inline glm::vec3 getControllerPos(Controller controller)
	{
		return getController(controller)[3];
	}
	// same as getControllerPos() unless in StateGame
	inline glm::vec4 getControllerPos4D(Controller controller)
	{
		return *(glm::vec4*)getController4D(controller)[4];
	}

	// m/s
	inline glm::vec3 getHeadVel()
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::vec3&)>
			(fdm::getModFuncPointer(id, "getHeadVel"));
		func(result);
		return result;
	}
	// same as getHeadVel() unless in StateGame
	inline glm::vec4 getHeadVel4D()
	{
		if (!isLoaded()) return glm::vec4{ 0 };
		glm::vec4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::vec4&)>
			(fdm::getModFuncPointer(id, "getHeadVel4D"));
		func(result);
		return result;
	}

	// m/s
	inline glm::vec3 getControllerVel(Controller controller)
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, glm::vec3&)>
			(fdm::getModFuncPointer(id, "getControllerVel"));
		func(controller, result);
		return result;
	}
	// same as getControllerVel() unless in StateGame
	inline glm::vec4 getControllerVel4D(Controller controller)
	{
		if (!isLoaded()) return glm::vec4{ 0 };
		glm::vec4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, glm::vec4&)>
			(fdm::getModFuncPointer(id, "getControllerVel4D"));
		func(controller, result);
		return result;
	}

	// rad/s
	inline glm::vec3 getHeadAngVel()
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(glm::vec3&)>
			(fdm::getModFuncPointer(id, "getHeadAngVel"));
		func(result);
		return result;
	}
	// same as getHeadAngVel() unless in StateGame
	inline fdm::m4::BiVector4 getHeadAngVel4D()
	{
		if (!isLoaded()) return fdm::m4::BiVector4{ 0 };
		fdm::m4::BiVector4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(fdm::m4::BiVector4&)>
			(fdm::getModFuncPointer(id, "getHeadAngVel4D"));
		func(result);
		return result;
	}

	// rad/s
	inline glm::vec3 getControllerAngVel(Controller controller)
	{
		if (!isLoaded()) return glm::vec3{ 0 };
		glm::vec3 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, glm::vec3&)>
			(fdm::getModFuncPointer(id, "getControllerAngVel"));
		func(controller, result);
		return result;
	}
	// same as getControllerAngVel() unless in StateGame
	inline fdm::m4::BiVector4 getControllerAngVel4D(Controller controller)
	{
		if (!isLoaded()) return fdm::m4::BiVector4{ 0 };
		fdm::m4::BiVector4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(Controller, fdm::m4::BiVector4&)>
			(fdm::getModFuncPointer(id, "getControllerAngVel4D"));
		func(controller, result);
		return result;
	}

	inline glm::vec3 getHeadLeft()
	{
		return getHead()[0];
	}
	// same as getHeadLeft() unless in StateGame
	inline glm::vec4 getHeadLeft4D()
	{
		return *(glm::vec4*)getHead4D()[0];
	}

	inline glm::vec3 getControllerLeft(Controller controller)
	{
		return getController(controller)[0];
	}
	// same as getControllerLeft() unless in StateGame
	inline glm::vec4 getControllerLeft4D(Controller controller)
	{
		return *(glm::vec4*)getController4D(controller)[0];
	}

	inline glm::vec3 getHeadUp()
	{
		return getHead()[1];
	}
	// same as getHeadUp() unless in StateGame
	inline glm::vec4 getHeadUp4D()
	{
		return *(glm::vec4*)getHead4D()[1];
	}

	inline glm::vec3 getControllerUp(Controller controller)
	{
		return getController(controller)[1];
	}
	// same as getControllerUp() unless in StateGame
	inline glm::vec4 getControllerUp4D(Controller controller)
	{
		return *(glm::vec4*)getController4D(controller)[1];
	}

	inline glm::vec3 getHeadForward()
	{
		return getHead()[2];
	}
	// same as getHeadForward() unless in StateGame
	inline glm::vec4 getHeadForward4D()
	{
		return *(glm::vec4*)getHead4D()[2];
	}

	inline glm::vec3 getControllerForward(Controller controller)
	{
		return getController(controller)[2];
	}
	// same as getControllerForward() unless in StateGame
	inline glm::vec4 getControllerForward4D(Controller controller)
	{
		return *(glm::vec4*)getController4D(controller)[2];
	}
	// {0,0,0,1} unless in StateGame
	inline glm::vec4 getHeadOver()
	{
		return *(glm::vec4*)getHead4D()[3];
	}
	// {0,0,0,1} unless in StateGame
	inline glm::vec4 getControllerOver(Controller controller)
	{
		return *(glm::vec4*)getController4D(controller)[3];
	}

	inline bool isEntityPlayerInVR(const fdm::EntityPlayer* entity)
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)(const fdm::EntityPlayer*)>
			(fdm::getModFuncPointer(id, "isEntityPlayerInVR"));
		return func(entity);
	}

	inline glm::mat4 getProjectionMatrix(Eye eye)
	{
		if (!isLoaded()) return glm::mat4{ 1 };
		glm::mat4 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(Eye, glm::mat4&)>
			(fdm::getModFuncPointer(id, "getProjectionMatrix"));
		func(eye, result);
		return result;
	}
	inline glm::mat4 getViewMatrix(Eye eye)
	{
		if (!isLoaded()) return glm::mat4{ 1 };
		glm::mat4 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(Eye, glm::mat4&)>
			(fdm::getModFuncPointer(id, "getViewMatrix"));
		func(eye, result);
		return result;
	}
	inline fdm::m4::Mat5 getViewMatrix4D(Eye eye)
	{
		if (!isLoaded()) return fdm::m4::Mat5{ 1 };
		fdm::m4::Mat5 result{ 1 };
		static auto func = reinterpret_cast<fdm::m4::Mat5(__stdcall*)(Eye, fdm::m4::Mat5&)>
			(fdm::getModFuncPointer(id, "getViewMatrix4D"));
		func(eye, result);
		return result;
	}
	inline bool inVR()
	{
		if (!isLoaded()) return false;
		static auto func = reinterpret_cast<bool(__stdcall*)()>
			(fdm::getModFuncPointer(id, "inVR"));
		return func();
	}
	inline float getControllerYZAngleAdjustment()
	{
		if (!isLoaded()) return 0.0f;
		static auto func = reinterpret_cast<float(__stdcall*)()>
			(fdm::getModFuncPointer(id, "getControllerYZAngleAdjustment"));
		return func();
	}

	inline void renderPointer()
	{
		if (!isLoaded()) return;
		static auto func = reinterpret_cast<void(__stdcall*)()>
			(fdm::getModFuncPointer(id, "renderPointer"));
		return func();
	}

	/*
	If you are already calling StateTitleScreen::render() then you can just call VR::renderPointer() at the end of your code.
	If not, or if it is still broken, use this.
	Example:
	void StateTest::render(StateManager& s)
	{
		if (VR::inVR())
		{
			VR::vrStateRender(s,
				[](StateManager& s) // vrRender
				{
					// the surroundings rendering. use getViewMatrix() for 3D and getViewMatrix4D() for 4D.
				},
				[](StateManager& s) // uiRender
				{
					// your normal UI rendering code.
				}
			)
			return;
		}

		// your normal rendering code.
	}
	*/
	inline void vrStateRender(fdm::StateManager& s,
		const std::function<void(fdm::StateManager& s)>& vrRender,
		const std::function<void(fdm::StateManager& s)>& uiRender)
	{
		if (!inVR()) return;
		static auto func = reinterpret_cast<void(__stdcall*)(
			fdm::StateManager&,
			const std::function<void(fdm::StateManager & s)>&,
			const std::function<void(fdm::StateManager & s)>&)>
			(fdm::getModFuncPointer(id, "vrStateRender"));
		return func(s, vrRender, uiRender);
	}

	inline fdm::m4::Mat5 getEntityPlayerHandMat(const fdm::EntityPlayer* entity, Controller controller)
	{
		if (!isEntityPlayerInVR(entity)) return { 1 };
		fdm::m4::Mat5 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(const fdm::EntityPlayer*, Controller, fdm::m4::Mat5&)>
			(fdm::getModFuncPointer(id, "getEntityPlayerHandMat"));
		func(entity, controller, result);
		return result;
	}
	inline fdm::m4::Mat5 getEntityPlayerHeadMat(const fdm::EntityPlayer* entity)
	{
		if (!isEntityPlayerInVR(entity)) return { 1 };
		fdm::m4::Mat5 result{ 1 };
		static auto func = reinterpret_cast<void(__stdcall*)(const fdm::EntityPlayer*, fdm::m4::Mat5&)>
			(fdm::getModFuncPointer(id, "getEntityPlayerHeadMat"));
		func(entity, result);
		return result;
	}
	inline glm::vec4 getEntityPlayerHeadPos(const fdm::EntityPlayer* entity)
	{
		if (!isEntityPlayerInVR(entity)) return glm::vec4{ 0 };
		glm::vec4 result{ 0 };
		static auto func = reinterpret_cast<void(__stdcall*)(const fdm::EntityPlayer*, glm::vec4&)>
			(fdm::getModFuncPointer(id, "getEntityPlayerHeadPos"));
		func(entity, result);
		return result;
	}
	// height / Player::HEIGHT
	inline float getEntityPlayerHeightRatio(const fdm::EntityPlayer* entity)
	{
		if (!isEntityPlayerInVR(entity)) return 1.0f;
		static auto func = reinterpret_cast<float(__stdcall*)(const fdm::EntityPlayer*)>
			(fdm::getModFuncPointer(id, "getEntityPlayerHeightRatio"));
		return func(entity);
	}
}
