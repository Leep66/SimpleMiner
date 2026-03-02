#include "Game/World.hpp"
#include "Game/Chunk.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Game/ChunkCoords.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Game.hpp"
#include "Game/ChunkFileUtils.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Engine/Core/JobSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "ThirdParty/Noise/SmoothNoise.hpp"
#include "Engine/Renderer/ConstantBuffer.hpp"
#include "Game/Player.hpp"
#include "Engine/ParticleSystem/ParticleSystem.hpp"
#include "Engine/ParticleSystem/ParticleEmitter.hpp"



ParticleSystem* g_theParticleSystem = nullptr;

extern DevConsole* g_theDevConsole;

static constexpr float BASE_WORLD_TIME_SCALE = 500.f;
static constexpr float SECONDS_PER_DAY = 60.0f * 60.0f * 24.0f;

static inline int CenterX(const IntVec2& cc) { return (cc.x << CHUNK_BITS_X) + (CHUNK_SIZE_X >> 1); }
static inline int CenterY(const IntVec2& cc) { return (cc.y << CHUNK_BITS_Y) + (CHUNK_SIZE_Y >> 1); }

struct GenerateChunkJob : Job
{
	explicit GenerateChunkJob(Chunk* c) : chunk(c) {}

	void Execute() override 
	{
		if (!chunk) return;
		chunk->m_state = ChunkState::ACTIVATING_GENERATING;
		chunk->GenerateBlocks();

		chunk->m_state = ChunkState::ACTIVATING_GENERATE_COMPLETE;
	}
	Chunk* chunk = nullptr;
};

struct LoadChunkJob : Job 
{
	explicit LoadChunkJob(Chunk* c, std::string path) : chunk(c), filename(std::move(path)) {}

	void Execute() override 
	{
		if (!chunk) return;
		chunk->m_state = ChunkState::ACTIVATING_LOADING;
		const bool ok = ChunkFileUtils::Load(*chunk, filename);
		if (ok) 
		{
			chunk->MarkClean();
			chunk->m_needsSaving = false;
			chunk->m_state = ChunkState::ACTIVATING_LOAD_COMPLETE;
		}
		else 
		{
			chunk->m_state = ChunkState::ACTIVATING_GENERATING;
			chunk->GenerateBlocks();
			chunk->m_state = ChunkState::ACTIVATING_GENERATE_COMPLETE;
		}
	}
	Chunk* chunk = nullptr;
	std::string filename;
};

struct CpuMesh 
{
	std::vector<Vertex_PCU>  verts;
	std::vector<uint32_t>    indices;
	IntVec2                  cc;
	uint32_t                 generation = 0; 
};

struct CompletedCpuMesh { CpuMesh mesh; };

std::mutex                  m_completedMutex;
std::deque<CompletedCpuMesh> m_completedMeshes;

struct MeshBuildJob : Job
{
	MeshBuildJob(Chunk* c, uint32_t gen) : chunk(c), generation(gen) {}

	void Execute() override
	{
		if (!chunk) return;

		ChunkState state = chunk->m_state.load(std::memory_order_acquire);
		if (state == ChunkState::ACTIVATING_GENERATING ||
			state == ChunkState::ACTIVATING_QUEUED_GENERATE) {
			return;
		}

		if (generation != chunk->m_meshGeneration.load(std::memory_order_relaxed))
			return;

		CpuMesh out;
		out.cc = chunk->GetChunkCoords();
		out.generation = generation;

		chunk->AddVertsForChunk();
		chunk->AddVertsForDebug();

		result = std::move(out);
	}



	Chunk* chunk = nullptr;
	uint32_t generation = 0;

	CpuMesh  result;
};

World::World(Game* owner)
	: m_game(owner)
	, m_jobSystem(owner->m_jobSystem)
{
	m_camera = m_game->m_player->GetCameraPtr();
	m_air = BlockDefinition::GetIndexByName("Air");
	m_glowstone = BlockDefinition::GetIndexByName("Glowstone");
	m_cobblestone = BlockDefinition::GetIndexByName("Cobblestone");
	m_chiseledBrick = BlockDefinition::GetIndexByName("ChiseledBrick");
	m_water = BlockDefinition::GetIndexByName("Water");
	m_glowstoneRed = BlockDefinition::GetIndexByName("GlowstoneRed");
	m_glowstoneGreen = BlockDefinition::GetIndexByName("GlowstoneGreen");
	m_glowstoneBlue = BlockDefinition::GetIndexByName("GlowstoneBlue");




	m_worldClock = new Clock(*m_game->m_clock);
	m_shader = g_theRenderer->CreateShader("Data/Shaders/WorldLighting");
	m_worldCBO = g_theRenderer->CreateConstantBuffer(sizeof(WorldConstants));

	m_currentWeather = GetWeatherPreset(WeatherType::SUNNY);
	m_lastWeatherChangeDays = m_worldTimeDays;

	ParticleSystemConfig particleConfig;
	particleConfig.m_game = m_game;
	g_theParticleSystem = new ParticleSystem(particleConfig);
	g_theParticleSystem->m_camera = m_game->m_player->GetCamera();

	m_particleSystem = g_theParticleSystem;

	InitWeatherEmitters();
	UpdateWeatherEmittersForState();
}

World::~World()
{
	if (m_jobSystem)
	{
		m_jobSystem->CancelPendingJobs();
	}
		
	DeactivateAllChunks();
	
	m_activeChunks.clear();
	m_jobSystem = nullptr;

	SAFE_DELETE(m_worldCBO);
	SAFE_DELETE(g_theParticleSystem);
	m_particleSystem = nullptr;
}


void World::Update(float deltaSeconds)
{
	if (g_theParticleSystem)
	{
		g_theParticleSystem->m_camera = m_game->m_player->GetCamera();
	}
	
	UpdateRaycastFromCamera();

	PumpCompletedJobs(8);

	const int MAX_UPLOADS_PER_FRAME = 2;
	const int MAX_ACTIVATIONS_PER_FRAME = 1;
	const int MAX_DEACTIVATIONS_PER_FRAME = 1;

	const IntVec3 camCell = GetGlobalCoords(m_camera->GetPosition());
	const IntVec2 camCC = GetChunkCoords(camCell);
	const int camCX = CenterX(camCC);
	const int camCY = CenterY(camCC);

	const int ACT_RANGE = CHUNK_ACTIVATION_RANGE + 1;
	const int DEA_RANGE = CHUNK_DEACTIVATION_RANGE;
	const int ACTSQUARE = ACT_RANGE * ACT_RANGE;
	const int DEASQUARE = DEA_RANGE * DEA_RANGE;

	{
		int uploads = 0;
		while (uploads < MAX_UPLOADS_PER_FRAME)
		{
			if (!UploadOneMeshIfAvailable())
				break;
			++uploads;
		}
	}

	{
		int activates = 0;
		while ((int)m_activeChunks.size() < MAX_ACTIVE_CHUNKS && activates < MAX_ACTIVATIONS_PER_FRAME)
		{
			IntVec2 best{};
			if (!FindNearestMissingInRange(camCC, camCX, camCY, ACTSQUARE, best))
				break;

			ActivateChunkAt(best);
			++activates;
		}
	}

	{
		int deactivates = 0;
		while (deactivates < MAX_DEACTIVATIONS_PER_FRAME)
		{
			IntVec2 farKey{};
			if (!FindFarthestToDeactivate(camCX, camCY, DEASQUARE, farKey))
				break;

			DeactivateChunkAt(farKey);
			++deactivates;
		}
	}

	ProcessDirtyLighting();
	UpdateFluids(deltaSeconds);
	RebuildDirtyChunksNearCamera(2);


	float realSeconds = m_worldClock->GetDeltaSeconds();

	m_worldTimeDays += realSeconds * (BASE_WORLD_TIME_SCALE / SECONDS_PER_DAY);

	if (m_worldTimeDays > 100000.f)
		m_worldTimeDays = fmodf(m_worldTimeDays, 1.0f);

	UpdateWeather(deltaSeconds);

	UpdateWorldConstants(deltaSeconds);

	if (m_rainEmitter || m_snowEmitter)
	{
		Vec3 camPos = m_game->m_player->GetCamera().GetPosition();
		Vec3 center = camPos + Vec3(0.f, 0.f, 30.f);

		if (m_rainEmitter)
			m_rainEmitter->m_config.position = center;
		if (m_snowEmitter)
			m_snowEmitter->m_config.position = center;
	}

	if (m_rainEmitter)
		m_rainEmitter->Update(deltaSeconds);
	if (m_snowEmitter)
		m_snowEmitter->Update(deltaSeconds);
}






void World::Render() const
{
	g_theRenderer->BindShader(m_shader);
	g_theRenderer->BeginOpaquePass();

	for (auto& kv : m_activeChunks) 
	{
		if (kv.second)
		{
			kv.second->Render();
		}
	}

	for (int i = 0; i < (int)m_game->m_entities.size(); ++i)
	{
		Entity* entity = m_game->m_entities[i];
		if (entity)
		{
			entity->Render();
		}
	}
	

	const GameRaycastResult3D& r = m_currentRaycast;

	if (r.m_rayMaxLength > 0.0f && r.m_rayForwardNormal.GetLengthSquared() > 0.0f)
	{
		DebugAddRaycastResult(
			r,
			0.02f,
			0.0f,
			Rgba8(255, 255, 255, 255),
			Rgba8(255, 255, 255, 255),
			DebugRenderMode::X_RAY);
	}

	if (r.m_didImpact)
	{
		const IntVec3& cell = r.m_hitCell;


		AABB3 box(
			Vec3(float(cell.x), float(cell.y), float(cell.z)),
			Vec3(float(cell.x + 1), float(cell.y + 1), float(cell.z + 1)));

		DebugAddWorldWireAABB3(
			box,
			0.0f,
			Rgba8(255, 255, 255, 255),
			Rgba8(255, 255, 255, 255),
			DebugRenderMode::X_RAY);
	}

	g_theRenderer->BindShader(nullptr);


	if (m_rainEmitter)
		m_rainEmitter->SimpleRender();
	if (m_snowEmitter)
		m_snowEmitter->SimpleRender();


	g_theRenderer->EndOpaquePass();

}



