#include "Game/Chunk.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Renderer/VertexBuffer.hpp"
#include "Engine/Renderer/IndexBuffer.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "Game/ChunkCoords.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/BlockIterator.hpp"
#include "Game/World.hpp"
#include "Game/Game.hpp"
#include "Game/BiomeDefinitions.hpp"
#include "Game/TreeStamp.hpp"
#include "Engine/Math/MathUtils.hpp"


extern Renderer* g_theRenderer;

static const Rgba8 TOP_BOT_COLOR(255, 255, 255, 255);
static const Rgba8 EW_COLOR(230, 230, 230, 255);
static const Rgba8 NS_COLOR(200, 200, 200, 255);

uint8_t LightLevelToByte(uint8_t level) 
{
	constexpr float s = 255.0f / 15.0f;
	int v = (int)(level * s + 0.5f);
	return (uint8_t)GetClamped((float)v, 0.f, 255.f); 
}



static Vec3 ColorToVec3(const Rgba8& c)
{
	return Vec3(
		(float)c.r / 255.0f,
		(float)c.g / 255.0f,
		(float)c.b / 255.0f
	);
}

static Rgba8 MakeFaceLightColor(
	const BlockIterator& it,
	const BlockIterator& neighbor,
	const Rgba8& dirShade)
{
	const Block* srcBlock = nullptr;

	if (neighbor.m_chunk && neighbor.m_blockIdx >= 0)
	{
		srcBlock = &neighbor.m_chunk->GetBlockRef(neighbor.m_blockIdx);
	}
	else
	{
		srcBlock = &it.m_chunk->GetBlockRef(it.m_blockIdx);
	}

	Vec3 outRGB = ColorToVec3(srcBlock->m_outdoorLightRGB);
	Vec3 inRGB = ColorToVec3(srcBlock->m_indoorLightRGB);

	Vec3 totalRGB = outRGB + inRGB;

	float dirFactor = (float)dirShade.r / 255.0f;
	totalRGB *= dirFactor;

	float outL = outRGB.x + outRGB.y + outRGB.z;
	float inL = inRGB.x + inRGB.y + inRGB.z;

	float skyFactor = 0.0f;
	if ((outL + inL) > 0.0001f)
	{
		skyFactor = outL / (outL + inL);
	}

	Rgba8 result;

	result.r = (uint8_t)(GetClamped(totalRGB.x, 0.0f, 1.0f) * 255);
	result.g = (uint8_t)(GetClamped(totalRGB.y, 0.0f, 1.0f) * 255);
	result.b = (uint8_t)(GetClamped(totalRGB.z, 0.0f, 1.0f) * 255);

	result.a = (uint8_t)(skyFactor * 255.0f); 

	return result;
}






static inline float To01(float nMinus1To1) { return 0.5f * (nMinus1To1 + 1.0f); }

static inline bool InLocalBounds(int lx, int ly, int lz)
{
	return (unsigned)lx < (unsigned)CHUNK_SIZE_X
		&& (unsigned)ly < (unsigned)CHUNK_SIZE_Y
		&& (unsigned)lz < (unsigned)CHUNK_SIZE_Z;
}


static inline float saturatef(float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); }

static inline float TreeProbForParams(Biome biome)
{
	switch (biome)
	{
	case Biome::Ocean:
	case Biome::DeepOcean:
	case Biome::FrozenOcean:
		return 0.3f;
	case Biome::Beach:
		return 0.5f;
	case Biome::SnowyBeach:
		return 0.3f;
	case Biome::Desert:
		return 0.6f;
	case Biome::Savanna:
		return 0.5f;
	case Biome::Plains:
		return 0.7f;
	case Biome::SnowyPlains:
		return 0.7f;
	case Biome::Forest:
		return 1.0f;
	case Biome::Jungle:
		return 1.0f;
	case Biome::Taiga:
		return 1.0f;
	case Biome::SnowyTaiga:
		return 0.7f;
	case Biome::StonyPeaks:
		return 0.0f;
	case Biome::SnowyPeaks:
		return 0.7f;
	case Biome::Unknown:
		return 0.2f;
	default:
		return 0.2f;
	}
}


static inline int ChooseSizeTier(int gx, int gy, float humidity01) 
{
	const float n = To01(Compute2dPerlinNoise(
		(float)gx, (float)gy, 48.f, 2u,
		DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
		true, TREE_SEED + 33u));
	const float v = saturatef(n + 0.25f * humidity01);
	return (v < 0.60f) ? 0 : (v < 0.90f) ? 1 : 2;
}



static inline TreeSpecies SpeciesFromBiome(Biome biome)
{
	RandomNumberGenerator rng;
	switch (biome) 
	{
	case Biome::Forest:
		return (rng.RollRandomFloatZeroToOne() <= 0.5f) ? TreeSpecies::OAK : TreeSpecies::BIRCH;

	case Biome::Taiga:
	case Biome::SnowyTaiga:
		return TreeSpecies::SPRUCE;

	case Biome::Jungle:
		return TreeSpecies::JUNGLE;

	case Biome::Plains:
	case Biome::SnowyPlains:
		return TreeSpecies::BIRCH;

	case Biome::Savanna:
		return (rng.RollRandomFloatZeroToOne() <= 0.7f) ? TreeSpecies::ACACIA : TreeSpecies::CACTUS;

	case Biome::Desert:
		return TreeSpecies::CACTUS;

	case Biome::SnowyBeach:
	case Biome::Beach:
	case Biome::Ocean:
	case Biome::DeepOcean:
	case Biome::FrozenOcean:
		return TreeSpecies::CACTUS;



	default:
		return TreeSpecies::NONE;
	}
}

static inline uint8_t PickSpruceLeavesByTemp(Chunk* c, float temperature01)
{
	return (temperature01 <= 0.30f) ? c->m_spruceLeavesSnow : c->m_spruceLeaves;
}

