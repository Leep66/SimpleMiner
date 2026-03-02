#pragma once
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <mutex>
#include <deque>
#include "Engine/Math/IntVec2.hpp"
#include "Engine/Math/IntVec3.hpp"
#include "Game/BlockIterator.hpp"

class Renderer;
class Camera;
class Chunk;
class Game;
class JobSystem;
struct Job;
struct CompletedCpuMesh;
class Clock;
class ParticleEmitter;
class ParticleSystem;

struct WorldConstants
{
	Vec4 IndoorLightColor;
	Vec4 OutdoorLightColor;
	Vec4 SkyColor;
	float FogNearDistance = 200.f;
	float FogFarDistance = 230.f;
	float GlobalLightScalar = 1.f;
	float Padding = 0.f;
};

enum class WeatherType
{
	SUNNY,
	CLOUDY,
	RAIN,
	STORM,
	SNOW
};

struct WeatherState
{
	WeatherType m_type = WeatherType::SUNNY;

	float m_ambientScale = 1.0f;
	float m_fogNear = 200.0f;
	float m_fogFar = 230.0f; 

	float m_rainRate = 0.0f;
	float m_snowRate = 0.0f;
	float m_stormIntensity = 0.0f;
};



struct GameRaycastResult3D : public RaycastResult3D
{
	IntVec3       m_hitCell = IntVec3::ZERO;
	Vec3          m_faceNormal = Vec3::ZERO;
	BlockIterator m_blockIter;

	void Clear()
	{
		m_didImpact = false;
		m_impactDist = 0.f;
		m_impactPos = Vec3::ZERO;
		m_impactNormal = Vec3::ZERO;
		m_hitCell = IntVec3::ZERO;
		m_faceNormal = Vec3::ZERO;
		m_blockIter = BlockIterator();

		m_rayStart = Vec3::ZERO;
		m_rayForwardNormal = Vec3::ZERO;
		m_rayMaxLength = 1.f;
	}
};

struct FluidSource
{
	IntVec3 position;
	int     remainingSpreads = 0;
	float   timer = 0.f;
	int     rootId = 0;
	bool    isRootSource = false;
};


struct WaterCell
{
	IntVec3 pos;
	int rootId;
};



class World
{
public:
	World(Game* owner);
	~World();

	void Update(float deltaSeconds);
	void Render() const;
	void DebugRender() const;
	int GetChunksVertsSize() const;
	int GetChunksIndicesSize() const;
	Chunk* FindActiveChunk(const IntVec2& chunkCoords);
	Chunk* FindChunkAt(const IntVec2& chunkCoords);
	Chunk* FindFirstDirtyChunk() const;
	uint8_t CurrentPlaceType() const;

	std::string GetPlaceModeText() const;

	void DeactivateAllChunks();

	bool DigBlock();
	bool PlaceBlock();

	void InitLightingForChunk(Chunk* chunk);
	void MarkBoundaryBlocksDirty(Chunk* chunk);

	void MarkHorizontalNeighborsDirty(Chunk* chunk, int x, int y, int z);

	bool RaycastSolidBlock(const Vec3& start, const Vec3& dir, float maxDist,
		IntVec3& outCell, Vec3& outImpactPos, Vec3& outNormal) const;
	bool GetBlockTypeAtCell(const IntVec3& cell, uint8_t& outType) const;
	bool SetBlockTypeAtCell(const IntVec3& cell, uint8_t type); 
	void UpdateFluids(float deltaSeconds);


	void MarkChunkDirty(Chunk* c);
	void MarkNeighborsDirtyIfNeeded(Chunk* c, const IntVec2& cc, int lx, int ly, int lz);

	void RemoveFromDirtyList(Chunk* c);
	Chunk* PickNearestDirty(int camCX, int camCY);
	bool FindNearestMissingInRange(const IntVec2& camCC, int camCX, int camCY, int act2, IntVec2& outCC);
	bool FindFarthestToDeactivate(int camCX, int camCY, int deact2, IntVec2& outKey);

	bool FindTopSolidBelowCamera(const IntVec3& camCell,
		Chunk*& outChunk,
		IntVec2& outCC,
		int& outLX, int& outLY, int& outLZ);

	void PumpCompletedJobs(size_t maxPerFrame = 8);

	bool UploadOneMeshIfAvailable();

	void RebuildChunkMesh(Chunk* c);

	void RebuildDirtyChunksNearCamera(int maxPerFrame);
	
