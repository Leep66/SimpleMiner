#pragma once
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include <cmath> 


// Load and Save
static constexpr uint8_t CHUNK_FILE_VERSION = 1;

// Chunk Coords
constexpr int CHUNK_BITS_X = 4;
constexpr int CHUNK_BITS_Y = 4;
constexpr int CHUNK_BITS_Z = 7;

constexpr int CHUNK_SIZE_X = 1 << CHUNK_BITS_X;   // 16
constexpr int CHUNK_SIZE_Y = 1 << CHUNK_BITS_Y;   // 16
constexpr int CHUNK_SIZE_Z = 1 << CHUNK_BITS_Z;   // 128

constexpr int CHUNK_MAX_X = CHUNK_SIZE_X - 1;
constexpr int CHUNK_MAX_Y = CHUNK_SIZE_Y - 1;
constexpr int CHUNK_MAX_Z = CHUNK_SIZE_Z - 1;

constexpr int CHUNK_MASK_X = CHUNK_MAX_X;
constexpr int CHUNK_MASK_Y = CHUNK_MAX_Y << CHUNK_BITS_X;
constexpr int CHUNK_MASK_Z = CHUNK_MAX_Z << (CHUNK_BITS_X + CHUNK_BITS_Y);

constexpr int BLOCKS_PER_CHUNK = CHUNK_SIZE_X * CHUNK_SIZE_Y * CHUNK_SIZE_Z;

constexpr int SHIFT_Y = CHUNK_BITS_X;
constexpr int SHIFT_Z = CHUNK_BITS_X + CHUNK_BITS_Y;

constexpr int STEP_X = 1;
constexpr int STEP_Y = 1 << SHIFT_Y;
constexpr int STEP_Z = 1 << SHIFT_Z;

constexpr int CHUNK_ACTIVATION_RANGE = 280;
constexpr int CHUNK_DEACTIVATION_RANGE = CHUNK_ACTIVATION_RANGE + CHUNK_SIZE_X + CHUNK_SIZE_Y;
constexpr int CHUNK_ACTIVATION_RADIUS_X = 1 + (CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_X);
constexpr int CHUNK_ACTIVATION_RADIUS_Y = 1 + (CHUNK_ACTIVATION_RANGE / CHUNK_SIZE_Y);
constexpr int MAX_ACTIVE_CHUNKS = (2 * CHUNK_ACTIVATION_RADIUS_X) * (2 * CHUNK_ACTIVATION_RADIUS_Y);

inline int LocalCoordsToIndex(int x, int y, int z)
{
	return (x & CHUNK_MAX_X) | ((y & CHUNK_MAX_Y) << SHIFT_Y) | ((z & CHUNK_MAX_Z) << SHIFT_Z);
}

inline int LocalCoordsToIndex(const IntVec3& p)
{
	return LocalCoordsToIndex(p.x, p.y, p.z);
}

inline int IndexToLocalX(int idx)
{
	return idx & CHUNK_MASK_X;
}

inline int IndexToLocalY(int idx)
{
	return (idx & CHUNK_MASK_Y) >> SHIFT_Y;
}

inline int IndexToLocalZ(int idx)
{
	return (idx & CHUNK_MASK_Z) >> SHIFT_Z;
}

inline IntVec3 IndexToLocalCoords(int idx)
{
	return IntVec3(IndexToLocalX(idx), IndexToLocalY(idx), IndexToLocalZ(idx));
}

inline IntVec2 GetChunkCoords(const IntVec3& g)
{
	const int cx = (int)std::floor((double)g.x / (double)CHUNK_SIZE_X);
	const int cy = (int)std::floor((double)g.y / (double)CHUNK_SIZE_Y);
	return IntVec2(cx, cy);
}

inline IntVec2 GetChunkCenter(const IntVec2& c)
{
	return IntVec2(c.x * CHUNK_SIZE_X + (CHUNK_SIZE_X >> 1), c.y * CHUNK_SIZE_Y + (CHUNK_SIZE_Y >> 1));
}

inline IntVec3 GlobalCoordsToLocalCoords(const IntVec3& g)
{
	const int cx = (int)std::floor((double)g.x / (double)CHUNK_SIZE_X);
	const int cy = (int)std::floor((double)g.y / (double)CHUNK_SIZE_Y);
	const int cz = (int)std::floor((double)g.z / (double)CHUNK_SIZE_Z);
	return IntVec3(g.x - cx * CHUNK_SIZE_X, g.y - cy * CHUNK_SIZE_Y, g.z - cz * CHUNK_SIZE_Z);
}

inline int GlobalCoordsToIndex(const IntVec3& g)
{
	const IntVec3 local = GlobalCoordsToLocalCoords(g);
	return LocalCoordsToIndex(local);
}

inline int GlobalCoordsToIndex(int x, int y, int z)
{
	return GlobalCoordsToIndex(IntVec3(x, y, z));
}

inline IntVec3 GetGlobalCoords(const IntVec2& c, const IntVec3& local)
{
	return IntVec3(c.x * CHUNK_SIZE_X + local.x, c.y * CHUNK_SIZE_Y + local.y, local.z);
}

inline IntVec3 GetGlobalCoords(const IntVec2& c, int index)
{
	const IntVec3 local = IndexToLocalCoords(index);
	return GetGlobalCoords(c, local);
}

inline IntVec3 GetGlobalCoords(const Vec3& p)
{
	return IntVec3((int)std::floor(p.x), (int)std::floor(p.y), (int)std::floor(p.z));
}