static inline uint8_t PickLeavesBySpecies(Chunk* c, TreeSpecies s, float temperature01) {
	switch (s) {
	case TreeSpecies::SPRUCE: return PickSpruceLeavesByTemp(c, temperature01); 
	case TreeSpecies::ACACIA: return c->m_acaciaLeaves;
	case TreeSpecies::BIRCH:  return c->m_birchLeaves;
	case TreeSpecies::JUNGLE: return c->m_jungleLeaves;
	case TreeSpecies::OAK:    return c->m_oakLeaves;
	default:                  return c->m_oakLeaves;
	}
}




static inline TreePalette MakePalette(Chunk* c, TreeSpecies s, float temperature01, float /*humidity01*/)
{
	const uint8_t leaves = PickLeavesBySpecies(c, s, temperature01);
	switch (s) {
	case TreeSpecies::SPRUCE: return TreePalette{ c->m_spruceLog, leaves, 6, 10 };
	case TreeSpecies::ACACIA: return TreePalette{ c->m_acaciaLog, leaves, 4,  7 };
	case TreeSpecies::BIRCH:  return TreePalette{ c->m_birchLog,  leaves, 5,  8 };
	case TreeSpecies::JUNGLE: return TreePalette{ c->m_jungleLog, leaves, 7, 11 };
	case TreeSpecies::OAK:    return TreePalette{ c->m_oakLog,    leaves, 5,  8 };
	case TreeSpecies::CACTUS: return TreePalette{ c->m_cactusLog, c->m_air, 2, 5 };
	default:                  return TreePalette{ c->m_oakLog,    c->m_oakLeaves, 5, 8 };
	}
}





Chunk::Chunk(const IntVec2& chunkXY, World* owner)
	: m_chunkCoords(chunkXY)
	, m_world(owner)
	, m_blockWorldSize(1.0f)
	, m_atlasDims(IntVec2(8, 8))
{
	m_vbo = nullptr; m_ibo = nullptr;
	m_vboCapacityBytes = m_iboCapacityBytes = 0;
	m_vboStrideBytes = sizeof(Vertex_PCU);
	m_iboStrideBytes = sizeof(unsigned int);

	m_debugVBO = nullptr; m_debugIBO = nullptr;
	m_dbgVboCapacityBytes = m_dbgIboCapacityBytes = 0;
	m_dbgVboStrideBytes = sizeof(Vertex_PCU);
	m_dbgIboStrideBytes = sizeof(unsigned int);

	const Vec3 mins(
		(float)m_chunkCoords.x * (float)CHUNK_SIZE_X * m_blockWorldSize,
		(float)m_chunkCoords.y * (float)CHUNK_SIZE_Y * m_blockWorldSize,
		0.0f
	);
	const Vec3 maxs(
		mins.x + (float)CHUNK_SIZE_X * m_blockWorldSize,
		mins.y + (float)CHUNK_SIZE_Y * m_blockWorldSize,
		(float)CHUNK_SIZE_Z * m_blockWorldSize
	);
	m_worldBounds = AABB3(mins, maxs);

	m_texture = m_world->m_game->m_texture;

	Initialize();
}

Chunk::~Chunk()
{
	if (m_vbo) 
	{ 
		delete m_vbo; 
		m_vbo = nullptr; 
	}

	if (m_ibo) 
	{ 
		delete m_ibo; 
		m_ibo = nullptr; 
	}

	if (m_debugVBO)
	{
		delete m_debugVBO;
		m_debugVBO = nullptr;
	}

	if (m_debugIBO)
	{
		delete m_debugIBO;
		m_debugIBO = nullptr;
	}

	m_vertices.clear(); 
	m_indices.clear();
	m_debugVertices.clear();
	m_debugIndices.clear();
	m_blocks.clear();

	m_east = nullptr;
	m_west = nullptr;
	m_north = nullptr;
	m_south = nullptr;
	m_above = nullptr;
	m_below = nullptr;



}

void Chunk::Initialize()
{
	m_blocks.resize(BLOCKS_PER_CHUNK);

	m_air = BlockDefinition::GetIndexByName("Air");
	m_water = BlockDefinition::GetIndexByName("Water");
	m_sand = BlockDefinition::GetIndexByName("Sand");
	m_snow = BlockDefinition::GetIndexByName("Snow");
	m_ice = BlockDefinition::GetIndexByName("Ice");
	m_dirt = BlockDefinition::GetIndexByName("Dirt");
	m_stone = BlockDefinition::GetIndexByName("Stone");
	m_coal = BlockDefinition::GetIndexByName("Coal");
	m_iron = BlockDefinition::GetIndexByName("Iron");
	m_gold = BlockDefinition::GetIndexByName("Gold");
	m_diamond = BlockDefinition::GetIndexByName("Diamond");
	m_obsidian = BlockDefinition::GetIndexByName("Obsidian");
	m_lava = BlockDefinition::GetIndexByName("Lava");
	m_glowstone = BlockDefinition::GetIndexByName("Glowstone");
	m_glowstoneRed = BlockDefinition::GetIndexByName("GlowstoneRed");
	m_glowstoneGreen = BlockDefinition::GetIndexByName("GlowstoneGreen");
	m_glowstoneBlue = BlockDefinition::GetIndexByName("GlowstoneBlue");

	m_cobblestone = BlockDefinition::GetIndexByName("Cobblestone");
	m_chiseledBrick = BlockDefinition::GetIndexByName("ChiseledBrick");
	m_grass = BlockDefinition::GetIndexByName("Grass");
	m_grassLight = BlockDefinition::GetIndexByName("GrassLight");
	m_grassDark = BlockDefinition::GetIndexByName("GrassDark");
	m_grassYellow = BlockDefinition::GetIndexByName("GrassYellow");
	m_acaciaLog = BlockDefinition::GetIndexByName("AcaciaLog");;
	m_acaciaPlanks = BlockDefinition::GetIndexByName("AcaciaPlanks");;
	m_acaciaLeaves = BlockDefinition::GetIndexByName("AcaciaLeaves");
	m_cactusLog = BlockDefinition::GetIndexByName("CactusLog");
	m_oakLog = BlockDefinition::GetIndexByName("OakLog");
	m_oakPlanks = BlockDefinition::GetIndexByName("OakPlanks");
	m_oakLeaves = BlockDefinition::GetIndexByName("OakLeaves");
	m_birchLog = BlockDefinition::GetIndexByName("BirchLog");
	m_birchPlanks = BlockDefinition::GetIndexByName("BirchPlanks");
	m_birchLeaves = BlockDefinition::GetIndexByName("BirchLeaves");
	m_jungleLog = BlockDefinition::GetIndexByName("JungleLog");
	m_junglePlanks = BlockDefinition::GetIndexByName("JunglePlanks");
	m_jungleLeaves = BlockDefinition::GetIndexByName("JungleLeaves");
	m_spruceLog = BlockDefinition::GetIndexByName("SpruceLog");
	m_sprucePlanks = BlockDefinition::GetIndexByName("SprucePlanks");
	m_spruceLeaves = BlockDefinition::GetIndexByName("SpruceLeaves");
	m_spruceLeavesSnow = BlockDefinition::GetIndexByName("SpruceLeavesSnow");

}




