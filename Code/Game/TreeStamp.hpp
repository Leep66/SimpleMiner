#pragma once
#include "Game/Chunk.hpp"

static const StampBlock OAK_SMALL_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},
	{0,0,3,LEAVES},{1,0,3,LEAVES},{-1,0,3,LEAVES},{0,1,3,LEAVES},{0,-1,3,LEAVES},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{1,1,4,LEAVES},{1,-1,4,LEAVES},{-1,1,4,LEAVES},{-1,-1,4,LEAVES},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES}
};
static const TreeStamp OAK_SMALL{ OAK_SMALL_DATA, (int)(sizeof(OAK_SMALL_DATA) / sizeof(StampBlock)) };

static const StampBlock OAK_MEDIUM_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{1,1,4,LEAVES},{1,-1,4,LEAVES},{-1,1,4,LEAVES},{-1,-1,4,LEAVES},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{2,0,5,LEAVES},{-2,0,5,LEAVES},{0,2,5,LEAVES},{0,-2,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{2,1,5,LEAVES},{2,-1,5,LEAVES},{-2,1,5,LEAVES},{-2,-1,5,LEAVES},
	{1,2,5,LEAVES},{-1,2,5,LEAVES},{1,-2,5,LEAVES},{-1,-2,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES},
	{1,1,7,LEAVES},{1,-1,7,LEAVES},{-1,1,7,LEAVES},{-1,-1,7,LEAVES}
};
static const TreeStamp OAK_MEDIUM{ OAK_MEDIUM_DATA, (int)(sizeof(OAK_MEDIUM_DATA) / sizeof(StampBlock)) };

static const StampBlock OAK_LARGE_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},{0,0,8,LOG},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{2,0,5,LEAVES},{-2,0,5,LEAVES},{0,2,5,LEAVES},{0,-2,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{2,1,5,LEAVES},{2,-1,5,LEAVES},{-2,1,5,LEAVES},{-2,-1,5,LEAVES},
	{1,2,5,LEAVES},{-1,2,5,LEAVES},{1,-2,5,LEAVES},{-1,-2,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{2,0,6,LEAVES},{-2,0,6,LEAVES},{0,2,6,LEAVES},{0,-2,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{2,1,6,LEAVES},{2,-1,6,LEAVES},{-2,1,6,LEAVES},{-2,-1,6,LEAVES},
	{1,2,6,LEAVES},{-1,2,6,LEAVES},{1,-2,6,LEAVES},{-1,-2,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES},
	{2,0,7,LEAVES},{-2,0,7,LEAVES},{0,2,7,LEAVES},{0,-2,7,LEAVES},
	{1,1,7,LEAVES},{1,-1,7,LEAVES},{-1,1,7,LEAVES},{-1,-1,7,LEAVES},
	{2,1,7,LEAVES},{2,-1,7,LEAVES},{-2,1,7,LEAVES},{-2,-1,7,LEAVES},
	{1,2,7,LEAVES},{-1,2,7,LEAVES},{1,-2,7,LEAVES},{-1,-2,7,LEAVES},
	{0,0,8,LEAVES},{1,0,8,LEAVES},{-1,0,8,LEAVES},{0,1,8,LEAVES},{0,-1,8,LEAVES},
	{1,1,8,LEAVES},{1,-1,8,LEAVES},{-1,1,8,LEAVES},{-1,-1,8,LEAVES},
	{0,0,9,LEAVES},{1,0,9,LEAVES},{-1,0,9,LEAVES},{0,1,9,LEAVES},{0,-1,9,LEAVES},
	{1,1,9,LEAVES},{1,-1,9,LEAVES},{-1,1,9,LEAVES},{-1,-1,9,LEAVES}
};
static const TreeStamp OAK_LARGE{ OAK_LARGE_DATA, (int)(sizeof(OAK_LARGE_DATA) / sizeof(StampBlock)) };

static const StampBlock SPRUCE_SMALL_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},
	{0,0,3,LEAVES},{1,0,3,LEAVES},{-1,0,3,LEAVES},{0,1,3,LEAVES},{0,-1,3,LEAVES},
	{1,1,3,LEAVES},{1,-1,3,LEAVES},{-1,1,3,LEAVES},{-1,-1,3,LEAVES},
	{2,0,3,LEAVES},{-2,0,3,LEAVES},{0,2,3,LEAVES},{0,-2,3,LEAVES},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{1,1,4,LEAVES},{1,-1,4,LEAVES},{-1,1,4,LEAVES},{-1,-1,4,LEAVES},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES}
};
static const TreeStamp SPRUCE_SMALL{ SPRUCE_SMALL_DATA, (int)(sizeof(SPRUCE_SMALL_DATA) / sizeof(StampBlock)) };