void World::DebugRender() const
{
	for (auto& kv : m_activeChunks) 
	{
		Chunk* c = kv.second;
		if (!c) continue;
		c->DebugRender();
	}
	

}

uint8_t World::CurrentPlaceType() const
{
	switch (m_placeMode % 7)
	{
	case 0: return m_glowstone;
	case 1: return m_cobblestone;
	case 2: return m_chiseledBrick;
	case 3: return m_water;
	case 4: return m_glowstoneRed;
	case 5: return m_glowstoneGreen;
	case 6: return m_glowstoneBlue;
	default: return 0; 
	}
}



std::string World::GetPlaceModeText() const
{
	switch (m_placeMode)
	{
	case 0: return "Glowstone";
	case 1: return "Cobblestone";
	case 2: return "ChiseledBrick";
	case 3: return "Water";
	case 4: return "Glowstone Red";
	case 5: return "Glowstone Green";
	case 6: return "Glowstone Blue";
	default:
		return "Invalid";
		break;
	}
}

void World::DeactivateAllChunks()
{
	while (!m_activeChunks.empty()) 
	{
		auto it = m_activeChunks.begin();
		DeactivateChunkAt(it->first);
	}
	m_dirtyList.clear();

	m_inflightChunks.clear();
	m_queuedForActivate.clear();
	m_queuedForDeactivate.clear(); 
}

static inline int FloorMod(int a, int m)
{
	int r = a % m; return r < 0 ? r + m : r;
}