void Chunk::Update(float deltaSeconds)
{
	UNUSED(deltaSeconds);

/*
	if (m_isDirty) 
	{ 
		
	}*/
}

void Chunk::Render() const
{
	if (!m_vbo || !m_ibo || m_indexCountGPU <= 0) return;

	if (!m_isActive || !m_isRenderable) return;

	g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);
	g_theRenderer->SetSamplerMode(SamplerMode::POINT_CLAMP);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetStatesIfChanged();
	g_theRenderer->BindTexture(m_texture);

	g_theRenderer->DrawIndexedVertexBuffer(m_vbo, m_ibo, (unsigned)m_indexCountGPU);

}

void Chunk::DebugRender() const
{	
	if (!m_debugVBO || !m_debugIBO) return;
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
	g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->DrawIndexedVertexBuffer(m_debugVBO, m_debugIBO, (int)m_debugIndices.size());
}

void Chunk::Deactivate()
{
	if (!m_isActive) return;

	ReleaseGpuBuffers();
	ReleaseDebugBuffers();

	m_vertices.clear();
	m_indices.clear();
	m_debugVertices.clear();
	m_debugIndices.clear();
	m_isActive = false;
	m_isRenderable = false;
}

void Chunk::MarkDirty()
{
	if (!m_isDirty) 
	{
		m_isDirty = true;
		
		if (m_world) 
		{
			m_meshGeneration++;
		}
	}
}

void Chunk::MarkClean()
{
	if (m_world && m_inDirtyList) 
	{
		m_world->RemoveFromDirtyList(this);
		m_inDirtyList = false;
	}
	m_isDirty = false;
	
}

void Chunk::ReleaseDebugBuffers()
{
	if (m_debugVBO) { delete m_debugVBO; m_debugVBO = nullptr; }
	if (m_debugIBO) { delete m_debugIBO; m_debugIBO = nullptr; }
	m_dbgVboCapacityBytes = m_dbgIboCapacityBytes = 0;
}

void Chunk::ReleaseGpuBuffers()
{
	if (m_vbo) { delete m_vbo; m_vbo = nullptr; }
	if (m_ibo) { delete m_ibo; m_ibo = nullptr; }
	m_vboCapacityBytes = m_iboCapacityBytes = 0;
}

int Chunk::GlobalX(int lx) const
{
	{ return m_chunkCoords.x * CHUNK_SIZE_X + lx; }
}

int Chunk::GlobalY(int ly) const
{
	{ return m_chunkCoords.y * CHUNK_SIZE_Y + ly; }
}

int Chunk::GlobalZ(int lz) const
{
	{ (void)lz; return lz; }
}

uint8_t Chunk::GetTypeAtLocal(int lx, int ly, int lz) const
{
	if ((unsigned)lx >= (unsigned)CHUNK_SIZE_X) return m_air;
	if ((unsigned)ly >= (unsigned)CHUNK_SIZE_Y) return m_air;
	if ((unsigned)lz >= (unsigned)CHUNK_SIZE_Z) return m_air;
	const int idx = LocalCoordsToIndex(lx, ly, lz);
	return m_blocks[(size_t)idx].m_typeIndex;
}

uint8_t Chunk::GetTypeAtLocal(int blockIndex) const
{
	if (blockIndex < 0 || blockIndex >= BLOCKS_PER_CHUNK) 
	{
		return m_air;
	}
	return m_blocks[(size_t)blockIndex].m_typeIndex;
}

bool Chunk::SetTypeAtLocal(int lx, int ly, int lz, uint8_t newType)
{
	if ((unsigned)lx >= (unsigned)CHUNK_SIZE_X) return false;
	if ((unsigned)ly >= (unsigned)CHUNK_SIZE_Y) return false;
	if ((unsigned)lz >= (unsigned)CHUNK_SIZE_Z) return false;

	const int idx = LocalCoordsToIndex(lx, ly, lz);
	return SetTypeAtLocal(idx, newType);
}

bool Chunk::SetTypeAtLocal(int idx, uint8_t newType)
{
	if ((unsigned)idx >= (unsigned)BLOCKS_PER_CHUNK) return false;

	uint8_t& cur = m_blocks[(size_t)idx].m_typeIndex;
	if (cur == newType) return false;

	cur = newType;
	return true;
}

