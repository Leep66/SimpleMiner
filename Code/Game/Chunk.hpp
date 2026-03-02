#pragma once
#include <vector>
#include <cstdint>
#include "Game/Block.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCUTBN.hpp"
#include "Engine/Renderer/Renderer.hpp" 
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Engine/Math/AABB2.hpp"
#include <atomic>

class Renderer;
class World;
class BlockIterator;

enum Face : int { SIDE, TOP, BOTTOM };

enum class ChunkState
{
	MISSING,
	ON_DISK,
	CONSTRUCTING,

	ACTIVATING_QUEUED_LOAD,
	ACTIVATING_LOADING,
	ACTIVATING_LOAD_COMPLETE,

	ACTIVATING_QUEUED_GENERATE,
	ACTIVATING_GENERATING,
	ACTIVATING_GENERATE_COMPLETE,

	ACTIVE,

	DEACTIVATING_QUEUED_SAVE,
	DEACTIVATING_SAVING,
	DEACTIVATING_SAVE_COMPLETE,
	DECONSTRUCTING,

	NUM_CHUNK_STATES
};

enum StampBlockKind : uint8_t { LOG = 0, LEAVES = 1, PLANKS = 2 };

struct StampBlock
{
	int8_t dx, dy, dz;
	StampBlockKind kind;
};

struct TreeStamp 
{
	const StampBlock* data;
	int count;
};

struct TreePalette 
{
	uint8_t logIdx;
	uint8_t leavesIdx;
	int     minH;
	int     maxH;
};

enum class TreeSpecies 
{
	NONE = -1,
	OAK,
	SPRUCE,
	ACACIA,
	BIRCH,
	JUNGLE,
	CACTUS
};


class Chunk
{
public:

	Chunk(const IntVec2& chunkXY, World* owner);
	~Chunk();

	void Initialize();

	void Update(float deltaSeconds);

	void Render() const;
	void DebugRender() const;

	void Deactivate();

	void MarkDirty();
	void MarkClean();

	void ReleaseGpuBuffers();
	void ReleaseDebugBuffers();

	size_t GetVertexCount() const { return m_vertexCountGPU; }
	size_t GetIndexCount()  const { return m_indexCountGPU; }

	bool IsDirty() const { return m_isDirty; }
	bool NeedsSaving() const { return m_needsSaving; }
	inline const IntVec2& GetChunkCoords() const { return m_chunkCoords; }
	inline const AABB3& GetWorldBounds() const { return m_worldBounds; }

	inline int GlobalX(int lx) const;
	inline int GlobalY(int ly) const;
	inline int GlobalZ(int lz) const;

	void UpdateBuffersForChunk();
	void UpdateDebugBuffersForChunk();

	void GenerateBlocks();

	uint8_t  GetTypeAtLocal(int blockIndex) const;
	bool     SetTypeAtLocal(int blockIndex, uint8_t typeIndex);

	uint8_t  GetTypeAtLocal(int lx, int ly, int lz) const;
	bool     SetTypeAtLocal(int lx, int ly, int lz, uint8_t typeIndex);

	void AddVertsForChunk();
	void AddVertsForDebug();

	inline Block& GetBlockRef(int blockIdx)
	{
		return m_blocks[blockIdx];
	}


public:

	std::vector<Vertex_PCU> m_vertices;
	std::vector<uint32_t>   m_indices;

	std::vector<Vertex_PCU> m_debugVertices;
	std::vector<uint32_t> m_debugIndices;
	IntVec2 m_chunkCoords = IntVec2(0, 0);

	bool m_needsSaving = false;
	bool m_isDirty = false;
	bool m_inDirtyList = false;
	bool m_isActive = false;
	bool m_isRenderable = false;

	Chunk* m_east = nullptr;
	Chunk* m_west = nullptr;
	Chunk* m_north = nullptr;
	Chunk* m_south = nullptr;
	Chunk* m_above = nullptr;
	Chunk* m_below = nullptr;

	AABB3   m_worldBounds;

	std::vector<Block> m_blocks;

	World* m_world = nullptr;

	std::atomic<ChunkState> m_state{ ChunkState::MISSING };
	std::atomic<uint32_t>   m_meshGeneration{ 0 };


private:
	
	uint8_t GetBlockIndexFromBlockType(BlockType type);
	int CountVisibleBlocks() const;

	void PlaceTreeStampAt(int lxRoot, int lyRoot, int groundZ,
		const TreeStamp& stamp, uint8_t logIdx, uint8_t leavesIdx);
	void ScatterTrees(
		const float* temperatureMap,
		const float* humidityMap,
		const float* erosionMap,
		const float* continentalnessMap,
		const float* peaksValleysMap);
private:
	size_t m_vertexCountGPU = 0;
	size_t m_indexCountGPU = 0;
	
	float   m_blockWorldSize = 1.0f;
	IntVec2 m_atlasDims = IntVec2(8, 8);

	Texture* m_texture = nullptr;
	
	VertexBuffer* m_vbo = nullptr;
	IndexBuffer* m_ibo = nullptr;
	unsigned int  m_vboCapacityBytes = 0;
	unsigned int  m_iboCapacityBytes = 0;
	unsigned int  m_vboStrideBytes = sizeof(Vertex_PCU);
	unsigned int  m_iboStrideBytes = sizeof(unsigned int);

	VertexBuffer* m_debugVBO = nullptr;
	IndexBuffer* m_debugIBO = nullptr;
	unsigned int  m_dbgVboCapacityBytes = 0;
	unsigned int  m_dbgIboCapacityBytes = 0;
	unsigned int  m_dbgVboStrideBytes = sizeof(Vertex_PCU);
	unsigned int  m_dbgIboStrideBytes = sizeof(unsigned int);

public:
	uint8_t m_air = 0;
	uint8_t m_water = 0;
	uint8_t m_sand = 0;
	uint8_t m_snow = 0;
	uint8_t m_ice = 0;
	uint8_t m_dirt = 0;
	uint8_t m_stone = 0;
	uint8_t m_coal = 0;
	uint8_t m_iron = 0;
	uint8_t m_gold = 0;
	uint8_t m_diamond = 0;
	uint8_t m_obsidian = 0;
	uint8_t m_lava = 0;
	uint8_t m_glowstone = 0;
	uint8_t m_glowstoneRed = 0;
	uint8_t	m_glowstoneGreen = 0;
	uint8_t	m_glowstoneBlue = 0;
	uint8_t m_cobblestone = 0;
	uint8_t m_chiseledBrick = 0;
	uint8_t m_grass = 0;
	uint8_t m_grassLight = 0;
	uint8_t m_grassDark = 0;
	uint8_t m_grassYellow = 0;
	uint8_t m_acaciaLog = 0;
	uint8_t m_acaciaPlanks = 0;
	uint8_t m_acaciaLeaves = 0;
	uint8_t m_cactusLog = 0;
	uint8_t m_oakLog = 0;
	uint8_t m_oakPlanks = 0;
	uint8_t m_oakLeaves = 0;
	uint8_t m_birchLog = 0;
	uint8_t m_birchPlanks = 0;
	uint8_t m_birchLeaves = 0;
	uint8_t m_jungleLog = 0;
	uint8_t m_junglePlanks = 0;
	uint8_t m_jungleLeaves = 0;
	uint8_t m_spruceLog = 0;
	uint8_t m_sprucePlanks = 0;
	uint8_t m_spruceLeaves = 0;
	uint8_t m_spruceLeavesSnow = 0;

	

};