bool World::DigBlock()
{
	GameRaycastResult3D& r = m_currentRaycast;

	if (!r.m_didImpact)
		return false;

	BlockIterator it = r.m_blockIter;
	if (!it.m_chunk || it.m_blockIdx < 0)
		return false;

	Chunk* c = it.m_chunk;
	const IntVec3 hitCell = r.m_hitCell;

	uint8_t blockType = c->GetTypeAtLocal(it.m_blockIdx);
	if (blockType == m_water)
	{
		int  rootId = -1;
		bool isRoot = false;

		for (const WaterCell& w : m_waterBlocks)
		{
			if (w.pos == hitCell)
			{
				rootId = w.rootId;
				break;
			}
		}

		if (rootId >= 0)
		{
			for (const FluidSource& s : m_waterSources)
			{
				if (s.rootId == rootId && s.isRootSource && s.position == hitCell)
				{
					isRoot = true;
					break;
				}
			}
		}

		if (!isRoot)
		{
			return false;
		}

		StartDrainFromRoot(rootId);

		m_waterBlocks.erase(
			std::remove_if(m_waterBlocks.begin(), m_waterBlocks.end(),
				[hitCell](const WaterCell& w)
				{
					return w.pos == hitCell;
				}),
			m_waterBlocks.end());
	}


	const IntVec2 cc = GetChunkCoords(hitCell);
	const int lx = FloorMod(hitCell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(hitCell.y, CHUNK_SIZE_Y);
	const int lz = hitCell.z;
	if (lz < 0 || lz > CHUNK_MAX_Z)
		return false;

	if (!c->SetTypeAtLocal(lx, ly, lz, m_air))
		return false;

	MarkChunkDirty(c);
	MarkNeighborsDirtyIfNeeded(c, cc, lx, ly, lz);

	const int idx = LocalCoordsToIndex(lx, ly, lz);
	BlockIterator selfIt(c, idx);

	if (lz + 1 <= CHUNK_MAX_Z) {
		int aboveIdx = LocalCoordsToIndex(lx, ly, lz + 1);
		Block& above = c->GetBlockRef(aboveIdx);
		const BlockDefinition& aboveDef = BlockDefinition::GetDefinitionByIndex(above.m_typeIndex);

		if (above.IsSky() && !aboveDef.m_isOpaque) {
			int zDown = lz;
			while (zDown >= 0) {
				int curIdx = LocalCoordsToIndex(lx, ly, zDown);
				Block& curB = c->GetBlockRef(curIdx);
				const BlockDefinition& curDef = BlockDefinition::GetDefinitionByIndex(curB.m_typeIndex);

				if (curDef.m_isOpaque)
					break;

				if (!curB.IsSky()) {
					curB.SetIsSky(true);
					BlockIterator curIt(c, curIdx);
					MarkLightingDirty(curIt);
				}

				--zDown;
			}
		}
	}

	MarkLightingDirty(selfIt);
	MarkLightingDirtyIfNotOpaque(selfIt.GetWestNeighbor());
	MarkLightingDirtyIfNotOpaque(selfIt.GetEastNeighbor());
	MarkLightingDirtyIfNotOpaque(selfIt.GetSouthNeighbor());
	MarkLightingDirtyIfNotOpaque(selfIt.GetNorthNeighbor());
	MarkLightingDirtyIfNotOpaque(selfIt.GetBelowNeighbor());
	MarkLightingDirtyIfNotOpaque(selfIt.GetAboveNeighbor());

	c->m_needsSaving = true;
	return true;
}




bool World::PlaceBlock()
{
	GameRaycastResult3D& r = m_currentRaycast;

	if (!r.m_didImpact)
		return false;

	const IntVec3 hitCell = r.m_hitCell;
	const Vec3 faceNormal = r.m_faceNormal;

	IntVec3 placeCell = hitCell;
	if (faceNormal.x > 0.5f)        placeCell.x += 1;
	else if (faceNormal.x < -0.5f)  placeCell.x -= 1;
	else if (faceNormal.y > 0.5f)   placeCell.y += 1;
	else if (faceNormal.y < -0.5f)  placeCell.y -= 1;
	else if (faceNormal.z > 0.5f)   placeCell.z += 1;
	else if (faceNormal.z < -0.5f)  placeCell.z -= 1;

	if (placeCell.z < 0 || placeCell.z > CHUNK_MAX_Z)
		return false;

	const uint8_t t = CurrentPlaceType();

	const IntVec2 cc = GetChunkCoords(placeCell);
	Chunk* c = FindActiveChunk(cc);
	if (!c)
		return false;

	const int lx = FloorMod(placeCell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(placeCell.y, CHUNK_SIZE_Y);
	const int lz = placeCell.z;

	if (!c->SetTypeAtLocal(lx, ly, lz, t))
		return false;

	if (t == m_water)
	{
		bool alreadyExist = false;
		for (const FluidSource& s : m_waterSources)
		{
			if (s.position == placeCell)
			{
				alreadyExist = true;
				break;
			}
		}

		if (!alreadyExist)
		{
			FluidSource newSource;
			newSource.position = placeCell;
			newSource.remainingSpreads = MAX_WATER_SPREADS;
			newSource.timer = 0.0f;
			newSource.rootId = m_nextWaterRootId++;
			newSource.isRootSource = true;

			m_waterSources.push_back(newSource);
			m_waterBlocks.push_back(WaterCell{ placeCell, newSource.rootId });
		}
	}



	MarkChunkDirty(c);
	MarkNeighborsDirtyIfNeeded(c, cc, lx, ly, lz);

	const int idx = LocalCoordsToIndex(lx, ly, lz);
	Block& placedBlock = c->GetBlockRef(idx);
	const BlockDefinition& newDef = BlockDefinition::GetDefinitionByIndex(placedBlock.m_typeIndex);

	bool wasSky = placedBlock.IsSky();
	bool nowOpaque = newDef.m_isOpaque;

	BlockIterator it(c, idx);

	if (wasSky && nowOpaque) {
		placedBlock.SetIsSky(false);
		MarkLightingDirty(it);

		int zDown = lz - 1;
		while (zDown >= 0) {
			int curIdx = LocalCoordsToIndex(lx, ly, zDown);
			Block& curB = c->GetBlockRef(curIdx);

			if (!curB.IsSky())
				break;

			curB.SetIsSky(false);
			BlockIterator curIt(c, curIdx);
			MarkLightingDirty(curIt);

			--zDown;
		}
	}

	MarkLightingDirty(it);
	MarkLightingDirtyIfNotOpaque(it.GetWestNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetEastNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetSouthNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetNorthNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetBelowNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetAboveNeighbor());

	c->m_needsSaving = true;
	return true;
}




void World::InitLightingForChunk(Chunk* chunk)
{
	if (!chunk) return;

	for (int i = 0; i < BLOCKS_PER_CHUNK; i++) 
	{
		Block& b = chunk->GetBlockRef(i);
		b.SetIndoorLight(0);
		b.SetOutdoorLight(0);
		b.ClearRGBLight();
		b.SetIsSky(false);
		b.SetIsLightDirty(false);
	}

	MarkBoundaryBlocksDirty(chunk);

	for (int x = 0; x < CHUNK_SIZE_X; x++) 
	{
		for (int y = 0; y < CHUNK_SIZE_Y; y++) 
		{
			bool skyVisible = true;

			for (int z = CHUNK_MAX_Z; z >= 0; z--) 
			{
				int idx = LocalCoordsToIndex(x, y, z);
				Block& b = chunk->GetBlockRef(idx);
				const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(b.m_typeIndex);

				if (skyVisible && !def.m_isOpaque)
				{
					b.SetIsSky(true);
				}
				else 
				{
					b.SetIsSky(false);
					if (def.m_isOpaque)
					{
						skyVisible = false;
					}
				}
			}

			skyVisible = true;
			for (int z = CHUNK_MAX_Z; z >= 0; z--) 
			{
				int idx = LocalCoordsToIndex(x, y, z);
				Block& b = chunk->GetBlockRef(idx);
				const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(b.m_typeIndex);

				if (skyVisible && !def.m_isOpaque)
				{
					if (b.IsSky())
					{
						b.SetOutdoorLight(15);

						b.m_outdoorLightRGB = Rgba8(255, 255, 255, 255);

						MarkLightingDirty(BlockIterator(chunk, idx));
						MarkHorizontalNeighborsDirty(chunk, x, y, z);
					}
				}


				if (def.m_isOpaque) 
				{
					skyVisible = false;
				}
			}
		}
	}

	for (int idx = 0; idx < BLOCKS_PER_CHUNK; idx++) 
	{
		const uint8_t type = chunk->GetTypeAtLocal(idx);
		const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(type);

		if (def.m_indoorLighting > 0 || def.m_outdoorLighting > 0) 
		{
			MarkLightingDirty(BlockIterator(chunk, idx));
		}
	}
}

void World::MarkBoundaryBlocksDirty(Chunk* chunk)
{
	const IntVec2& cc = chunk->GetChunkCoords();

	for (int y = 0; y < CHUNK_SIZE_Y; y++)
	{
		for (int z = 0; z < CHUNK_SIZE_Z; z++) 
		{
			int westIdx = LocalCoordsToIndex(0, y, z);
			Block& westBlock = chunk->GetBlockRef(westIdx);
			const BlockDefinition& westDef = BlockDefinition::GetDefinitionByIndex(westBlock.m_typeIndex);
			if (!westDef.m_isOpaque && FindChunkAt(cc + IntVec2(-1, 0)))
			{
				MarkLightingDirty(BlockIterator(chunk, westIdx));
			}

			int eastIdx = LocalCoordsToIndex(CHUNK_MAX_X, y, z);
			Block& eastBlock = chunk->GetBlockRef(eastIdx);
			const BlockDefinition& eastDef = BlockDefinition::GetDefinitionByIndex(eastBlock.m_typeIndex);
			if (!eastDef.m_isOpaque && FindChunkAt(cc + IntVec2(1, 0))) 
			{
				MarkLightingDirty(BlockIterator(chunk, eastIdx));
			}
		}
	}

	for (int x = 0; x < CHUNK_SIZE_X; x++) 
	{
		for (int z = 0; z < CHUNK_SIZE_Z; z++)
		{
			int southIdx = LocalCoordsToIndex(x, 0, z);
			Block& southBlock = chunk->GetBlockRef(southIdx);
			const BlockDefinition& southDef = BlockDefinition::GetDefinitionByIndex(southBlock.m_typeIndex);
			if (!southDef.m_isOpaque && FindChunkAt(cc + IntVec2(0, -1))) 
			{
				MarkLightingDirty(BlockIterator(chunk, southIdx));
			}

			int northIdx = LocalCoordsToIndex(x, CHUNK_MAX_Y, z);
			Block& northBlock = chunk->GetBlockRef(northIdx);
			const BlockDefinition& northDef = BlockDefinition::GetDefinitionByIndex(northBlock.m_typeIndex);
			if (!northDef.m_isOpaque && FindChunkAt(cc + IntVec2(0, 1))) 
			{
				MarkLightingDirty(BlockIterator(chunk, northIdx));
			}
		}
	}
}

void World::MarkHorizontalNeighborsDirty(Chunk* chunk, int x, int y, int z)
{
	auto markIfNonOpaque = [&](int dx, int dy) {
		int nx = x + dx, ny = y + dy;
		if (nx >= 0 && nx < CHUNK_SIZE_X && ny >= 0 && ny < CHUNK_SIZE_Y) {
			int nidx = LocalCoordsToIndex(nx, ny, z);
			Block& nb = chunk->GetBlockRef(nidx);
			const BlockDefinition& nbDef = BlockDefinition::GetDefinitionByIndex(nb.m_typeIndex);
			if (!nbDef.m_isOpaque && !nb.IsSky()) {
				MarkLightingDirty(BlockIterator(chunk, nidx));
			}
		}
		};

	markIfNonOpaque(-1, 0);
	markIfNonOpaque(1, 0); 
	markIfNonOpaque(0, -1);
	markIfNonOpaque(0, 1); 
}


bool World::RaycastVsBlocks(
	const Vec3& start,
	const Vec3& forward,
	float maxDist,
	GameRaycastResult3D& out)
{
	out.Clear();
	out.m_rayStart = start;
	out.m_rayForwardNormal = forward;
	out.m_rayMaxLength = maxDist;

	IntVec3 hitCell;
	Vec3    impactPos;
	Vec3    impactNormal;

	if (!RaycastSolidBlock(start, forward, maxDist, hitCell, impactPos, impactNormal))
		return false;

	out.m_didImpact = true;
	out.m_impactPos = impactPos;
	out.m_impactNormal = impactNormal;
	out.m_impactDist = (impactPos - start).GetLength();
	out.m_hitCell = hitCell;
	out.m_faceNormal = impactNormal;

	const IntVec2 cc = GetChunkCoords(hitCell);
	Chunk* chunk = FindActiveChunk(cc);
	if (!chunk)
		return true; 

	const int lx = FloorMod(hitCell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(hitCell.y, CHUNK_SIZE_Y);
	const int lz = hitCell.z;
	if (lz < 0 || lz > CHUNK_MAX_Z)
		return true;

	const int localIdx = LocalCoordsToIndex(lx, ly, lz);
	out.m_blockIter = BlockIterator(chunk, localIdx);

	return true;
}



void World::UpdateRaycastFromCamera()
{
	if (m_isRaylock)
		return;

	m_currentRaycast.Clear();

	if (!m_camera || !m_game || !m_game->m_player)
		return;

	Player* player = m_game->m_player;

	Vec3 start;
	Vec3 dir;

	if (player->IsInFixedAngleTrackingMode())
	{
		start = player->GetEyePosition();
		dir = player->GetAim().GetForwardNormal();
	}
	else
	{
		start = m_camera->GetPosition();
		dir = m_camera->GetOrientation().GetForwardNormal();
	}

	if (dir.GetLengthSquared() < 1e-6f)
		return;

	dir = dir.GetNormalized();
	const float MAX_DIST = 8.0f;

	RaycastVsBlocks(start, dir, MAX_DIST, m_currentRaycast);
}





void World::ToggleRayLock()
{
	m_isRaylock = !m_isRaylock;

	if (m_isRaylock)
	{
		m_lockedRaycast = m_currentRaycast;
	}
}

bool World::RaycastSolidBlock(const Vec3& start, const Vec3& dir, float maxDist, IntVec3& outCell, Vec3& outImpactPos, Vec3& outNormal) const
{
	Vec3 rayDir = dir;
	if (rayDir.GetLengthSquared() < 1e-6f) 
	{
		return false;
	}
	rayDir = rayDir.GetNormalized();

	IntVec3 cell = GetGlobalCoords(start);

	int stepX = (rayDir.x > 0.f) ? 1 : (rayDir.x < 0.f ? -1 : 0);
	int stepY = (rayDir.y > 0.f) ? 1 : (rayDir.y < 0.f ? -1 : 0);
	int stepZ = (rayDir.z > 0.f) ? 1 : (rayDir.z < 0.f ? -1 : 0);

	const float BIG = 1e30f;
	float tDeltaX = (stepX != 0) ? (1.f / std::abs(rayDir.x)) : BIG;
	float tDeltaY = (stepY != 0) ? (1.f / std::abs(rayDir.y)) : BIG;
	float tDeltaZ = (stepZ != 0) ? (1.f / std::abs(rayDir.z)) : BIG;

	auto nextBoundary = [&](float s, float d, int step) -> float
	{
		if (step > 0) {
			float cellBorder = std::floor(s) + 1.f;
			return (cellBorder - s) / d;
		}
		else if (step < 0) {
			float cellBorder = std::floor(s);
			return (cellBorder - s) / d;
		}
		else {
			return BIG;
		}
	};

	float t = 0.f;
	float tMaxX = nextBoundary(start.x, rayDir.x, stepX);
	float tMaxY = nextBoundary(start.y, rayDir.y, stepY);
	float tMaxZ = nextBoundary(start.z, rayDir.z, stepZ);

	{
		uint8_t type = 0;
		GetBlockTypeAtCell(cell, type);
		if (BlockDefinition::GetDefinitionByIndex(type).m_isOpaque)
		{
			outCell = cell;
			outImpactPos = start;
			outNormal = Vec3::ZERO;
			return true;
		}
	}

	while (t <= maxDist)
	{
		enum HitAxis { AXIS_X, AXIS_Y, AXIS_Z } axis;
		if (tMaxX < tMaxY) {
			if (tMaxX < tMaxZ) axis = AXIS_X;
			else axis = AXIS_Z;
		}
		else {
			if (tMaxY < tMaxZ) axis = AXIS_Y;
			else axis = AXIS_Z;
		}

		if (axis == AXIS_X)
		{
			cell.x += stepX;
			t = tMaxX;
			tMaxX += tDeltaX;
			outNormal = Vec3(-float(stepX), 0.f, 0.f);
		}
		else if (axis == AXIS_Y) 
		{
			cell.y += stepY;
			t = tMaxY;
			tMaxY += tDeltaY;
			outNormal = Vec3(0.f, -float(stepY), 0.f);
		}
		else 
		{
			cell.z += stepZ;
			t = tMaxZ;
			tMaxZ += tDeltaZ;
			outNormal = Vec3(0.f, 0.f, -float(stepZ));
		}

		if (t > maxDist) 
		{
			break;
		}

		uint8_t type = 0;
		GetBlockTypeAtCell(cell, type);
		const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(type);
		if (def.m_isOpaque) 
		{
			outCell = cell;
			outImpactPos = start + rayDir * t;
			return true;
		}
	}

	return false;
}

bool World::GetBlockTypeAtCell(const IntVec3& cell, uint8_t& outType) const
{
	if (cell.z < 0 || cell.z > CHUNK_MAX_Z) {
		outType = m_air;
		return false;
	}

	const IntVec2 cc = GetChunkCoords(cell);
	Chunk* c = const_cast<World*>(this)->FindActiveChunk(cc);
	if (!c) {
		outType = m_air;
		return false;
	}

	const int lx = FloorMod(cell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(cell.y, CHUNK_SIZE_Y);
	const int lz = cell.z;

	outType = c->GetTypeAtLocal(lx, ly, lz);
	return true;
}

bool World::SetBlockTypeAtCell(const IntVec3& cell, uint8_t type)
{
	if (cell.z < 0 || cell.z > CHUNK_MAX_Z) {
		return false;
	}

	const IntVec2 cc = GetChunkCoords(cell);
	Chunk* c = FindActiveChunk(cc);
	if (!c) {
		return false;
	}

	const int lx = FloorMod(cell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(cell.y, CHUNK_SIZE_Y);
	const int lz = cell.z;

	if (!c->SetTypeAtLocal(lx, ly, lz, type)) {
		return false;
	}

	MarkChunkDirty(c);
	MarkNeighborsDirtyIfNeeded(c, cc, lx, ly, lz);

	const int idx = LocalCoordsToIndex(lx, ly, lz);
	BlockIterator it(c, idx);

	MarkLightingDirty(it);
	MarkLightingDirtyIfNotOpaque(it.GetWestNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetEastNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetSouthNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetNorthNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetBelowNeighbor());
	MarkLightingDirtyIfNotOpaque(it.GetAboveNeighbor());

	c->m_needsSaving = true;
	return true;
}

void World::UpdateFluids(float deltaSeconds)
{
	static constexpr float SPREAD_INTERVAL = 0.5f;
	static constexpr float DRAIN_INTERVAL = 0.05f;

	if (!m_drainingWater.empty())
	{
		m_drainTimer += deltaSeconds;
		while (m_drainTimer >= DRAIN_INTERVAL && !m_drainingWater.empty())
		{
			m_drainTimer -= DRAIN_INTERVAL;

			IntVec3 cell = m_drainingWater.front();
			m_drainingWater.erase(m_drainingWater.begin());

			uint8_t t = m_air;
			GetBlockTypeAtCell(cell, t);
			if (t == m_water)
			{
				SetBlockTypeAtCell(cell, m_air);

				auto it = std::find_if(
					m_waterBlocks.begin(), m_waterBlocks.end(),
					[cell](const WaterCell& w)
					{
						return w.pos == cell;
					});

				if (it != m_waterBlocks.end())
				{
					*it = m_waterBlocks.back();
					m_waterBlocks.pop_back();
				}
			}

		}

		if (!m_drainingWater.empty())
			return;
	}

	std::vector<FluidSource> newSources;

	size_t i = 0;
	while (i < m_waterSources.size())
	{
		FluidSource& src = m_waterSources[i];

		if (src.remainingSpreads <= 0)
		{
			m_waterSources[i] = m_waterSources.back();
			m_waterSources.pop_back();
			continue;
		}

		src.timer += deltaSeconds;
		if (src.timer < SPREAD_INTERVAL)
		{
			++i;
			continue;
		}
		src.timer = 0.f;

		bool didSpread = false;

		IntVec3 belowPos = src.position + IntVec3(0, 0, -1);
		uint8_t belowType = m_air;
		bool canCheckBelow = (belowPos.z >= 0);

		if (canCheckBelow)
		{
			GetBlockTypeAtCell(belowPos, belowType);
		}

		if (canCheckBelow && belowType == m_air)
		{
			if (SetBlockTypeAtCell(belowPos, m_water))
			{
				m_waterBlocks.push_back(WaterCell{ belowPos, src.rootId });

				src.position = belowPos;
				src.remainingSpreads--;
				didSpread = true;
			}

			++i;
			continue;
		}


		if (canCheckBelow && belowType == m_water)
		{
			m_waterSources[i] = m_waterSources.back();
			m_waterSources.pop_back();
			continue;
		}

		bool shouldSpreadHorizontally =
			(!canCheckBelow) ||
			(belowType != m_air && belowType != m_water);

		if (shouldSpreadHorizontally)
		{
			static const IntVec3 OFFSETS[4] =
			{
				IntVec3(1, 0, 0),
				IntVec3(-1, 0, 0),
				IntVec3(0, 1, 0),
				IntVec3(0,-1, 0),
			};

			for (int d = 0; d < 4 && src.remainingSpreads > 0; ++d)
			{
				IntVec3 sidePos = src.position + OFFSETS[d];

				uint8_t type = m_air;
				GetBlockTypeAtCell(sidePos, type);

				if (type != m_air)
					continue;

				IntVec3 finalPos = sidePos;
				IntVec3 below = finalPos + IntVec3(0, 0, -1);

				while (below.z >= 0)
				{
					uint8_t t = m_air;
					GetBlockTypeAtCell(below, t);

					if (t == m_air)
					{
						finalPos = below;
						below.z -= 1;
					}
					else break;
				}

				if (SetBlockTypeAtCell(finalPos, m_water))
				{
					m_waterBlocks.push_back(WaterCell{ finalPos, src.rootId });

					FluidSource child;
					child.position = finalPos;
					child.remainingSpreads = src.remainingSpreads - 1;
					child.timer = 0.f;
					child.rootId = src.rootId;
					child.isRootSource = false;

					if (child.remainingSpreads > 0)
						newSources.push_back(child);

					src.remainingSpreads--;
					didSpread = true;
				}
			}

		}

		if (src.isRootSource)
		{
			i++;
			continue; 
		}

		if (!didSpread || src.remainingSpreads <= 0)
		{
			m_waterSources[i] = m_waterSources.back();
			m_waterSources.pop_back();
		}
		else
		{
			i++;
		}

	}

	for (auto& child : newSources)
	{
		m_waterSources.push_back(child);
	}
}







void World::MarkChunkDirty(Chunk* c) 
{
	if (!c) return;
	if (c->m_isActive) 
	{ 
		if (!c->m_inDirtyList) 
		{
			c->MarkDirty();
			m_dirtyList.push_back(c);
			c->m_inDirtyList = true;
		}
	}
}




void World::MarkNeighborsDirtyIfNeeded(Chunk* c, const IntVec2& cc, int lx, int ly, int /*lz*/)
{
	if (!c) return;

	if (lx == 0)             MarkChunkDirty(FindChunkAt(cc + IntVec2(-1, 0)));
	if (lx == CHUNK_MAX_X)   MarkChunkDirty(FindChunkAt(cc + IntVec2(1, 0)));
	if (ly == 0)             MarkChunkDirty(FindChunkAt(cc + IntVec2(0, -1)));
	if (ly == CHUNK_MAX_Y)   MarkChunkDirty(FindChunkAt(cc + IntVec2(0, 1)));
}

int World::GetChunksVertsSize() const 
{
	int sum = 0; 
	for (auto& kv : m_activeChunks)
	{
		if (kv.second)
		{
			sum += (int)kv.second->m_vertices.size();
		}
	}
		
	return sum;
}


int World::GetChunksIndicesSize() const 
{
	int sum = 0;
	for (const auto& kv : m_activeChunks) 
	{
		const Chunk* c = kv.second; 
		if (!c) continue;
		sum += (int)c->GetIndexCount();
	}
	return sum;
}

Chunk* World::FindActiveChunk(const IntVec2& chunkCoords) 
{
	auto it = m_activeChunks.find(chunkCoords);
	if (it == m_activeChunks.end()) return nullptr;
	Chunk* c = it->second;
	return (c && c->m_isActive) ? c : nullptr;
}



Chunk* World::FindChunkAt(const IntVec2& chunkCoords)
{
	auto it = m_activeChunks.find(chunkCoords);
	return (it == m_activeChunks.end()) ? nullptr : it->second;
}

Chunk* World::FindFirstDirtyChunk() const
{
	for (auto& kv : m_activeChunks) 
	{
		Chunk* chunk = kv.second;
		if (chunk && chunk->m_isActive && chunk->m_isDirty)
		{
			return chunk;
		}
	}
	return nullptr;
}

void World::RemoveFromDirtyList(Chunk* c)
{
	for (size_t i = 0; i < m_dirtyList.size(); ++i) 
	{
		if (m_dirtyList[i] == c) 
		{
			m_dirtyList[i] = m_dirtyList.back();
			m_dirtyList.pop_back();
			return;
		}
	}
}

Chunk* World::PickNearestDirty(int camCX, int camCY)
{
	if (m_dirtyList.empty()) return nullptr;
	Chunk* best = nullptr; int bestD2 = INT_MAX;

	size_t i = 0;
	while (i < m_dirtyList.size()) 
	{
		Chunk* c = m_dirtyList[i];
		if (!c || !c->m_isActive || !c->m_isDirty) 
		{
			if (c) c->m_inDirtyList = false;
			m_dirtyList[i] = m_dirtyList.back();
			m_dirtyList.pop_back();
			continue;
		}
		const IntVec2 cc = c->GetChunkCoords();
		const int dx = CenterX(cc) - camCX;
		const int dy = CenterY(cc) - camCY;
		const int d2 = dx * dx + dy * dy;
		if (d2 < bestD2) { bestD2 = d2; best = c; }
		++i;
	}
	return best;
}

bool World::FindNearestMissingInRange(const IntVec2& camCC,
	int /*camCX*/, int /*camCY*/,
	int actSquare, IntVec2& outCC)
{
	const IntVec2 camCtr = GetChunkCenter(camCC);

	float   bestD2 = FLT_MAX;
	IntVec2 bestCC{};

	for (int dy = -CHUNK_ACTIVATION_RADIUS_Y; dy <= CHUNK_ACTIVATION_RADIUS_Y; ++dy) 
	{
		for (int dx = -CHUNK_ACTIVATION_RADIUS_X; dx <= CHUNK_ACTIVATION_RADIUS_X; ++dx) 
		{

			const IntVec2 cc(camCC.x + dx, camCC.y + dy);
			if (m_activeChunks.count(cc) ||
				m_queuedForActivate.count(cc) ||
				m_inflightChunks.count(cc)) 
			{
				continue;
			}

			const IntVec2 ctr = GetChunkCenter(cc);
			const float dxw = float(ctr.x - camCtr.x);
			const float dyw = float(ctr.y - camCtr.y);
			const float d2 = dxw * dxw + dyw * dyw;

			if (d2 > float(actSquare)) continue;

			if (d2 < bestD2)
			{
				bestD2 = d2;
				bestCC = cc;
			}
		}
	}

	if (bestD2 < FLT_MAX) 
	{
		outCC = bestCC;
		return true;
	}
	return false;
}



bool World::FindFarthestToDeactivate(int camCX, int camCY, int deactSquare, IntVec2& outKey)
{
	bool has = false; int worst = -1;
	for (auto& kv : m_activeChunks)
	{
		Chunk* c = kv.second; if (!c) continue;
		const IntVec2 cc = c->GetChunkCoords();
		const int dx = CenterX(cc) - camCX;
		const int dy = CenterY(cc) - camCY;
		const int d2 = dx * dx + dy * dy;
		if (d2 > deactSquare && d2 > worst) { worst = d2; outKey = kv.first; has = true; }
	}
	return has;
}



bool World::FindTopSolidBelowCamera(const IntVec3& camCell,
	Chunk*& outChunk,
	IntVec2& outCC,
	int& outLX, int& outLY, int& outLZ)
{
	outChunk = nullptr;

	const IntVec2 cc = GetChunkCoords(camCell);
	Chunk* c = FindActiveChunk(cc);
	if (!c) return false;

	const int lx0 = FloorMod(camCell.x, CHUNK_SIZE_X);
	const int ly0 = FloorMod(camCell.y, CHUNK_SIZE_Y);
	int lz0 = camCell.z;
	if (lz0 < 0) lz0 = 0;
	if (lz0 > CHUNK_MAX_Z) lz0 = CHUNK_MAX_Z;

	for (int z = lz0; z >= 0; --z) 
	{
		if (c->GetTypeAtLocal(lx0, ly0, z) != m_air) 
		{
			outChunk = c; outCC = cc;
			outLX = lx0; outLY = ly0; outLZ = z;
			return true;
		}
	}
	return false; 
}




static inline bool IsNeighbor(const IntVec2& a, const IntVec2& b, int dx, int dy)
{
	return (b.x == a.x + dx) && (b.y == a.y + dy);
}

static void LinkNeighborsFor(Chunk* c, std::map<IntVec2, Chunk*>& active)
{
	const IntVec2& cc = c->GetChunkCoords();
	auto lk = [&](const IntVec2& k)->Chunk* 
	{
		auto it = active.find(k);
		return it == active.end() ? nullptr : it->second;
	};
	Chunk* e = lk(cc + IntVec2(+1, 0));
	Chunk* w = lk(cc + IntVec2(-1, 0));
	Chunk* n = lk(cc + IntVec2(0, +1));
	Chunk* s = lk(cc + IntVec2(0, -1));

	if (e) { c->m_east = e; e->m_west = c; }
	if (w) { c->m_west = w; w->m_east = c; }
	if (n) { c->m_north = n; n->m_south = c; }
	if (s) { c->m_south = s; s->m_north = c; }
}


static inline void UnlinkNeighborsFor(Chunk* c) noexcept
{
	if (!c) return;

	auto unlink = [c](Chunk*& n, Chunk* Chunk::* back) 
	{
		if (!n) return;
		if ((n->*back) == c) 
		{ 
			(n->*back) = nullptr;
		}
		n = nullptr;
	};

	unlink(c->m_east, &Chunk::m_west);
	unlink(c->m_west, &Chunk::m_east);
	unlink(c->m_north, &Chunk::m_south);
	unlink(c->m_south, &Chunk::m_north);
	unlink(c->m_above, &Chunk::m_below);
	unlink(c->m_below, &Chunk::m_above);
}


void World::ActivateChunkAt(const IntVec2& cc)
{
	if (FindActiveChunk(cc)) return;

	auto u = std::make_unique<Chunk>(cc, this);
	u->m_blocks.resize(BLOCKS_PER_CHUNK);
	Chunk* raw = u.get();

	m_queuedForActivate.insert(cc);
	m_inflightChunks.emplace(cc, std::move(u)); 

/*
	if (!m_jobSystem) {
		if (!LoadChunkFromDisk(*raw)) {
			raw->GenerateBlocks();
			raw->m_needsSaving = false;
		}
		else {
			raw->m_needsSaving = false;
		}
		raw->m_isActive = true;
		m_activeChunks[cc] = raw;
		LinkNeighborsFor(raw, m_activeChunks);
		MarkChunkDirty(raw);
		m_queuedForActivate.erase(cc);
		m_inflightChunks.erase(cc);
		return;
	}*/

	const std::string fn = ChunkFileUtils::BuildChunkFilename(*raw);
	if (FileExist(fn)) 
	{
		raw->m_state.store(ChunkState::ACTIVATING_QUEUED_LOAD, std::memory_order_release);
		m_jobSystem->Enqueue(new LoadChunkJob(raw, fn));
	}
	else 
	{
		raw->m_state.store(ChunkState::ACTIVATING_QUEUED_GENERATE, std::memory_order_release);
		m_jobSystem->Enqueue(new GenerateChunkJob(raw));
	}
}

void World::DeactivateChunkAt(const IntVec2& cc)
{
	auto it = m_activeChunks.find(cc);
	if (it == m_activeChunks.end()) return;

	Chunk* c = it->second;
	if (!c) {
		m_activeChunks.erase(it);
		return;
	}

	UnlinkNeighborsFor(c);
	m_activeChunks.erase(it);

	if (c->m_inDirtyList)
	{
		RemoveAllFromDirtyList(c);
	}

	UndirtyAllBlocksInChunk(c);

	if (c->m_needsSaving)
	{
		SaveChunkToDisk(*c);
	}

	c->Deactivate();
	delete c;
}

void World::RemoveAllFromDirtyList(Chunk* c)
{
	size_t i = 0;
	while (i < m_dirtyList.size()) 
	{
		if (m_dirtyList[i] == c) {
			m_dirtyList[i] = m_dirtyList.back();
			m_dirtyList.pop_back();
		}
		else {
			++i;
		}
	}
	c->m_inDirtyList = false;
}


bool World::LoadChunkFromDisk(Chunk& chunk)
{
	const std::string filename = ChunkFileUtils::BuildChunkFilename(chunk);

	if (!FileExist(filename)) {
		return false;
	}

	const bool ok = ChunkFileUtils::Load(chunk, filename);
	if (ok) 
	{
		chunk.MarkClean();        
		chunk.m_needsSaving = false; 
	}
	return ok;
}

void World::SaveChunkToDisk(Chunk& chunk)
{
	if (!chunk.m_needsSaving)
	{
		return;
	}

	const std::string filename = ChunkFileUtils::BuildChunkFilename(chunk);

	const bool ok = ChunkFileUtils::Save(chunk, filename);
	if (ok)
	{
		chunk.m_needsSaving = false; 
	}
	else 
	{
		
		g_theDevConsole->AddLine(DevConsole::WARNING, Stringf("Failed to save chunk: %s", filename.c_str()));
	}
}


void World::GenerateBlocksForChunk(Chunk& chunk)
{
	chunk.GenerateBlocks();
}


bool World::HasAll4NeighborsActive(Chunk* c) const
{
	if (!c) return false;

	Chunk* e = c->m_east;
	Chunk* w = c->m_west;
	Chunk* n = c->m_north;
	Chunk* s = c->m_south;

	auto isActive = [](Chunk* ch)
	{
		return ch != nullptr && ch->m_isActive;
	};

	return isActive(e) && isActive(w) && isActive(n) && isActive(s);
}


void World::OnChunkActive(Chunk* c)
{
	if (!c) return;
	c->m_isActive = true;

	LinkNeighborsFor(c, m_activeChunks);
	InitLightingForChunk(c);

	MarkChunkDirty(c);
	if (c->m_east)  MarkChunkDirty(c->m_east);
	if (c->m_west)  MarkChunkDirty(c->m_west);
	if (c->m_north) MarkChunkDirty(c->m_north);
	if (c->m_south) MarkChunkDirty(c->m_south);
}
void World::DebugPrintBlockLightingAtCell(const IntVec3& cell)
{
	uint8_t type = 0;
	if (!GetBlockTypeAtCell(cell, type))
	{
		g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
			Stringf("Cell (%d,%d,%d) is out of world or air (type=%d)",
				cell.x, cell.y, cell.z, (int)type));
		return;
	}

	const IntVec2 cc = GetChunkCoords(cell);
	Chunk* c = FindActiveChunk(cc);
	if (!c)
	{
		g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
			Stringf("Chunk (%d,%d) not active", cc.x, cc.y));
		return;
	}

	const int lx = FloorMod(cell.x, CHUNK_SIZE_X);
	const int ly = FloorMod(cell.y, CHUNK_SIZE_Y);
	const int lz = cell.z;
	const int idx = LocalCoordsToIndex(lx, ly, lz);

	Block& b = c->GetBlockRef(idx);

	const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(b.m_typeIndex);

	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
		Stringf("Cell (%d,%d,%d): type=%d (%s), in=%d, out=%d, sky=%d, opaque=%d",
			cell.x, cell.y, cell.z,
			(int)b.m_typeIndex, def.m_name.c_str(),
			(int)b.GetIndoorLight(),
			(int)b.GetOutdoorLight(),
			b.IsSky() ? 1 : 0,
			def.m_isOpaque ? 1 : 0));
}

void World::UpdateWorldConstants(float /*deltaSeconds*/)
{
	float dayFactor = GetDayFactor();

	Vec4 nightSky = Vec4(0.10f, 0.12f, 0.18f, 1.f);
	Vec4 daySky = Vec4(0.55f, 0.70f, 0.95f, 1.f);

	Vec4 nightOutdoor = Vec4(0.15f, 0.18f, 0.25f, 1.f);
	Vec4 dayOutdoor = Vec4(1.00f, 1.00f, 1.00f, 1.f);

	Vec4 skyColor = Lerp(nightSky, daySky, dayFactor);
	Vec4 outdoorColor = Lerp(nightOutdoor, dayOutdoor, dayFactor);

	switch (m_currentWeather.m_type)
	{
	case WeatherType::SUNNY: break;
	case WeatherType::CLOUDY:
		skyColor *= 0.90f;
		outdoorColor *= 0.90f;
		break;
	case WeatherType::RAIN:
		skyColor *= 0.75f;
		outdoorColor *= 0.75f;
		break;
	case WeatherType::STORM:
		skyColor *= 0.60f;
		outdoorColor *= 0.60f;
		break;
	case WeatherType::SNOW:
		skyColor *= 1.15f;
		outdoorColor *= 1.10f;
		break;
	}

	skyColor *= m_currentWeather.m_ambientScale;
	outdoorColor *= m_currentWeather.m_ambientScale;

	float lightningStrength = 0.f;
	if (m_currentWeather.m_stormIntensity > 0.001f)
	{
		float stormT = m_worldTimeDays * 6.f;
		float flickerT = m_worldTimeDays * 500.f;

		float stormPerlin = Compute1dPerlinNoise(stormT, 1.f, 3, 0.5f, 2.f, true, 999u);
		float flickerPerlin = Compute1dPerlinNoise(flickerT, 1.f, 5, 0.5f, 2.f, true, 998u);

		float storm01 = stormPerlin * 0.5f + 0.5f;
		float stormMask = RangeMapClamped(storm01, 0.5f, 1.f, 0.f, 1.f);

		float flicker01 = flickerPerlin * 0.5f + 0.5f;
		float flickerStrength = RangeMapClamped(flicker01, 0.4f, 1.f, 0.f, 1.f);
		flickerStrength = flickerStrength * flickerStrength * flickerStrength;

		lightningStrength = stormMask * flickerStrength * m_currentWeather.m_stormIntensity;
	}

	Vec4 white(1, 1, 1, 1);
	Vec4 skyColorWithLightning = Lerp(skyColor, white, lightningStrength);
	Vec4 outdoorWithLightningColor = Lerp(outdoorColor, white, lightningStrength);

	Vec4 indoorColor = Rgba8(255, 230, 204, 255).ToVec4();
	float glowT = m_worldTimeDays * 100.f;
	float glowPerlin = Compute1dPerlinNoise(glowT, 1.f, 9, 0.5f, 2.f, true, 997u);
	float glowStrength = RangeMapClamped(glowPerlin, -1.f, 1.f, 0.9f, 1.05f);
	indoorColor *= glowStrength;

	WorldConstants worldData;

	worldData.IndoorLightColor = indoorColor;
	worldData.OutdoorLightColor = outdoorWithLightningColor;
	worldData.SkyColor = skyColorWithLightning;

	worldData.FogNearDistance = m_currentWeather.m_fogNear;
	worldData.FogFarDistance = m_currentWeather.m_fogFar;

	Vec3 sunRGB = Vec3(outdoorWithLightningColor.x,
		outdoorWithLightningColor.y,
		outdoorWithLightningColor.z);
	float envLum = DotProduct3D(sunRGB, Vec3(0.3333f, 0.3333f, 0.3333f));
	envLum = GetClamped(envLum, 0.0f, 1.0f);

	float minNight = 0.12f;
	worldData.GlobalLightScalar = RangeMapClamped(envLum, 0.f, 1.f, minNight, 1.f);

	m_currentSkyColor = skyColorWithLightning;

	g_theRenderer->CopyCPUToGPU(&worldData, sizeof(WorldConstants), m_worldCBO);
	g_theRenderer->BindConstantBuffer(4, m_worldCBO);
}






void World::UpdateWeather(float /*deltaSeconds*/)
{
	if (m_worldTimeDays - m_lastWeatherChangeDays >= 0.5f)
	{
		RandomizeNextWeather();
		m_lastWeatherChangeDays = m_worldTimeDays;
	}
}

WeatherState World::GetWeatherPreset(WeatherType type) const
{
	WeatherState s;
	s.m_type = type;

	const float dayFactor = GetDayFactor();

	const float baseFogNear = 200.0f;
	const float baseFogFar = 230.0f;

	switch (type)
	{
	case WeatherType::SUNNY:
		s.m_ambientScale = Interpolate(0.3f, 1.0f, dayFactor);
		s.m_fogNear = baseFogNear;
		s.m_fogFar = baseFogFar;
		s.m_rainRate = 0.0f;
		s.m_snowRate = 0.0f;
		s.m_stormIntensity = 0.0f;
		break;

	case WeatherType::CLOUDY:
		s.m_ambientScale = Interpolate(0.25f, 0.8f, dayFactor);
		s.m_fogNear = baseFogNear - 20.0f;
		s.m_fogFar = baseFogFar - 20.0f;
		s.m_rainRate = 0.0f;
		s.m_snowRate = 0.0f;
		s.m_stormIntensity = 0.0f;
		break;

	case WeatherType::RAIN:
		s.m_ambientScale = Interpolate(0.2f, 0.7f, dayFactor);
		s.m_fogNear = baseFogNear - 40.0f;
		s.m_fogFar = baseFogFar - 40.0f;
		s.m_rainRate = 1.0f;
		s.m_snowRate = 0.0f;
		s.m_stormIntensity = 0.3f;
		break;

	case WeatherType::STORM:
		s.m_ambientScale = Interpolate(0.15f, 0.5f, dayFactor);
		s.m_fogNear = baseFogNear - 60.0f;
		s.m_fogFar = baseFogFar - 60.0f;
		s.m_rainRate = 2.0f;
		s.m_snowRate = 0.0f;
		s.m_stormIntensity = 1.0f; 
		break;

	case WeatherType::SNOW:
		s.m_ambientScale = Interpolate(0.25f, 0.9f, dayFactor);
		s.m_fogNear = baseFogNear - 30.0f;
		s.m_fogFar = baseFogFar - 30.0f;
		s.m_rainRate = 0.0f;
		s.m_snowRate = 1.0f;
		s.m_stormIntensity = 0.0f;
		break;
	}

	return s;
}


float World::GetDayFactor() const
{
	float dayFrac = m_worldTimeDays - floorf(m_worldTimeDays);

	if (dayFrac < 0.25f)
	{
		return RangeMapClamped(dayFrac, 0.0f, 0.25f, 0.0f, 1.0f);
	}
	else if (dayFrac < 0.75f)
	{
		return 1.0f;
	}
	else
	{
		return RangeMapClamped(dayFrac, 0.75f, 1.0f, 1.0f, 0.0f);
	}
}





void World::SetWeather(WeatherType type)
{
	m_currentWeather = GetWeatherPreset(type);
	UpdateWeatherEmittersForState();
}


void World::RandomizeNextWeather()
{
	RandomNumberGenerator rng;
	float r = rng.RollRandomFloatZeroToOne();
	WeatherType type = WeatherType::SUNNY;

	if (r < 0.3f) type = WeatherType::SUNNY;
	else if (r < 0.5f) type = WeatherType::CLOUDY;
	else if (r < 0.7f) type = WeatherType::RAIN;
	else if (r < 0.9f) type = WeatherType::SNOW;
	else type = WeatherType::STORM;

	SetWeather(type);
	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
		Stringf("Weather changed to: %s", GetWeatherText().c_str()));
}


void World::InitWeatherEmitters()
{
	if (!m_particleSystem)
		return;


	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = m_particleSystem;
		cfg.name = "WorldRain";

		cfg.position = Vec3(-50.f, -50.f, 100.f);
		cfg.spawnArea = Vec3(80.f, 80.f, 5.f);

		cfg.mainStage.texPath = "Data/Images/rainwater.png";
		cfg.blendMode = BlendMode::ALPHA;
		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, -30.f);
		cfg.mainStage.velocityVariance = Vec3(2.f, 2.f, 5.f);

		cfg.mainStage.lifetime = 100.0f;
		cfg.mainStage.lifetimeVariance = 0.f;

		cfg.mainStage.startColor = Rgba8(180, 200, 255, 220);
		cfg.mainStage.endColor = Rgba8(200, 220, 255, 180);

		cfg.mainStage.startSize = 0.2f;
		cfg.mainStage.endSize = 0.2f;
		cfg.mainStage.billboardType = 0u;

		cfg.spawnRate = 1000.f;
		cfg.maxParticles = 200000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.useSubStage = false;

		m_rainEmitter = m_particleSystem->CreateEmitter(cfg);
	}

	{
		ParticleEmitterConfig cfg;
		cfg.m_owner = m_particleSystem;
		cfg.name = "WorldSnow";

		cfg.position = Vec3(-50.f, -50.f, 100.f);
		cfg.spawnArea = Vec3(80.f, 80.f, 5.f);

		cfg.mainStage.texPath = "Data/Images/snow.png";
		cfg.blendMode = BlendMode::ALPHA;

		cfg.mainStage.baseVelocity = Vec3(0.f, 0.f, -10.f);
		cfg.mainStage.velocityVariance = Vec3(1.5f, 1.5f, 1.5f);

		cfg.mainStage.lifetime = 100.0f;
		cfg.mainStage.lifetimeVariance = 0.f;

		cfg.mainStage.startColor = Rgba8(240, 245, 255, 200);
		cfg.mainStage.endColor = Rgba8(250, 250, 255, 150);

		cfg.mainStage.startSize = 0.3f;
		cfg.mainStage.endSize = 0.3f;
		cfg.mainStage.billboardType = 0u;

		cfg.spawnRate = 1000.f;
		cfg.maxParticles = 100000;
		cfg.isLooping = true;
		cfg.enabled = true;
		cfg.duration = -1.0f;

		cfg.useSubStage = false;

		m_snowEmitter = m_particleSystem->CreateEmitter(cfg);
	}
}

