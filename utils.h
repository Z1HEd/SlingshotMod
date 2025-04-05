#pragma once

#include <glm/glm.hpp>
#include <math.h>

namespace utils {
	// framerate independent a=lerp(a,b,t)
	inline float deltaRatio(float ratio, double dt, double targetDelta)
	{
		const double rDelta = dt / (1.0 / (1.0 / targetDelta));
		const double s = 1.0 - ratio;

		return (float)(1.0 - pow(s, rDelta));
	}
	inline float deltaRatio(float ratio, double dt)
	{
		return deltaRatio(ratio, dt, 1.0 / 100.0);
	}
	inline float lerp(float a, float b, float ratio, bool clampRatio = true)
	{
		if (clampRatio)
			ratio = glm::clamp(ratio, 0.f, 1.f);
		return a + (b - a) * ratio;
	}
	inline glm::vec4 lerp(const glm::vec4& a, const glm::vec4& b, float ratio, bool clampRatio = true)
	{
		return glm::vec4{ lerp(a.x, b.x, ratio, clampRatio),lerp(a.y, b.y, ratio, clampRatio),lerp(a.z, b.z, ratio, clampRatio),lerp(a.w, b.w, ratio, clampRatio) };
	}
	inline float ilerp(float a, float b, float ratio, double dt, bool clampRatio = true)
	{
		return lerp(a, b, deltaRatio(ratio, dt), clampRatio);
	}
	inline glm::vec4 ilerp(const glm::vec4& a, const glm::vec4& b, float ratio, double dt, bool clampRatio = true)
	{
		return lerp(a, b, deltaRatio(ratio, dt), clampRatio);
	}
	inline float easeInOutQuad(float x)
	{
		return x < 0.5f ? 2.0f * x * x : 1.0f - powf(-2.0f * x + 2.0f, 2.0f) / 2.0f;
	}
}