void Chunk::GenerateBlocks()
{
	for (int i = 0; i < BLOCKS_PER_CHUNK; ++i)
		m_blocks[(size_t)i].m_typeIndex = (uint8_t)BlockType::AIR;

	const unsigned terrainSeed = GAME_SEED + 0u;
	const unsigned humiditySeed = GAME_SEED + 1u;
	const unsigned temperatureSeed = GAME_SEED + 2u;
	const unsigned dirtSeed = GAME_SEED + 5u;
	const unsigned oreSeed = GAME_SEED + 6u;
	const unsigned erosionSeed = GAME_SEED + 7u;
	const unsigned contSeed = GAME_SEED + 8u;
	const unsigned peaksValleySeed = GAME_SEED + 9u;

	float humidityMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	float temperatureMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	float erosionMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	float continentalnessMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	float peaksValleysMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];
	int dirtDepthMap[CHUNK_SIZE_X * CHUNK_SIZE_Y];

	auto GetHeightOffset = [](float continent) -> float {
		float c = std::clamp(continent, 0.0f, 1.0f);
		return (0.5f - c) * 0.35f;
		};

	auto GetSquashFactor = [](float continent) -> float {
		float c = std::clamp(continent, 0.0f, 1.0f);
		return (1.0f - c) * 0.02f;
		};

	for (int ly = 0; ly < CHUNK_SIZE_Y; ++ly)
	{
		for (int lx = 0; lx < CHUNK_SIZE_X; ++lx)
		{
			const int gx = GlobalX(lx);
			const int gy = GlobalY(ly);
			const int idx = ly * CHUNK_SIZE_X + lx;

			humidityMap[idx] = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				HUMIDITY_NOISE_SCALE, HUMIDITY_NOISE_OCTAVES,
				DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
				true, humiditySeed));

			temperatureMap[idx] = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				TEMPERATURE_NOISE_SCALE, TEMPERATURE_NOISE_OCTAVES,
				DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
				true, temperatureSeed));

			erosionMap[idx] = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				EROSION_NOISE_SCALE, EROSION_NOISE_OCTAVES,
				DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
				true, erosionSeed));

			continentalnessMap[idx] = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				CONTINENTALNESS_NOISE_SCALE, CONTINENTALNESS_NOISE_OCTAVES,
				DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
				true, contSeed));

			peaksValleysMap[idx] = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				PEAKS_VALLEYS_NOISE_SCALE, PEAKS_VALLEYS_NOISE_OCTAVES,
				DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
				true, peaksValleySeed));

			const float dirt01 = To01(Compute2dPerlinNoise((float)gx, (float)gy,
				1.f, 1u, 1.f, 1.f, true, dirtSeed));

			const int dirtDepth = MIN_DIRT_OFFSET_Z +
				(int)(dirt01 * (float)(MAX_DIRT_OFFSET_Z - MIN_DIRT_OFFSET_Z) + 0.5f);

			dirtDepthMap[idx] = dirtDepth;
		}
	}

	for (int lz = 0; lz < CHUNK_SIZE_Z; ++lz)
		for (int ly = 0; ly < CHUNK_SIZE_Y; ++ly)
			for (int lx = 0; lx < CHUNK_SIZE_X; ++lx)
			{
				const int idx = LocalCoordsToIndex(lx, ly, lz);
				const int idxXY = ly * CHUNK_SIZE_X + lx;

				const int gx = GlobalX(lx);
				const int gy = GlobalY(ly);
				const int gz = lz;

				const float humidity = humidityMap[idxXY];
				const float temperature = temperatureMap[idxXY];
				const float erosion = erosionMap[idxXY];
				const float continentalness = continentalnessMap[idxXY];
				const float peaksValleys = peaksValleysMap[idxXY];

				Biome biome = GetBiomeForParams(temperature, humidity, erosion, continentalness, peaksValleys);

				if (gz == LAVA_Z) {
					m_blocks[idx].m_typeIndex = (uint8_t)BlockType::LAVA;
					continue;
				}
				if (gz == OBSIDIAN_Z) {
					m_blocks[idx].m_typeIndex = (uint8_t)BlockType::OBSIDIAN;
					continue;
				}

				float rawNoise = Compute3dPerlinNoise((float)gx, (float)gy, (float)gz,
					DENSITY_NOISE_SCALE, DENSITY_NOISE_OCTAVES,
					DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE,
					true, terrainSeed);

				float bias = DENSITY_BIAS_PER_Z * ((float)gz - DEFAULT_TERRAIN_HEIGHT);
				float density = rawNoise + bias;

				float h = GetHeightOffset(continentalness);
				float s = GetSquashFactor(continentalness);

				float baseHeight = DEFAULT_TERRAIN_HEIGHT + (h * 100.0f);

				float t = (gz - baseHeight) / std::max(baseHeight, 10.0f);

				density += h; 
				density -= s * t;

				if (gz > DEFAULT_TERRAIN_HEIGHT + 30)
					density += 0.02f * (gz - (DEFAULT_TERRAIN_HEIGHT + 30)) / 20.0f;

				if (density < 0.0f)
				{
					if (gz < 50) {
						const float raw = Compute3dPerlinNoise((float)gx, (float)gy, (float)gz,
							16.f, 3u, 1.f, 20.f, false, oreSeed);

						float ore01 = GetClamped(0.5f * (raw + 1.f), 0.f, 1.f);

						BlockType oreType = BlockType::STONE;
						if (ore01 < DIAMOND_CHANCE) oreType = BlockType::DIAMOND;
						else if (ore01 < GOLD_CHANCE) oreType = BlockType::GOLD;
						else if (ore01 < IRON_CHANCE) oreType = BlockType::IRON;
						else if (ore01 < COAL_CHANCE) oreType = BlockType::COAL;

						m_blocks[idx].m_typeIndex = (uint8_t)oreType;
					}
					else {
						m_blocks[idx].m_typeIndex = (uint8_t)BlockType::STONE;
					}
				}
				else
				{
					if (gz <= SEA_LEVEL_Z)
					{
						BlockType type = BlockType::WATER;
						if (gz == SEA_LEVEL_Z)
						{
							if (biome == Biome::FrozenOcean ||
								biome == Biome::SnowyPlains ||
								biome == Biome::SnowyTaiga ||
								biome == Biome::SnowyPeaks)
								type = BlockType::ICE;
							else if (biome == Biome::DeepOcean && temperature < 0.38f)
								type = BlockType::ICE;
						}
						else
						{
							float iceDepthF = DEFAULT_TERRAIN_HEIGHT -
								RangeMapClamped(temperature, ICE_TEMPERATURE_MAX, ICE_TEMPERATURE_MIN, ICE_DEPTH_MIN, ICE_DEPTH_MAX);
							if (temperature < 0.38f && (float)gz > iceDepthF)
								type = BlockType::ICE;
						}
						m_blocks[idx].m_typeIndex = (uint8_t)type;
					}
					else
					{
						m_blocks[idx].m_typeIndex = (uint8_t)BlockType::AIR;
					}
				}
			}

	for (int ly = 0; ly < CHUNK_SIZE_Y; ++ly)
	{
		for (int lx = 0; lx < CHUNK_SIZE_X; ++lx)
		{
			const int idxXY = ly * CHUNK_SIZE_X + lx;
			const float temperature = temperatureMap[idxXY];
			const float humidity = humidityMap[idxXY];
			const float erosion = erosionMap[idxXY];
			const float continentalness = continentalnessMap[idxXY];
			const float peaksValleys = peaksValleysMap[idxXY];
			const int dirtDepth = dirtDepthMap[idxXY];

			int surfaceZ = -1;
			for (int lz = CHUNK_SIZE_Z - 1; lz >= 0; --lz)
			{
				const int idx = LocalCoordsToIndex(lx, ly, lz);
				uint8_t blockType = m_blocks[idx].m_typeIndex;

				if (blockType != (uint8_t)BlockType::AIR &&
					blockType != (uint8_t)BlockType::WATER &&
					blockType != (uint8_t)BlockType::ICE)
				{
					surfaceZ = lz;
					break;
				}
			}

			if (surfaceZ >= 0 && surfaceZ < CHUNK_SIZE_Z - 5)
			{
				const int surfaceIdx = LocalCoordsToIndex(lx, ly, surfaceZ);
				Biome biome = GetBiomeForParams(temperature, humidity, erosion, continentalness, peaksValleys);
				BlockType surface = GetSurfaceBlockForBiome(biome);

				if (biome == Biome::Ocean)
				{
					surface = (surfaceZ >= SEA_LEVEL_Z) ? BlockType::SAND : BlockType::DIRT;
				}
				else if (biome == Biome::DeepOcean)
				{
					if (surfaceZ >= SEA_LEVEL_Z)
						surface = (temperature < 0.30f) ? BlockType::SNOW : BlockType::SAND;
				}
				else if (biome == Biome::FrozenOcean)
				{
					if (surfaceZ >= SEA_LEVEL_Z)
						surface = BlockType::SNOW;
				}

				m_blocks[surfaceIdx].m_typeIndex = GetBlockIndexFromBlockType(surface);

				const int dirtTopZ = surfaceZ - dirtDepth;
				for (int lz = surfaceZ - 1; lz >= 0 && lz >= dirtTopZ; --lz)
				{
					const int belowIdx = LocalCoordsToIndex(lx, ly, lz);
					if (surface == BlockType::GRASS ||
						surface == BlockType::GRASSLIGHT ||
						surface == BlockType::GRASSDARK ||
						surface == BlockType::GRASSYELLOW)
						m_blocks[belowIdx].m_typeIndex = (uint8_t)BlockType::DIRT;
					else if (surface == BlockType::SAND || surface == BlockType::SNOW)
						m_blocks[belowIdx].m_typeIndex = GetBlockIndexFromBlockType(surface);
				}

				if (biome == Biome::Desert)
				{
					for (int d = 1; d <= 5 && (surfaceZ - d) >= 0; ++d)
					{
						const int belowIdx = LocalCoordsToIndex(lx, ly, surfaceZ - d);
						m_blocks[belowIdx].m_typeIndex = (uint8_t)BlockType::SAND;
					}
				}
			}
		}
	}

	ScatterTrees(
		temperatureMap,
		humidityMap,
		erosionMap,
		continentalnessMap,
		peaksValleysMap);
}