void World::UpdateWeatherEmittersForState()
{
	if (!m_particleSystem)
		return;

	bool wantRain = (m_currentWeather.m_rainRate > 0.01f);
	bool wantSnow = (m_currentWeather.m_snowRate > 0.01f);

	const float baseRainSpawnRate = 2000.f;
	const float baseSnowSpawnRate = 2000.f;

	if (m_rainEmitter)
	{
		m_rainEmitter->m_config.enabled = wantRain;
		m_rainEmitter->m_config.spawnRate = baseRainSpawnRate * m_currentWeather.m_rainRate;
	}

	if (m_snowEmitter)
	{
		m_snowEmitter->m_config.enabled = wantSnow;
		m_snowEmitter->m_config.spawnRate = baseSnowSpawnRate * m_currentWeather.m_snowRate;

	}
}


std::string World::GetWeatherText() const
{
	switch (m_currentWeather.m_type)
	{
	case WeatherType::SUNNY:
		return "Sunny";
	case WeatherType::CLOUDY:
		return "Cloudy";
	case WeatherType::RAIN:
		return "Rain";
	case WeatherType::SNOW:
		return "Snow";
	case WeatherType::STORM:
		return "Storm";
	default:
		return "Invalid";
	}
}

std::string World::GetCurrentTimeText() const
{
	float totalDays = m_worldTimeDays;

	int dayIndex = (int)floorf(totalDays) + 1;
	float dayFrac = totalDays - floorf(totalDays); 

	float totalSeconds = dayFrac * SECONDS_PER_DAY; 
	if (totalSeconds < 0.f) totalSeconds = 0.f;

	int hour = (int)(totalSeconds / 3600.0f);
	int minute = (int)((totalSeconds - hour * 3600.0f) / 60.0f);

	if (hour < 0)   hour = 0;
	if (hour > 23)  hour = 23;
	if (minute < 0)   minute = 0;
	if (minute > 59)  minute = 59;

	return Stringf("Day %d  %02d:%02d", dayIndex, hour, minute);
}


