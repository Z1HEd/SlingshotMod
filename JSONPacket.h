#pragma once

namespace JSONPacket
{
	inline constexpr const char* C_SLINGSHOT_SHOOT = "zihed.slingshot.client.shoot";
	inline constexpr const char* C_SLINGSHOT_CANCEL_DRAWING = "zihed.slingshot.client.cancel_drawing";
	inline constexpr const char* C_SLINGSHOT_UPDATE = "zihed.slingshot.client.update";
	inline constexpr const char* C_SLINGSHOT_START_DRAWING = "zihed.slingshot.client.start_drawing";
	inline constexpr const char* S_SLINGSHOT_START_DRAWING = "zihed.slingshot.server.start_drawing";
	inline constexpr const char* S_SLINGSHOT_STOP_DRAWING = "zihed.slingshot.server.stop_drawing";
	inline constexpr const char* S_SLINGSHOT_CANCEL_DRAWING = "zihed.slingshot.server.cancel_drawing";
}