static const StampBlock SPRUCE_MEDIUM_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{2,0,4,LEAVES},{-2,0,4,LEAVES},{0,2,4,LEAVES},{0,-2,4,LEAVES},
	{1,1,4,LEAVES},{1,-1,4,LEAVES},{-1,1,4,LEAVES},{-1,-1,4,LEAVES},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{2,0,5,LEAVES},{-2,0,5,LEAVES},{0,2,5,LEAVES},{0,-2,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES}
};
static const TreeStamp SPRUCE_MEDIUM{ SPRUCE_MEDIUM_DATA, (int)(sizeof(SPRUCE_MEDIUM_DATA) / sizeof(StampBlock)) };

static const StampBlock SPRUCE_LARGE_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},
	{0,0,6,LOG},{0,0,7,LOG},{0,0,8,LOG},{0,0,9,LOG},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{2,0,5,LEAVES},{-2,0,5,LEAVES},{0,2,5,LEAVES},{0,-2,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{2,0,6,LEAVES},{-2,0,6,LEAVES},{0,2,6,LEAVES},{0,-2,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES},
	{2,0,7,LEAVES},{-2,0,7,LEAVES},{0,2,7,LEAVES},{0,-2,7,LEAVES},
	{1,1,7,LEAVES},{1,-1,7,LEAVES},{-1,1,7,LEAVES},{-1,-1,7,LEAVES},
	{0,0,8,LEAVES},{1,0,8,LEAVES},{-1,0,8,LEAVES},{0,1,8,LEAVES},{0,-1,8,LEAVES},
	{1,1,8,LEAVES},{1,-1,8,LEAVES},{-1,1,8,LEAVES},{-1,-1,8,LEAVES},
	{0,0,9,LEAVES},{1,0,9,LEAVES},{-1,0,9,LEAVES},{0,1,9,LEAVES},{0,-1,9,LEAVES}
};
static const TreeStamp SPRUCE_LARGE{ SPRUCE_LARGE_DATA, (int)(sizeof(SPRUCE_LARGE_DATA) / sizeof(StampBlock)) };

static const StampBlock BIRCH_SMALL_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES}
};
static const TreeStamp BIRCH_SMALL{ BIRCH_SMALL_DATA, (int)(sizeof(BIRCH_SMALL_DATA) / sizeof(StampBlock)) };

static const StampBlock BIRCH_MEDIUM_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES},
	{1,1,7,LEAVES},{1,-1,7,LEAVES},{-1,1,7,LEAVES},{-1,-1,7,LEAVES}
};
static const TreeStamp BIRCH_MEDIUM{ BIRCH_MEDIUM_DATA, (int)(sizeof(BIRCH_MEDIUM_DATA) / sizeof(StampBlock)) };

static const StampBlock BIRCH_LARGE_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},{0,0,8,LOG},{0,0,9,LOG},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES},
	{0,0,8,LEAVES},{1,0,8,LEAVES},{-1,0,8,LEAVES},{0,1,8,LEAVES},{0,-1,8,LEAVES},
	{1,1,8,LEAVES},{1,-1,8,LEAVES},{-1,1,8,LEAVES},{-1,-1,8,LEAVES},
	{0,0,9,LEAVES},{1,0,9,LEAVES},{-1,0,9,LEAVES},{0,1,9,LEAVES},{0,-1,9,LEAVES}
};
static const TreeStamp BIRCH_LARGE{ BIRCH_LARGE_DATA, (int)(sizeof(BIRCH_LARGE_DATA) / sizeof(StampBlock)) };