void World::StartDrainFromRoot(int rootId)
{
	if (!m_drainingWater.empty())
		return;

	for (const WaterCell& w : m_waterBlocks)
	{
		if (w.rootId == rootId)
		{
			m_drainingWater.push_back(w.pos);
		}
	}

	size_t i = 0;
	while (i < m_waterSources.size())
	{
		if (m_waterSources[i].rootId == rootId)
		{
			m_waterSources[i] = m_waterSources.back();
			m_waterSources.pop_back();
		}
		else
		{
			++i;
		}
	}

	m_drainTimer = 0.0f;
}

void World::StartDrainFrom(const IntVec3& startCell)
{
	if (!m_drainingWater.empty())
		return;

	int rootId = -1;
	for (const WaterCell& w : m_waterBlocks)
	{
		if (w.pos == startCell)
		{
			rootId = w.rootId;
			break;
		}
	}

	if (rootId < 0)
		return;

	StartDrainFromRoot(rootId);
}


void World::DebugPrintBlockLightingInFrontOfPlayer()
{
	const Vec3 start = m_camera->GetPosition();
	const Vec3 dir = m_camera->GetOrientation().GetForwardNormal();

	IntVec3 hitCell;
	Vec3 impactPos;
	Vec3 impactNormal;
	const float MAX_DIST = 20.0f; 

	if (!RaycastSolidBlock(start, dir, MAX_DIST, hitCell, impactPos, impactNormal))
	{
		g_theDevConsole->AddLine(DevConsole::INFO_MAJOR,
			"DebugLight: no solid block hit by ray");
		return;
	}

	DebugPrintBlockLightingAtCell(hitCell);
}