	void DebugPrintBlockLightingInFrontOfPlayer();

	bool RaycastVsBlocks(
		const Vec3& start,
		const Vec3& forward,
		float maxDist,
		GameRaycastResult3D& out);

	void UpdateRaycastFromCamera(); 
	const GameRaycastResult3D& GetRaycast() const { return m_currentRaycast; }
	void ToggleRayLock();
	std::string GetWeatherText() const;
	std::string GetCurrentTimeText() const;
	void StartDrainFromRoot(int rootId);
	void StartDrainFrom(const IntVec3& startCell);


public:
	JobSystem* m_jobSystem = nullptr;
	std::vector<Chunk*>  m_dirtyList;

	std::map<IntVec2, Chunk*> m_activeChunks;
	std::map<IntVec2, std::unique_ptr<Chunk>> m_inflightChunks;
	std::set<IntVec2> m_queuedForActivate, m_queuedForDeactivate;
	Game* m_game = nullptr;

	uint8_t m_glowstone = 0;
	uint8_t m_cobblestone = 0;
	uint8_t m_chiseledBrick = 0;
	uint8_t m_water = 0;
	uint8_t m_glowstoneRed = 0;
	uint8_t m_glowstoneGreen = 0;
	uint8_t m_glowstoneBlue = 0;


	uint8_t m_air = 0;

	uint8_t m_placeMode = 0;
	Camera* m_camera = nullptr;
	Clock* m_worldClock = nullptr;
	float m_worldTimeDays = 0.5f;

	bool m_isRaylock = false;

	GameRaycastResult3D m_currentRaycast;
	GameRaycastResult3D m_lockedRaycast;

	Vec4 m_currentSkyColor = Vec4(0.78f, 0.85f, 0.9f, 1.f);

	std::vector<WaterCell>   m_waterBlocks;
	std::vector<IntVec3>     m_drainingWater;
	std::vector<FluidSource> m_waterSources;
	std::vector<IntVec3> m_waterSourceCells;

	int                      m_nextWaterRootId = 1;

	float m_fluidUpdateTimer = 0.0f;

	float m_drainTimer = 0.0f;

	static constexpr float FLUID_UPDATE_INTERVAL = 0.1f; 
	static constexpr int MAX_WATER_SPREADS = 32;
private:
	void ActivateChunkAt(const IntVec2& chunkCoords);
	void DeactivateChunkAt(const IntVec2& chunkCoords);
	void RemoveAllFromDirtyList(Chunk* c);
	bool LoadChunkFromDisk(Chunk& chunk);
	void SaveChunkToDisk(Chunk& chunk);  
	void GenerateBlocksForChunk(Chunk& chunk);
	bool HasAll4NeighborsActive(Chunk* c) const;
	void OnChunkActive(Chunk* c);

	void DebugPrintBlockLightingAtCell(const IntVec3& cell);
	void UpdateWorldConstants(float deltaSeconds);

	void        UpdateWeather(float deltaSeconds);
	WeatherState GetWeatherPreset(WeatherType type) const;
	float       GetDayFactor() const;

	void        SetWeather(WeatherType type);
	void        RandomizeNextWeather(); 
	void		InitWeatherEmitters();
	void		UpdateWeatherEmittersForState();


private:
	float   m_blockWorldSize = 1.0f;
	IntVec2 m_atlasDims = IntVec2(8, 8);

	uint32_t m_meshGeneration = 0;

	int  m_maxUploadsPerFrame = 4;
	int  m_maxActivatesPerFrame = 2;
	int  m_maxDeactivatesPerFrame = 2;

	WeatherState m_currentWeather;
	float		 m_lastWeatherChangeDays = 0.0f;


	bool         m_enableRandomWeather = true;

	ParticleSystem* m_particleSystem = nullptr;

	ParticleEmitter* m_rainEmitter = nullptr;
	ParticleEmitter* m_snowEmitter = nullptr;

	
public:
	void ProcessDirtyLighting();
	void ProcessNextDirtyLightBlock();
	void MarkLightingDirty(const BlockIterator& it);
	void UndirtyAllBlocksInChunk(Chunk* chunk);
	void MarkLightingDirtyIfNotOpaque(const BlockIterator& it);

private:
	std::deque<BlockIterator> m_dirtyLightBlocks;
	Shader* m_shader = nullptr;
	ConstantBuffer* m_worldCBO = nullptr;
};