enum : uint8_t
{
	FX_NegX = 1 << 0,
	FX_PosX = 1 << 1,
	FX_NegY = 1 << 2,
	FX_PosY = 1 << 3,
	FX_NegZ = 1 << 4,
	FX_PosZ = 1 << 5,
};


void Chunk::AddVertsForChunk()
{
	m_vertices.clear();
	m_indices.clear();

	m_vertices.reserve(BLOCKS_PER_CHUNK * 2 * 4);
	m_indices.reserve(BLOCKS_PER_CHUNK * 2 * 6);

	const float s = m_blockWorldSize;
	const float wx0 = m_worldBounds.m_mins.x;
	const float wy0 = m_worldBounds.m_mins.y;
	const float wz0 = m_worldBounds.m_mins.z;

	auto faceHidden = [](const BlockIterator& nb) -> bool
		{
			if (nb.m_chunk == nullptr || nb.m_blockIdx < 0)
				return false;

			const uint8_t nt = nb.m_chunk->GetTypeAtLocal(nb.m_blockIdx);
			return BlockDefinition::GetDefinitionByIndex(nt).m_isOpaque;
		};

	for (int idx = 0; idx < BLOCKS_PER_CHUNK; ++idx)
	{
		const int lx = (idx & CHUNK_MASK_X);
		const int ly = ((idx & CHUNK_MASK_Y) >> SHIFT_Y);
		const int lz = ((idx & CHUNK_MASK_Z) >> SHIFT_Z);

		const uint8_t t = GetTypeAtLocal(idx);
		const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(t);
		if (!def.m_isVisible)
			continue;

		BlockIterator it(this, idx);


		const float wx = wx0 + float(lx) * s;
		const float wy = wy0 + float(ly) * s;
		const float wz = wz0 + float(lz) * s;

		const Vec3 mins(wx, wy, wz);
		const Vec3 maxs(wx + s, wy + s, wz + s);

		const AABB2& sideuv = def.m_sideUV;
		const AABB2& botuv = def.m_botUV;
		const AABB2& topuv = def.m_topUV;

		{
			BlockIterator nb = it.GetWestNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, EW_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(mins.x, maxs.y, mins.z),
					Vec3(mins.x, mins.y, mins.z),
					Vec3(mins.x, mins.y, maxs.z),
					Vec3(mins.x, maxs.y, maxs.z),
					faceColor, sideuv);
			}
		}

		{
			BlockIterator nb = it.GetEastNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, EW_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(maxs.x, mins.y, mins.z),
					Vec3(maxs.x, maxs.y, mins.z),
					Vec3(maxs.x, maxs.y, maxs.z),
					Vec3(maxs.x, mins.y, maxs.z),
					faceColor, sideuv);
			}
		}

		{
			BlockIterator nb = it.GetSouthNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, NS_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(mins.x, mins.y, mins.z),
					Vec3(maxs.x, mins.y, mins.z),
					Vec3(maxs.x, mins.y, maxs.z),
					Vec3(mins.x, mins.y, maxs.z),
					faceColor, sideuv);
			}
		}

		{
			BlockIterator nb = it.GetNorthNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, NS_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(maxs.x, maxs.y, mins.z),
					Vec3(mins.x, maxs.y, mins.z),
					Vec3(mins.x, maxs.y, maxs.z),
					Vec3(maxs.x, maxs.y, maxs.z),
					faceColor, sideuv);
			}
		}

		{
			BlockIterator nb = it.GetBelowNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, TOP_BOT_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(mins.x, mins.y, mins.z),
					Vec3(mins.x, maxs.y, mins.z),
					Vec3(maxs.x, maxs.y, mins.z),
					Vec3(maxs.x, mins.y, mins.z),
					faceColor, botuv);
			}
		}

		{
			BlockIterator nb = it.GetAboveNeighbor();
			if (!faceHidden(nb))
			{
				Rgba8 faceColor = MakeFaceLightColor(it, nb, TOP_BOT_COLOR);
				AddVertsForQuad3D(
					m_vertices, m_indices,
					Vec3(mins.x, maxs.y, maxs.z),
					Vec3(mins.x, mins.y, maxs.z),
					Vec3(maxs.x, mins.y, maxs.z),
					Vec3(maxs.x, maxs.y, maxs.z),
					faceColor, topuv);
			}
		}
	}
}