void World::PumpCompletedJobs(size_t maxPerFrame)
{
	if (!m_jobSystem) return;

	std::vector<Job*> done;
	m_jobSystem->RetrieveCompleted(done, maxPerFrame);

	for (Job* j : done)
	{
		if (auto* gj = dynamic_cast<GenerateChunkJob*>(j))
		{
			Chunk* c = gj->chunk;
			if (c)
			{
				c->m_state.store(ChunkState::ACTIVATING_GENERATE_COMPLETE, std::memory_order_release);
				c->m_meshGeneration.fetch_add(1, std::memory_order_acq_rel);
				m_jobSystem->Enqueue(new MeshBuildJob(c, c->m_meshGeneration.load(std::memory_order_relaxed)));
			}
		}
		else if (auto* lj = dynamic_cast<LoadChunkJob*>(j)) 
		{
			Chunk* c = lj->chunk;
			if (c) 
			{
				c->m_state.store(ChunkState::ACTIVATING_LOAD_COMPLETE, std::memory_order_release);
				c->m_meshGeneration.fetch_add(1, std::memory_order_acq_rel);
				m_jobSystem->Enqueue(new MeshBuildJob(c, c->m_meshGeneration.load(std::memory_order_relaxed)));
			}
		}
		else if (auto* mj = dynamic_cast<MeshBuildJob*>(j)) 
		{
			std::lock_guard<std::mutex> g(m_completedMutex);
			m_completedMeshes.push_back(CompletedCpuMesh{ std::move(mj->result) });
		}

		delete j;
	}
}