static const StampBlock ACACIA_SMALL_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},
	{0,0,4,LEAVES},{1,0,4,LEAVES},{-1,0,4,LEAVES},{0,1,4,LEAVES},{0,-1,4,LEAVES},
	{1,1,4,LEAVES},{1,-1,4,LEAVES},{-1,1,4,LEAVES},{-1,-1,4,LEAVES},
	{2,0,4,LEAVES},{-2,0,4,LEAVES},{0,2,4,LEAVES},{0,-2,4,LEAVES}
};
static const TreeStamp ACACIA_SMALL{ ACACIA_SMALL_DATA, (int)(sizeof(ACACIA_SMALL_DATA) / sizeof(StampBlock)) };

static const StampBlock ACACIA_MEDIUM_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},
	{0,0,5,LEAVES},{1,0,5,LEAVES},{-1,0,5,LEAVES},{0,1,5,LEAVES},{0,-1,5,LEAVES},
	{1,1,5,LEAVES},{1,-1,5,LEAVES},{-1,1,5,LEAVES},{-1,-1,5,LEAVES},
	{2,0,5,LEAVES},{-2,0,5,LEAVES},{0,2,5,LEAVES},{0,-2,5,LEAVES},
	{2,1,5,LEAVES},{2,-1,5,LEAVES},{-2,1,5,LEAVES},{-2,-1,5,LEAVES},
	{1,2,5,LEAVES},{-1,2,5,LEAVES},{1,-2,5,LEAVES},{-1,-2,5,LEAVES}
};
static const TreeStamp ACACIA_MEDIUM{ ACACIA_MEDIUM_DATA, (int)(sizeof(ACACIA_MEDIUM_DATA) / sizeof(StampBlock)) };

static const StampBlock ACACIA_LARGE_DATA[] = {
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{2,0,6,LEAVES},{-2,0,6,LEAVES},{0,2,6,LEAVES},{0,-2,6,LEAVES},
	{2,1,6,LEAVES},{2,-1,6,LEAVES},{-2,1,6,LEAVES},{-2,-1,6,LEAVES},
	{1,2,6,LEAVES},{-1,2,6,LEAVES},{1,-2,6,LEAVES},{-1,-2,6,LEAVES},
	{3,0,6,LEAVES},{-3,0,6,LEAVES},{0,3,6,LEAVES},{0,-3,6,LEAVES},
	{2,2,6,LEAVES},{2,-2,6,LEAVES},{-2,2,6,LEAVES},{-2,-2,6,LEAVES}
};
static const TreeStamp ACACIA_LARGE{ ACACIA_LARGE_DATA, (int)(sizeof(ACACIA_LARGE_DATA) / sizeof(StampBlock)) };

static const StampBlock JUNGLE_SMALL_DATA[] = 
{
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},
	{0,0,6,LEAVES},{1,0,6,LEAVES},{-1,0,6,LEAVES},{0,1,6,LEAVES},{0,-1,6,LEAVES},
	{2,0,6,LEAVES},{-2,0,6,LEAVES},{0,2,6,LEAVES},{0,-2,6,LEAVES},
	{1,1,6,LEAVES},{1,-1,6,LEAVES},{-1,1,6,LEAVES},{-1,-1,6,LEAVES},
	{0,0,7,LEAVES},{1,0,7,LEAVES},{-1,0,7,LEAVES},{0,1,7,LEAVES},{0,-1,7,LEAVES}
};
static const TreeStamp JUNGLE_SMALL{ JUNGLE_SMALL_DATA, (int)(sizeof(JUNGLE_SMALL_DATA) / sizeof(StampBlock)) };

static const StampBlock JUNGLE_MEDIUM_DATA[] = 
{
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},{0,0,8,LOG},{0,0,9,LOG},
	{0,0,8,LEAVES},{1,0,8,LEAVES},{-1,0,8,LEAVES},{0,1,8,LEAVES},{0,-1,8,LEAVES},
	{2,0,8,LEAVES},{-2,0,8,LEAVES},{0,2,8,LEAVES},{0,-2,8,LEAVES},
	{1,1,8,LEAVES},{1,-1,8,LEAVES},{-1,1,8,LEAVES},{-1,-1,8,LEAVES},
	{0,0,9,LEAVES},{1,0,9,LEAVES},{-1,0,9,LEAVES},{0,1,9,LEAVES},{0,-1,9,LEAVES}
};
static const TreeStamp JUNGLE_MEDIUM{ JUNGLE_MEDIUM_DATA, (int)(sizeof(JUNGLE_MEDIUM_DATA) / sizeof(StampBlock)) };