void Chunk::AddVertsForDebug()
{
	m_debugVertices.clear(); m_debugIndices.clear();

	AddVertsForAABBWireframe3D(m_debugVertices, m_debugIndices, m_worldBounds, 0.1f);
}




uint8_t Chunk::GetBlockIndexFromBlockType(BlockType type)
{
	switch (type)
	{
	case BlockType::AIR:
		return m_air;
		break;
	case BlockType::WATER:
		return m_water;
		break;
	case BlockType::SAND:
		return m_sand;
		break;
	case BlockType::SNOW:
		return m_snow;
		break;
	case BlockType::ICE:
		return m_ice;
		break;
	case BlockType::DIRT:
		return m_dirt;
		break;
	case BlockType::STONE:
		return m_stone;
		break;
	case BlockType::COAL:
		return m_coal;
		break;
	case BlockType::IRON:
		return m_iron;
		break;
	case BlockType::GOLD:
		return m_gold;
		break;
	case BlockType::DIAMOND:
		return m_diamond;
		break;
	case BlockType::OBSIDIAN:
		return m_obsidian;
		break;
	case BlockType::LAVA:
		return m_lava;
		break;
	case BlockType::GLOWSTONE:
		return m_glowstone;
		break;
	case BlockType::COBBLESTONE:
		return m_cobblestone;
		break;
	case BlockType::CHISELEDBRICK:
		return m_chiseledBrick;
		break;
	case BlockType::GRASS:
		return m_grass;
		break;
	case BlockType::GRASSLIGHT:
		return m_grassLight;
		break;
	case BlockType::GRASSDARK:
		return m_grassDark;
		break;
	case BlockType::GRASSYELLOW:
		return m_grassYellow;
		break;
	case BlockType::ACACIALOG:
		return m_acaciaLog;
		break;
	case BlockType::ACACIAPLANKS:
		return m_acaciaPlanks;
		break;
	case BlockType::ACACIALEAVES:
		return m_acaciaLeaves;
		break;
	case BlockType::CACTUSLOG:
		return m_cactusLog;
		break;
	case BlockType::OAKLOG:
		return m_oakLog;
		break;
	case BlockType::OAKPLANKS:
		return m_oakPlanks;
		break;
	case BlockType::OAKLEAVES:
		return m_oakLeaves;
		break;
	case BlockType::BIRCHLOG:
		return m_birchLog;
		break;
	case BlockType::BIRCHPLANKS:
		return m_birchPlanks;
		break;
	case BlockType::BIRCHLEAVES:
		return m_birchLeaves;
		break;
	case BlockType::JUNGLELOG:
		return m_jungleLog;
		break;
	case BlockType::JUNGLEPLANKS:
		return m_junglePlanks;
		break;
	case BlockType::JUNGLELEAVES:
		return m_jungleLeaves;
		break;
	case BlockType::SPRUCELOG:
		return m_spruceLog;
		break;
	case BlockType::SPRUCEPLANKS:
		return m_sprucePlanks;
		break;
	case BlockType::SPRUCELEAVES:
		return m_spruceLeaves;
		break;
	case BlockType::SPRUCELEAVESSNOW:
		return m_spruceLeavesSnow;
		break;
	case BlockType::NUM_BLOCK_TYPE:
		return m_air;
		break;
	default:
		return m_air;
		break;
	}
}