// Static Variables
constexpr unsigned int GAME_SEED = 0u;

constexpr float DEFAULT_OCTAVE_PERSISTANCE = 0.5f;
constexpr float DEFAULT_NOISE_OCTAVE_SCALE = 2.0f;

static constexpr float DENSITY_NOISE_SCALE = 64.f;
static constexpr unsigned DENSITY_NOISE_OCTAVES = 3u;
static constexpr float DENSITY_BIAS_PER_Z = 2.f / (float)CHUNK_SIZE_Z;

constexpr float DEFAULT_TERRAIN_HEIGHT = 64.f;
constexpr float RIVER_DEPTH = 8.0f;
constexpr float TERRAIN_NOISE_SCALE = 200.0f;
constexpr unsigned int TERRAIN_NOISE_OCTAVES = 5u;

constexpr float HUMIDITY_NOISE_SCALE = 800.0f;
constexpr unsigned int HUMIDITY_NOISE_OCTAVES = 4u;

constexpr float TEMPERATURE_RAW_NOISE_SCALE = 0.0075f;
constexpr float TEMPERATURE_NOISE_SCALE = 400.0f;
constexpr unsigned int TEMPERATURE_NOISE_OCTAVES = 4u;

constexpr float HILLINESS_NOISE_SCALE = 250.0f;
constexpr unsigned int HILLINESS_NOISE_OCTAVES = 4u;

constexpr float OCEAN_START_THRESHOLD = 0.0f;
constexpr float OCEAN_END_THRESHOLD = 0.5f;
constexpr float OCEAN_DEPTH = 20.0f;

constexpr float OCEANESS_NOISE_SCALE = 960.f;
constexpr unsigned int OCEANESS_NOISE_OCTAVES = 3u;

constexpr float CONTINENTALNESS_NOISE_SCALE = 720.f;
constexpr unsigned int CONTINENTALNESS_NOISE_OCTAVES = 3u;



constexpr float EROSION_NOISE_SCALE = 150.f;
constexpr unsigned int EROSION_NOISE_OCTAVES = 6u;

constexpr float PEAKS_VALLEYS_NOISE_SCALE = 50.f;
constexpr unsigned int PEAKS_VALLEYS_NOISE_OCTAVES = 6u;

constexpr int   MIN_DIRT_OFFSET_Z = 3;
constexpr int   MAX_DIRT_OFFSET_Z = 4;
constexpr float MIN_SAND_HUMIDITY = 0.4f;
constexpr float MAX_SAND_HUMIDITY = 0.7f;
constexpr int   SEA_LEVEL_Z = CHUNK_SIZE_Z / 2 - 1;

constexpr float ICE_TEMPERATURE_MAX = 0.37f;
constexpr float ICE_TEMPERATURE_MIN = 0.0f;
constexpr float ICE_DEPTH_MIN = 0.0f;
constexpr float ICE_DEPTH_MAX = 8.0f;

constexpr float MIN_SAND_DEPTH_HUMIDITY = 0.4f;
constexpr float MAX_SAND_DEPTH_HUMIDITY = 0.0f;
constexpr float SAND_DEPTH_MIN = 0.0f;
constexpr float SAND_DEPTH_MAX = 6.0f;

constexpr float COAL_CHANCE = 0.05f;
constexpr float IRON_CHANCE = 0.02f;
constexpr float GOLD_CHANCE = 0.005f;
constexpr float DIAMOND_CHANCE = 0.0001f;

constexpr int   OBSIDIAN_Z = 1;
constexpr int   LAVA_Z = 0;


static constexpr int TREE_RADIUS_MAX = 2;
static constexpr int NOISE_BORDER = TREE_RADIUS_MAX + 1;
static const unsigned TREE_SEED = GAME_SEED + 100u;



static constexpr float ICE_TEMP_FREEZE_TOP = 0.38f;
static constexpr float ICE_TEMP_SHORE = 0.30f;

static inline double SimpleMinerToVanilla(double y) 
{
	return -64.0 + (383.0 / 127.0) * y;
}

static inline double VanillaToSimpleMiner(double y) 
{
	return (127.0 / 383.0) * (y + 64.0);
}

static inline IntVec2 MapVanillaToSimpleMiner(IntVec2 v)
{
	float minY = RangeMapClamped((float)v.x, -64.0, 319.0, 0.0, 127.0);
	float maxY = RangeMapClamped((float)v.y, -64.0, 319.0, 0.0, 127.0);
	return IntVec2((int)minY, (int)maxY);
}

static inline IntVec2 MapSimpleMinerToVanilla(IntVec2 o)
{
	float minY = RangeMapClamped((float)o.x, 0.0, 127.0, -64.0, 319.0);
	float maxY = RangeMapClamped((float)o.y, 0.0, 127.0, -64.0, 319.0);
	return IntVec2((int)minY, (int)maxY);
}

static inline float DensityAt(int gx, int gy, int gz, float baseH, unsigned seed)
{
	const float n3d = Compute3dPerlinNoise(
		(float)gx, (float)gy, (float)gz,
		DENSITY_NOISE_SCALE,
		DENSITY_NOISE_OCTAVES,
		DEFAULT_OCTAVE_PERSISTANCE,
		DEFAULT_NOISE_OCTAVE_SCALE,
		true, seed);

	const float bias = DENSITY_BIAS_PER_Z * ((float)gz - baseH);
	return n3d + bias; 
}