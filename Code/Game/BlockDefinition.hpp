#pragma once
#include <string>
#include <vector>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Core/Rgba8.hpp"


enum class BlockType : uint8_t 
{
	AIR, 
	WATER, 
	SAND,
	SNOW,
	ICE,
	DIRT,
	STONE,
	COAL,
	IRON,
	GOLD,
	DIAMOND,
	OBSIDIAN,
	LAVA,
	GLOWSTONE,
	COBBLESTONE,
	CHISELEDBRICK,
	GRASS,
	GRASSLIGHT,
	GRASSDARK,
	GRASSYELLOW,
	ACACIALOG,
	ACACIAPLANKS,
	ACACIALEAVES,
	CACTUSLOG,
	OAKLOG,
	OAKPLANKS,
	OAKLEAVES,
	BIRCHLOG,
	BIRCHPLANKS,
	BIRCHLEAVES,
	JUNGLELOG,
	JUNGLEPLANKS,
	JUNGLELEAVES,
	SPRUCELOG,
	SPRUCEPLANKS,
	SPRUCELEAVES,
	SPRUCELEAVESSNOW,

	NUM_BLOCK_TYPE
};


struct BlockDefinition
{
	std::string m_name = "";
	bool m_isVisible = false;
	bool m_isSolid = false;
	bool m_isOpaque = false;
	bool m_isLiquid = false;
	IntVec2 m_topSpriteCoords = IntVec2(7, 7);
	IntVec2 m_bottomSpriteCoords = IntVec2(7, 7);
	IntVec2 m_sideSpriteCoords = IntVec2(7, 7);
	int m_indoorLighting = 0;
	int m_outdoorLighting = 0;
	Rgba8   m_emissiveColor = Rgba8(0, 0, 0, 0);

	AABB2 m_topUV = AABB2::ZERO_TO_ONE;
	AABB2 m_botUV = AABB2::ZERO_TO_ONE;
	AABB2 m_sideUV = AABB2::ZERO_TO_ONE;

	static void InitializeBlockDefinitions(const char* path);

	static BlockDefinition const& GetDefinition(const std::string& name);

	static BlockDefinition const& GetDefinitionByIndex(size_t index);

	static uint8_t GetIndexByName(const std::string& name);

	static std::vector<BlockDefinition> s_blockDefs;

private:
	static BlockDefinition const& GetUnknown();

};