int Chunk::CountVisibleBlocks() const
{
	int visibleCount = 0;

	for (int idx = 0; idx < BLOCKS_PER_CHUNK; ++idx) 
	{
		const uint8_t blockType = m_blocks[idx].m_typeIndex;
		if (BlockDefinition::GetDefinitionByIndex(blockType).m_isVisible) 
		{
			visibleCount++;
		}
	}

	return visibleCount;
}

void Chunk::PlaceTreeStampAt(int lxRoot, int lyRoot, int groundZ,
	const TreeStamp& stamp, uint8_t logIdx, uint8_t leavesIdx)
{
	int topLogDzPlaced = INT_MIN;
	int trunkTopLx = lxRoot;
	int trunkTopLy = lyRoot;

	for (int i = 0; i < stamp.count; ++i) {
		const StampBlock& sb = stamp.data[i];
		if (sb.kind != LOG) continue;
		const int lx = lxRoot + (int)sb.dx;
		const int ly = lyRoot + (int)sb.dy;
		const int dz = (int)sb.dz;
		const int lz = groundZ + dz;
		if (lz <= groundZ) continue;
		if (!InLocalBounds(lx, ly, lz)) continue;
		if (GetTypeAtLocal(lx, ly, lz) != m_air) continue;
		if (SetTypeAtLocal(lx, ly, lz, logIdx)) {
			if (dz > topLogDzPlaced) {
				topLogDzPlaced = dz;
				trunkTopLx = lx; 
				trunkTopLy = ly;
			}
		}
	}

	for (int i = 0; i < stamp.count; ++i) {
		const StampBlock& sb = stamp.data[i];
		if (sb.kind != LEAVES) continue;
		const int lx = lxRoot + (int)sb.dx;
		const int ly = lyRoot + (int)sb.dy;
		const int lz = groundZ + (int)sb.dz;
		if (lz <= groundZ) continue;
		if (!InLocalBounds(lx, ly, lz)) continue;
		if (GetTypeAtLocal(lx, ly, lz) != m_air) continue;
		SetTypeAtLocal(lx, ly, lz, leavesIdx);
	}

	if (topLogDzPlaced != INT_MIN) {
		const int zCap = groundZ + topLogDzPlaced + 1;
		if (InLocalBounds(trunkTopLx, trunkTopLy, zCap) &&
			GetTypeAtLocal(trunkTopLx, trunkTopLy, zCap) == m_air) {
			SetTypeAtLocal(trunkTopLx, trunkTopLy, zCap, leavesIdx);
		}
	}

}




void Chunk::ScatterTrees(
	const float* temperatureMap,
	const float* humidityMap,
	const float* erosionMap,
	const float* continentalnessMap,
	const float* peaksValleysMap)
{
	const int nxBeg = -NOISE_BORDER;
	const int nyBeg = -NOISE_BORDER;
	const int nxEnd = CHUNK_SIZE_X + NOISE_BORDER;
	const int nyEnd = CHUNK_SIZE_Y + NOISE_BORDER;

	static constexpr int MAX_TREE_RADIUS = 3;
	const int safeXBeg = MAX_TREE_RADIUS;
	const int safeYBeg = MAX_TREE_RADIUS;
	const int safeXEnd = CHUNK_SIZE_X - MAX_TREE_RADIUS - 1;
	const int safeYEnd = CHUNK_SIZE_Y - MAX_TREE_RADIUS - 1;

	static constexpr int R = 2;

	auto densityMask01 = [&](int gx, int gy) -> float {
		float base = To01(Compute2dPerlinNoise((float)gx * 0.30f, (float)gy * 0.30f,
			1.f, 3u, DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE, true, TREE_SEED + 11u));
		float detail = To01(Compute2dPerlinNoise((float)gx * 1.10f, (float)gy * 1.10f,
			1.f, 2u, DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE, true, TREE_SEED + 12u));
		return 0.7f * base + 0.3f * detail;
		};

	auto blueKey01 = [&](int gx, int gy) -> float {
		return To01(Compute2dPerlinNoise((float)gx * 0.90f, (float)gy * 0.90f,
			1.f, 1u, DEFAULT_OCTAVE_PERSISTANCE, DEFAULT_NOISE_OCTAVE_SCALE, true, TREE_SEED + 777u));
		};

	for (int ny = nyBeg; ny < nyEnd; ++ny) {
		for (int nx = nxBeg; nx < nxEnd; ++nx) {
			const int lxCenter = nx;
			const int lyCenter = ny;

			if (lxCenter < safeXBeg || lxCenter > safeXEnd ||
				lyCenter < safeYBeg || lyCenter > safeYEnd) {
				continue;
			}

			const int gx = GlobalX(lxCenter);
			const int gy = GlobalY(lyCenter);
			const int clx = lxCenter;
			const int cly = lyCenter;
			const int li = cly * CHUNK_SIZE_X + clx;

			const float T = temperatureMap[li];
			const float H = humidityMap[li];
			const Biome biome = GetBiomeForParams(
				T, H, erosionMap[li], continentalnessMap[li], peaksValleysMap[li]);

			const float p = TreeProbForParams(biome);

			const float mask = densityMask01(gx, gy);
			if (mask > p) continue;

			const float selfKey = blueKey01(gx, gy);
			bool isPeak = true;
			for (int oy = -R; oy <= R && isPeak; ++oy) {
				for (int ox = -R; ox <= R; ++ox) {
					if (ox == 0 && oy == 0) continue;
					const float k = blueKey01(gx + ox, gy + oy);
					if (k >= selfKey) { isPeak = false; break; }
				}
			}
			if (!isPeak) continue;

			int groundZ = -1;
			for (int lz = CHUNK_SIZE_Z - 1; lz >= 0; --lz)
			{
				const int idx = LocalCoordsToIndex(clx, cly, lz);
				uint8_t blockType = m_blocks[idx].m_typeIndex;

				if (blockType != (uint8_t)BlockType::AIR &&
					blockType != (uint8_t)BlockType::WATER &&
					blockType != (uint8_t)BlockType::ICE)
				{
					groundZ = lz;
					break;
				}
			}

			if (groundZ == -1) continue;

			const int maxTreeAltitude = CHUNK_MAX_Z - 12;
			if (groundZ > maxTreeAltitude) continue;

			if (GetTypeAtLocal(clx, cly, groundZ + 1) != m_air) continue;

			const TreeSpecies species = SpeciesFromBiome(biome);
			const StampSet3 stamps = ChooseStampSet(species);
			const int tier = ChooseSizeTier(gx, gy, H);
			const TreeStamp* chosen = (tier == 0) ? &stamps.small : (tier == 1) ? &stamps.medium : &stamps.large;
			const TreePalette pal = MakePalette(this, species, T, H);

			PlaceTreeStampAt(lxCenter, lyCenter, groundZ, *chosen, pal.logIdx, pal.leavesIdx);
		}
	}
}





