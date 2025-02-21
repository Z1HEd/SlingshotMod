#define DEBUG_CONSOLE // Uncomment this if you want a debug console to start. You can use the Console class to print. You can use Console::inStrings to get input.

#include <4dm.h>

using namespace fdm;

constexpr std::string NAME = "Slingshot";

// Initialize the DLLMain
initDLL
$hook(void, StateIntro, init, StateManager& s)
{
	original(self, s);

	// initialize opengl stuff
	glewExperimental = true;
	glewInit();
	glfwInit();
}



// custom action
$hook(bool, ItemMaterial, action, World* world, Player* player, int action)
{
	original(self, world, player, action);

	if (self->name != NAME) return false; // if not the custom item, return.
	if (!action) return false; // if not right click, return.
	if (self->count <= 0) return false; // if no item, return.

	Console::printLine("Right clicking");

	return true; // action happend
}

// entity (on ground or in hand)
$hook(void, ItemMaterial, renderEntity, const m4::Mat5& mat, bool inHand, const glm::vec4& lightDir)
{
	if (self->name != NAME)
		return original(self, mat, inHand, lightDir);

	// code for rendering the model of your item
}

// item slot
$hook(void, ItemMaterial, render, const glm::ivec2& pos)
{
	if (self->name != NAME)
		return original(self, pos);

	TexRenderer& tr = *ItemTool::tr; // or TexRenderer& tr = ItemTool::tr; after 0.3
	const Tex2D* ogTex = tr.texture; // remember the original texture
	tr.texture = ResourceManager::get("Res/SlingshotItem.png", true); // set to custom texture
	// this is for a single item texture, not for a texture set like the ogTex
	tr.setClip(0, 0, 18, 18); // no offset, size=texture size
	tr.setPos(pos.x, pos.y, 70, 72); // the render position
	tr.render();
	tr.texture = ogTex; // return to the original texture
}

// add recipe
$hookStatic(void, CraftingMenu, loadRecipes)
{
	static bool recipesLoaded = false;

	if (recipesLoaded) return;

	recipesLoaded = true;

	original();

	CraftingMenu::recipes->push_back(
		nlohmann::json{
		{"recipe", {{{"name", "Stick"}, {"count", 2}},{{"name", "Hypersilk"}, {"count", 3}}}},
		{"result", {{"name", NAME}, {"count", 1}}}
		}
	);
}


void initItemNAME()
{
	// add the custom item
	(*Item::blueprints)[NAME] =
	{
	{ "type", "tool" }, // based on ItemMaterial
	{ "baseAttributes", nlohmann::json::object() } // no attributes
	};
}

$hook(void, StateIntro, init, StateManager& s)
{
	// ...
	original(self, s);
	// ...
	initItemNAME();
	// ...
}