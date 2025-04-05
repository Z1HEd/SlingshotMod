#pragma once
#include <4dm.h>

using namespace fdm;

class FullPlayerInventory {
public:
	inline static bool addItem(Player* player, std::unique_ptr<Item>& item) {
		bool combined=combineItem(player, item);
		if (!item)
			return combined;
		return placeItem(player, item) || combined;
	}
	inline static bool combineItem(Player* player, std::unique_ptr<Item>& item) {
		bool combined = player->hotbar.combineItem(item);
		if (!item) return combined;

		combined = player->equipment.combineItem(item) || combined;
		if (!item) return combined;

		return player->inventory.combineItem(item) || combined;
	}
	inline static bool placeItem(Player* player, std::unique_ptr<Item>& item) {
		bool placed = player->hotbar.placeItem(item);
		if (!item) return placed;

		placed = player->equipment.placeItem(item) || placed;
		if (!item) return placed;

		return player->inventory.placeItem(item) || placed;
	}
	inline static int getSlotIndex(Player* player, const glm::ivec2& cursorPos) {
		int result = player->inventory.getSlotIndex(cursorPos);
		if (result != -1) return result;

		result = player->hotbar.getSlotIndex(cursorPos);
		if (result != -1) return result;

		return player->equipment.getSlotIndex(cursorPos);
	}
	inline static uint32_t getSlotCount(Player* player) {
		return player->inventory.getSlotCount() + player->hotbar.getSlotCount() + player->equipment.getSlotCount();
	}
	inline static std::unique_ptr<Item>& getSlot(Player* player, int index) {
		if (index < player->hotbar.getSlotCount()) return player->hotbar.getSlot(index);
		index -= player->hotbar.getSlotCount();
		if (index <player->inventory.getSlotCount()) return player->inventory.getSlot(index);
		index -= player->inventory.getSlotCount();
		return player->equipment.getSlot(index);
	}
};