static inline unsigned int GrowTo(unsigned int need, unsigned int cur) 
{
	if (cur >= need) return cur;
	unsigned int cap = (cur == 0) ? need : cur;
	while (cap < need) cap <<= 1; 
	return cap;
}

void Chunk::UpdateBuffersForChunk()
{
	const unsigned int vtxStride = sizeof(Vertex_PCU);
	const unsigned int vtxCount = (unsigned int)m_vertices.size();
	const unsigned int vtxBytes = vtxStride * vtxCount;

	const unsigned int idxStride = sizeof(uint32_t);
	const unsigned int idxCount = (unsigned int)m_indices.size();
	const unsigned int idxBytes = idxStride * idxCount;

	if (vtxBytes == 0 || idxBytes == 0)
	{
		ReleaseGpuBuffers();
		m_indexCountGPU = 0;
		return;
	}

	auto grow = [](unsigned int need, unsigned int cur)
		{
			if (cur >= need) return cur;
			unsigned int cap = (cur == 0) ? need : cur;
			while (cap < need) cap <<= 1;
			return cap;
		};

	const bool needRecreateVBO = (!m_vbo) ||
		(m_vboStrideBytes != vtxStride) ||
		(m_vboCapacityBytes < vtxBytes);

	const bool needRecreateIBO = (!m_ibo) ||
		(m_iboStrideBytes != idxStride) ||
		(m_iboCapacityBytes < idxBytes);

	if (needRecreateVBO)
	{
		if (m_vbo) { delete m_vbo; m_vbo = nullptr; }
		m_vboCapacityBytes = grow(vtxBytes, m_vboCapacityBytes);
		m_vboStrideBytes = vtxStride;
		m_vbo = g_theRenderer->CreateVertexBuffer(m_vboCapacityBytes, m_vboStrideBytes);
	}

	if (needRecreateIBO)
	{
		if (m_ibo) { delete m_ibo; m_ibo = nullptr; }
		m_iboCapacityBytes = grow(idxBytes, m_iboCapacityBytes);
		m_iboStrideBytes = idxStride;
		m_ibo = g_theRenderer->CreateIndexBuffer(m_iboCapacityBytes, m_iboStrideBytes);
	}

	g_theRenderer->CopyCPUToGPU(
		m_vbo, m_ibo,
		m_vertices.data(), m_indices.data(),
		(int)vtxCount, (int)idxCount);

	m_indexCountGPU = (int)idxCount;
}



void Chunk::UpdateDebugBuffersForChunk()
{
	const unsigned int vtxStride = sizeof(Vertex_PCU);
	const unsigned int vtxCount = (unsigned int)m_debugVertices.size();
	const unsigned int vtxBytes = vtxStride * vtxCount;

	const unsigned int idxStride = sizeof(uint32_t);
	const unsigned int idxCount = (unsigned int)m_debugIndices.size();
	const unsigned int idxBytes = idxStride * idxCount;

	if (vtxBytes == 0 || idxBytes == 0) 
	{
		if (m_debugVBO) { delete m_debugVBO; m_debugVBO = nullptr; }
		if (m_debugIBO) { delete m_debugIBO; m_debugIBO = nullptr; }
		m_dbgVboCapacityBytes = m_dbgIboCapacityBytes = 0;
		return;
	}

	const bool needRecreateVBO = (!m_debugVBO) || (m_dbgVboStrideBytes != vtxStride) || (m_dbgVboCapacityBytes < vtxBytes);
	const bool needRecreateIBO = (!m_debugIBO) || (m_dbgIboStrideBytes != idxStride) || (m_dbgIboCapacityBytes < idxBytes);

	auto grow = [](unsigned int need, unsigned int cur) 
	{
		if (cur >= need) return cur;
		unsigned int cap = (cur == 0) ? need : cur;
		while (cap < need) cap <<= 1;
		return cap;
	};

	if (needRecreateVBO)
	{
		if (m_debugVBO) { delete m_debugVBO; m_debugVBO = nullptr; }
		m_dbgVboCapacityBytes = grow(vtxBytes, m_dbgVboCapacityBytes);
		m_dbgVboStrideBytes = vtxStride;
		m_debugVBO = g_theRenderer->CreateVertexBuffer(m_dbgVboCapacityBytes, m_dbgVboStrideBytes);
	}
	if (needRecreateIBO)
	{
		if (m_debugIBO) { delete m_debugIBO; m_debugIBO = nullptr; }
		m_dbgIboCapacityBytes = grow(idxBytes, m_dbgIboCapacityBytes);
		m_dbgIboStrideBytes = idxStride;
		m_debugIBO = g_theRenderer->CreateIndexBuffer(m_dbgIboCapacityBytes, m_dbgIboStrideBytes);
	}

	g_theRenderer->CopyCPUToGPU(m_debugVBO, m_debugIBO,
		m_debugVertices.data(), m_debugIndices.data(),
		(int)vtxCount, (int)idxCount);
}