bool World::UploadOneMeshIfAvailable() 
{
	CompletedCpuMesh cm;
	{
		std::lock_guard<std::mutex> g(m_completedMutex);
		if (m_completedMeshes.empty()) return false;
		cm = std::move(m_completedMeshes.front());
		m_completedMeshes.pop_front();
	}

	if (!m_queuedForActivate.count(cm.mesh.cc)) 
	{
		auto itActive = m_activeChunks.find(cm.mesh.cc);
		if (itActive == m_activeChunks.end()) return true;
		Chunk* c = itActive->second;
		if (cm.mesh.generation != c->m_meshGeneration.load(std::memory_order_relaxed)) return true;
		c->UpdateBuffersForChunk();
		c->UpdateDebugBuffersForChunk();
		c->MarkClean();
		return true;
	}

	auto itInflight = m_inflightChunks.find(cm.mesh.cc);
	if (itInflight == m_inflightChunks.end()) 
	{
		m_queuedForActivate.erase(cm.mesh.cc);
		return true;
	}

	Chunk* c = itInflight->second.get();
	if (cm.mesh.generation != c->m_meshGeneration.load(std::memory_order_relaxed)) return true;

	c->UpdateBuffersForChunk();
	c->UpdateDebugBuffersForChunk();
	c->MarkClean();

	Chunk* promoted = itInflight->second.release();
	m_inflightChunks.erase(itInflight);

	m_activeChunks.emplace(cm.mesh.cc, promoted);
	LinkNeighborsFor(promoted, m_activeChunks);
	OnChunkActive(promoted);
	promoted->m_state.store(ChunkState::ACTIVE, std::memory_order_release);

	m_queuedForActivate.erase(cm.mesh.cc);
	return true;
}

void World::RebuildChunkMesh(Chunk* c)
{
	if (!c || !c->m_isActive) return;

	c->AddVertsForChunk();
	c->UpdateBuffersForChunk();
	c->MarkClean();
	c->m_inDirtyList = false;
}

void World::RebuildDirtyChunksNearCamera(int maxPerFrame)
{
	if (m_dirtyList.empty())
		return;

	const IntVec3 camCell = GetGlobalCoords(m_camera->GetPosition());
	const IntVec2 camCC = GetChunkCoords(camCell);
	const int camCX = CenterX(camCC);
	const int camCY = CenterY(camCC);

	int rebuilt = 0;
	while (rebuilt < maxPerFrame)
	{
		Chunk* c = PickNearestDirty(camCX, camCY);
		if (!c)
			break;

		c->AddVertsForChunk();
		c->UpdateBuffersForChunk();
		c->MarkClean();
		c->m_isRenderable = true;
		++rebuilt;
	}
}


void World::MarkLightingDirty(const BlockIterator& it)
{
	if (!it.m_chunk || it.m_blockIdx < 0) return;

	Block& b = it.m_chunk->GetBlockRef(it.m_blockIdx);
	if (b.IsLightDirty()) {
		return;
	}

	b.SetIsLightDirty(true);
	m_dirtyLightBlocks.push_back(it);
}