static const StampBlock JUNGLE_LARGE_DATA[] = 
{
	{0,0,1,LOG},{0,0,2,LOG},{0,0,3,LOG},{0,0,4,LOG},{0,0,5,LOG},{0,0,6,LOG},{0,0,7,LOG},{0,0,8,LOG},{0,0,9,LOG},{0,0,10,LOG},{0,0,11,LOG},
	{0,0,9,LEAVES},{1,0,9,LEAVES},{-1,0,9,LEAVES},{0,1,9,LEAVES},{0,-1,9,LEAVES},
	{2,0,9,LEAVES},{-2,0,9,LEAVES},{0,2,9,LEAVES},{0,-2,9,LEAVES},
	{1,1,9,LEAVES},{1,-1,9,LEAVES},{-1,1,9,LEAVES},{-1,-1,9,LEAVES},
	{0,0,10,LEAVES},{1,0,10,LEAVES},{-1,0,10,LEAVES},{0,1,10,LEAVES},{0,-1,10,LEAVES},
	{2,0,10,LEAVES},{-2,0,10,LEAVES},{0,2,10,LEAVES},{0,-2,10,LEAVES},
	{1,1,10,LEAVES},{1,-1,10,LEAVES},{-1,1,10,LEAVES},{-1,-1,10,LEAVES},
	{0,0,11,LEAVES},{1,0,11,LEAVES},{-1,0,11,LEAVES},{0,1,11,LEAVES},{0,-1,11,LEAVES}
};
static const TreeStamp JUNGLE_LARGE{ JUNGLE_LARGE_DATA, (int)(sizeof(JUNGLE_LARGE_DATA) / sizeof(StampBlock)) };

static const StampBlock CACTUS_SMALL_DATA[] = 
{
	{0,0,1,LOG}, {0,0,2,LOG}
};
static const TreeStamp CACTUS_SMALL{ CACTUS_SMALL_DATA, (int)(sizeof(CACTUS_SMALL_DATA) / sizeof(StampBlock)) };


static const StampBlock CACTUS_MEDIUM_DATA[] = {
	{0,0,1,LOG}, {0,0,2,LOG}, {0,0,3,LOG},

	{1,0,2,LOG}
};
static const TreeStamp CACTUS_MEDIUM{ CACTUS_MEDIUM_DATA, (int)(sizeof(CACTUS_MEDIUM_DATA) / sizeof(StampBlock)) };


static const StampBlock CACTUS_LARGE_DATA[] = {
	{0,0,1,LOG}, {0,0,2,LOG}, {0,0,3,LOG}, {0,0,4,LOG}, {0,0,5,LOG},

	{-1,0,3,LOG},

	{1,0,4,LOG}, {2,0,4,LOG}
};
static const TreeStamp CACTUS_LARGE{ CACTUS_LARGE_DATA, (int)(sizeof(CACTUS_LARGE_DATA) / sizeof(StampBlock)) };


struct StampSet3 { TreeStamp small, medium, large; };

static inline StampSet3 ChooseStampSet(TreeSpecies s)
{
	switch (s) {
	case TreeSpecies::OAK:    return { OAK_SMALL,    OAK_MEDIUM,    OAK_LARGE };
	case TreeSpecies::SPRUCE: return { SPRUCE_SMALL, SPRUCE_MEDIUM, SPRUCE_LARGE };
	case TreeSpecies::ACACIA: return { ACACIA_SMALL, ACACIA_MEDIUM, ACACIA_LARGE };
	case TreeSpecies::BIRCH:  return { BIRCH_SMALL,  BIRCH_MEDIUM,  BIRCH_LARGE };
	case TreeSpecies::JUNGLE: return { JUNGLE_SMALL, JUNGLE_MEDIUM, JUNGLE_LARGE };
	case TreeSpecies::CACTUS: return { CACTUS_SMALL, CACTUS_MEDIUM, CACTUS_LARGE };
	default:                 return { OAK_SMALL,    OAK_MEDIUM,    OAK_LARGE };
	}
}