void World::ProcessDirtyLighting()
{
	while (!m_dirtyLightBlocks.empty())
	{
		ProcessNextDirtyLightBlock();
	}
}

void World::ProcessNextDirtyLightBlock()
{
	if (m_dirtyLightBlocks.empty()) return;

	BlockIterator it = m_dirtyLightBlocks.front();
	m_dirtyLightBlocks.pop_front();
	if (!it.m_chunk || it.m_blockIdx < 0) return;

	Block& b = it.m_chunk->GetBlockRef(it.m_blockIdx);
	b.SetIsLightDirty(false);

	const BlockDefinition& def = BlockDefinition::GetDefinitionByIndex(b.m_typeIndex);
	bool opaque = def.m_isOpaque;

	uint8_t outdoorLighting = 0;
	uint8_t indoorLighting = 0;

	if (b.IsSky())
	{
		outdoorLighting = 15;
	}

	if (def.m_outdoorLighting > 0)
	{
		outdoorLighting = Max(outdoorLighting, (uint8_t)def.m_outdoorLighting);
	}
	if (def.m_indoorLighting > 0)
	{
		indoorLighting = Max(indoorLighting, (uint8_t)def.m_indoorLighting);
	}

	auto ColorToVec3 = [](const Rgba8& c) -> Vec3
		{
			return Vec3(
				(float)c.r / 255.0f,
				(float)c.g / 255.0f,
				(float)c.b / 255.0f);
		};

	auto Vec3ToColor = [](const Vec3& v) -> Rgba8
		{
			float r = GetClamped(v.x, 0.0f, 1.0f) * 255.0f;
			float g = GetClamped(v.y, 0.0f, 1.0f) * 255.0f;
			float b = GetClamped(v.z, 0.0f, 1.0f) * 255.0f;
			return Rgba8((uint8_t)r, (uint8_t)g, (uint8_t)b, 255);
		};

	auto MaxVec3 = [](const Vec3& a, const Vec3& b) -> Vec3
		{
			return Vec3(
				Max(a.x, b.x),
				Max(a.y, b.y),
				Max(a.z, b.z));
		};

	Vec3 outdoorRGB(0.0f, 0.0f, 0.0f);
	Vec3 indoorRGB(0.0f, 0.0f, 0.0f);

	if (b.IsSky())
	{
		uint8_t skyI = 15;
		outdoorLighting = Max(outdoorLighting, skyI);
		float skyFactor = (float)skyI / 15.0f;
		outdoorRGB = MaxVec3(outdoorRGB, Vec3(skyFactor, skyFactor, skyFactor));
	}

	if (def.m_outdoorLighting > 0)
	{
		float f = (float)def.m_outdoorLighting / 15.0f;
		Vec3 e = ColorToVec3(def.m_emissiveColor);
		outdoorRGB = MaxVec3(outdoorRGB, e * f);
	}

	if (def.m_indoorLighting > 0)
	{
		float f = (float)def.m_indoorLighting / 15.0f;
		Vec3 e = ColorToVec3(def.m_emissiveColor);
		indoorRGB = MaxVec3(indoorRGB, e * f);
	}

	if (!opaque)
	{
		auto getNeighborScalarAndColor = [&](const BlockIterator& nbIt)
			-> std::pair<std::pair<uint8_t, uint8_t>, std::pair<Vec3, Vec3>>
			{
				if (!nbIt.m_chunk || nbIt.m_blockIdx < 0)
					return { { (uint8_t)0, (uint8_t)0 }, { Vec3::ZERO, Vec3::ZERO } };

				const Block& nb = nbIt.m_chunk->GetBlockRef(nbIt.m_blockIdx);
				const BlockDefinition& nbDef = BlockDefinition::GetDefinitionByIndex(nb.m_typeIndex);

				uint8_t nbOut = 0;
				uint8_t nbIn = 0;
				Vec3 nbOutRGB(0.0f, 0.0f, 0.0f);
				Vec3 nbInRGB(0.0f, 0.0f, 0.0f);

				if (nbDef.m_isOpaque)
				{
					nbOut = (uint8_t)nbDef.m_outdoorLighting;
					nbIn = (uint8_t)nbDef.m_indoorLighting;

					Vec3 e = ColorToVec3(nbDef.m_emissiveColor);
					if (nbOut > 0)
					{
						float f = (float)nbOut / 15.0f;
						nbOutRGB = e * f;
					}
					if (nbIn > 0)
					{
						float f = (float)nbIn / 15.0f;
						nbInRGB = e * f;
					}
				}
				else
				{
					nbOut = nb.GetOutdoorLight();
					nbIn = nb.GetIndoorLight();
					nbOutRGB = ColorToVec3(nb.m_outdoorLightRGB);
					nbInRGB = ColorToVec3(nb.m_indoorLightRGB);
				}

				return { { nbOut, nbIn }, { nbOutRGB, nbInRGB } };
			};

		BlockIterator neighbors[6] =
		{
			it.GetWestNeighbor(),
			it.GetEastNeighbor(),
			it.GetSouthNeighbor(),
			it.GetNorthNeighbor(),
			it.GetBelowNeighbor(),
			it.GetAboveNeighbor()
		};

		for (int i = 0; i < 6; ++i)
		{
			auto info = getNeighborScalarAndColor(neighbors[i]);
			uint8_t nbOut = info.first.first;
			uint8_t nbIn = info.first.second;
			Vec3 nbOutRGB = info.second.first;
			Vec3 nbInRGB = info.second.second;

			if (nbOut > 1)
			{
				uint8_t cand = nbOut - 1;
				if (cand > outdoorLighting)
				{
					outdoorLighting = cand;
				}

				float srcFactor = (float)nbOut / 15.0f;
				if (srcFactor > 0.0f)
				{
					float dstFactor = (float)cand / 15.0f;
					float scale = dstFactor / srcFactor;
					Vec3 candRGB = nbOutRGB * scale;
					outdoorRGB = MaxVec3(outdoorRGB, candRGB);
				}
			}

			if (nbIn > 1)
			{
				uint8_t cand = nbIn - 1;
				if (cand > indoorLighting)
				{
					indoorLighting = cand;
				}

				float srcFactor = (float)nbIn / 15.0f;
				if (srcFactor > 0.0f)
				{
					float dstFactor = (float)cand / 15.0f;
					float scale = dstFactor / srcFactor;
					Vec3 candRGB = nbInRGB * scale;
					indoorRGB = MaxVec3(indoorRGB, candRGB);
				}
			}
		}
	}

	uint8_t currentOutdoor = b.GetOutdoorLight();
	uint8_t currentIndoor = b.GetIndoorLight();

	Rgba8 newOutdoorRGB8 = Vec3ToColor(outdoorRGB);
	Rgba8 newIndoorRGB8 = Vec3ToColor(indoorRGB);

	bool changed =
		(outdoorLighting != currentOutdoor) ||
		(indoorLighting != currentIndoor) ||
		(b.m_outdoorLightRGB != newOutdoorRGB8) ||
		(b.m_indoorLightRGB != newIndoorRGB8);

	if (!changed)
	{
		return;
	}

	b.SetOutdoorLight(outdoorLighting);
	b.SetIndoorLight(indoorLighting);
	b.m_outdoorLightRGB = newOutdoorRGB8;
	b.m_indoorLightRGB = newIndoorRGB8;

	MarkChunkDirty(it.m_chunk);

	auto markNeighborChunkDirty = [&](const BlockIterator& nbIt)
		{
			if (nbIt.m_chunk)
			{
				MarkChunkDirty(nbIt.m_chunk);
			}
		};

	markNeighborChunkDirty(it.GetWestNeighbor());
	markNeighborChunkDirty(it.GetEastNeighbor());
	markNeighborChunkDirty(it.GetSouthNeighbor());
	markNeighborChunkDirty(it.GetNorthNeighbor());
	markNeighborChunkDirty(it.GetBelowNeighbor());
	markNeighborChunkDirty(it.GetAboveNeighbor());

	auto markNonOpaqueNeighborDirty = [&](const BlockIterator& nbIt)
		{
			if (!nbIt.m_chunk || nbIt.m_blockIdx < 0) return;
			const Block& nb = nbIt.m_chunk->GetBlockRef(nbIt.m_blockIdx);
			const BlockDefinition& nbDef = BlockDefinition::GetDefinitionByIndex(nb.m_typeIndex);
			if (!nbDef.m_isOpaque)
			{
				MarkLightingDirty(nbIt);
			}
		};

	markNonOpaqueNeighborDirty(it.GetWestNeighbor());
	markNonOpaqueNeighborDirty(it.GetEastNeighbor());
	markNonOpaqueNeighborDirty(it.GetSouthNeighbor());
	markNonOpaqueNeighborDirty(it.GetNorthNeighbor());
	markNonOpaqueNeighborDirty(it.GetBelowNeighbor());
	markNonOpaqueNeighborDirty(it.GetAboveNeighbor());
}






void World::MarkLightingDirtyIfNotOpaque(const BlockIterator& it)
{
	if (!it.m_chunk || it.m_blockIdx < 0) return;
	const Block& b = it.m_chunk->GetBlockRef(it.m_blockIdx);
	if (!b.IsFullOpaque()) 
	{
		MarkLightingDirty(it);
	}
}

void World::UndirtyAllBlocksInChunk(Chunk* chunk)
{
	if (!chunk) return;

	auto it = m_dirtyLightBlocks.begin();
	while (it != m_dirtyLightBlocks.end())
	{
		if (it->m_chunk == chunk) 
		{
			if (chunk)
			{
				Block& b = chunk->GetBlockRef(it->m_blockIdx);
				b.SetIsLightDirty(false);
			}
			it = m_dirtyLightBlocks.erase(it);
		}
		else {
			++it;
		}
	}
}
