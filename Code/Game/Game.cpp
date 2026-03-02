#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/ErrorWarningAssert.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/SimpleTriangleFont.hpp"
#include "Engine/Math/AABB2.hpp"
#include "Engine/Renderer/BitmapFont.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Window/Window.hpp"
#include "Engine/Renderer/DebugRender.hpp"
#include "Engine/Math/Mat44.hpp"
#include "Engine/Core/StringUtils.hpp"
#include "Game/BlockDefinition.hpp"
#include "Engine/Renderer/SpriteSheet.hpp"
#include "Game/ChunkCoords.hpp"
#include "Engine/Core/JobSystem.hpp"


extern App* g_theApp;
extern Renderer* g_theRenderer;
extern InputSystem* g_theInput;
extern AudioSystem* g_theAudio;
extern DevConsole* g_theDevConsole;
extern EventSystem* g_theEventSystem;
extern BitmapFont* g_theFont;
extern Window* g_theWindow;



Game::Game(App* owner)
	: m_App(owner)
{
	Startup();

}

Game::~Game()
{
	Shutdown();
}

void Game::Startup()
{

	m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/SpriteSheet_Dokucraft_Light_32px.png", true, 5);
	m_spriteSheet = new SpriteSheet(*m_texture, IntVec2(8, 8));


	m_clock = new Clock();
	m_attractRingTimer = new Timer(0.5f, m_clock);

	m_player = new Player(this);
	m_entities.push_back(m_player);
	

	
	
	/*
	Prop* cube1 = new Prop(this, PropShape::CUBE);
	m_entities.push_back(cube1);
	cube1->SetPosition(Vec3(2.f, 2.f, 0.f));
	
	cube1->m_angularVelocity.m_pitchDegrees = 30.f;
	cube1->m_angularVelocity.m_rollDegrees = 30.f;


	Prop* cube2 = new Prop(this, PropShape::CUBE);
	m_entities.push_back(cube2);
	cube2->SetPosition(Vec3(-2.f, -2.f, 0.f));
	cube2->m_isBlink = true;


	Prop* sphere1 = new Prop(this, PropShape::SPHERE);
	m_entities.push_back(sphere1);
	sphere1->SetPosition(Vec3(10.f, -5.f, 1.f));
	sphere1->m_angularVelocity.m_yawDegrees = 45.f;
	sphere1->m_texture = g_theRenderer->CreateOrGetTextureFromFile("Data/Images/TestUV.png");
	*/

	float screenWidth = g_gameConfigBlackboard.GetValue("screenWidth", 1600.f);
	float screenHeight = g_gameConfigBlackboard.GetValue("screenHeight", 800.f);

	m_screenCamera.SetMode(Camera::eMode_Orthographic);
	m_screenCamera.SetOrthographicView(Vec2(0.f, 0.f), Vec2(screenWidth, screenHeight));

	//CreateAndAddVertsForGrid(m_gridVerts, Vec3(0.f, 0.f, 0.f), Vec2(100.f, 100.f), 100, 100);

	//CreateOriginalWorldBasis();
	

	JobSystemConfig cfg;
	cfg.m_workerCount = Max(1u, std::thread::hardware_concurrency() - 1);
	cfg.m_fileIOWorkerCount = 0;
	cfg.m_maxExecuting = cfg.m_workerCount;
	m_jobSystem = new JobSystem(cfg);

	m_jobSystem->Startup();

}

void Game::Update()
{
	float deltaSeconds = m_clock->GetDeltaSeconds();

	UpdateCameras(deltaSeconds);

	
	if (g_theInput->WasKeyJustPressed('P'))
	{
		m_clock->TogglePause();
	}

	if (g_theInput->WasKeyJustPressed('O'))
	{
		m_clock->StepSingleFrame();
	}

	if (g_theInput->IsKeyDown('T'))
	{
		m_clock->SetTimeScale(0.1f);
	}
	else
	{
		m_clock->SetTimeScale(1.f);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_TILDE))
	{
		m_isConsoleOpen = !m_isConsoleOpen;
		g_theDevConsole->ToggleOpen();
	}

	if (m_currentState != m_nextState)
	{
		ExitState(m_currentState);
		m_currentState = m_nextState;
		EnterState(m_currentState);
	}

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		UpdateAttractMode(deltaSeconds);
		break;
	case GameState::PLAYING:
		UpdateGame(deltaSeconds);
		break;
	default:
		break;
	}
}

void Game::UpdateInput(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		m_nextState = GameState::ATTRACT;
		//SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
		//g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
		g_theApp->ResetGame();
		DebugRenderClear();
		CreateOriginalWorldBasis();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F2))
	{
		m_isDebugActive = !m_isDebugActive;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_F8))
	{
		g_theApp->ReloadGame();
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_1))
	{
		m_world->m_placeMode = 0;
	}

	if (g_theInput->IsKeyDown(KEYCODE_2))
	{
		m_world->m_placeMode = 1;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_3))
	{
		m_world->m_placeMode = 2;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_4))
	{
		m_world->m_placeMode = 3;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_5))
	{
		m_world->m_placeMode = 4;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_6))
	{
		m_world->m_placeMode = 5;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_7))
	{
		m_world->m_placeMode = 6;
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_LEFT_MOUSE)) 
	{
		m_world->DigBlock();
	}
	if (g_theInput->WasKeyJustPressed(KEYCODE_RIGHT_MOUSE)) 
	{
		m_world->PlaceBlock();
	}

	

	if (m_world)
	{

		if (g_theInput->IsKeyDown('Y'))
		{
			m_world->m_worldClock->SetTimeScale(100.f);
		}
		else
		{
			m_world->m_worldClock->SetTimeScale(1.f);
		}

		if (g_theInput->WasKeyJustPressed('R'))
		{
			m_world->ToggleRayLock();
		}

		if (g_theInput->WasKeyJustPressed('L'))
		{
			m_world->DebugPrintBlockLightingInFrontOfPlayer();
		}

	}

	/*if (g_theInput->WasKeyJustPressed(KEYCODE_1))
	{
		Vec3 lineStart = m_player->GetPosition();
		Vec3 lineEnd = lineStart + m_player->GetFwdNormal() * 20.f;
		DebugAddWorldLine(lineStart, lineEnd, 0.05f, 10.f, Rgba8::YELLOW, Rgba8::YELLOW, DebugRenderMode::X_RAY);
	}

	if (g_theInput->IsKeyDown(KEYCODE_2))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 spawnPos = Vec3(playerPos.x, playerPos.y, 0.f);

		Rgba8 sphereColor = Rgba8(150, 75, 0, 255);
		DebugAddWorldPoint(spawnPos, 0.5f, 60.f, sphereColor, sphereColor, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_3))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 spherePos = Vec3(playerPos.x + 2.f, playerPos.y, playerPos.z);

		

		DebugAddWorldWireSphere(playerPos, 1.f, 5.f, Rgba8::GREEN, Rgba8::RED, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_4))
	{
		Vec3 playerPos = m_player->GetPosition();
		EulerAngles playerOrientation = m_player->GetOrientation();

		Mat44 playerTransform = playerOrientation.GetAsMatrix_IFwd_JLeft_KUp();
		playerTransform.SetTranslation3D(playerPos); 

		DebugAddWorldBasis(playerTransform, -1.f, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_5))
	{
		Vec3 playerPos = m_player->GetPosition();
		EulerAngles playerOrientation = m_player->GetOrientation();

		Mat44 playerTransform = playerOrientation.GetAsMatrix_IFwd_JLeft_KUp();
		playerTransform.SetTranslation3D(playerPos);

		Mat44 cameraMatrix = m_player->GetCamera().GetCameraToWorldTransform();

		Vec3 forwardOffset = playerTransform.GetIBasis3D() * 1.5f;
		Vec3 billboardPos = playerPos + forwardOffset;



		std::string text = Stringf("Position: %.1f, %.1f, %.1f Orientation: %.1f, %.1f, %.1f",
			playerPos.x, playerPos.y, playerPos.z,
			playerOrientation.m_yawDegrees, playerOrientation.m_pitchDegrees, playerOrientation.m_rollDegrees);

		DebugAddWorldBillboardText(text, billboardPos, 0.125f, Vec2(0.5f, 0.5f), 10.f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USE_DEPTH, BillboardType::FULL_OPPOSING);
	}


	if (g_theInput->WasKeyJustPressed(KEYCODE_6))
	{
		Vec3 playerPos = m_player->GetPosition();
		Vec3 playerUp = Vec3(0.0f, 0.0f, 1.0f);
		Vec3 playerDown = -playerUp;

		Vec3 cylinderTop = playerPos + (playerUp * 0.5f);
		Vec3 cylinderBot = playerPos + (playerDown * 0.5f);
		
		DebugAddWorldWireCylinder(cylinderBot, cylinderTop, 0.5f, 10.f, Rgba8::WHITE, Rgba8::RED, DebugRenderMode::USE_DEPTH);
	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_7))
	{
		std::string text;
		EulerAngles orientation = m_player->GetOrientation();
		text = Stringf("Camera Orientation: %.2f, %.2f, %.2f", orientation.m_yawDegrees, orientation.m_pitchDegrees, orientation.m_rollDegrees);

		DebugAddMessage(text, 5.f, Rgba8::WHITE, Rgba8::WHITE);
	}*/
}

void Game::Render() const
{
	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);

	switch (m_currentState)
	{
	case GameState::ATTRACT:
		RenderAttractMode();
		break;
	case GameState::PLAYING:

		RenderGame();
		DebugRender();

		RenderUI();
		break;
	default:
		break;
	}

	RenderDevConsole();
}

void Game::EnterState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		EnterAttractMode();
		break;

	case GameState::PLAYING:
		EnterPlayingMode();
		break;
	}
}

void Game::ExitState(GameState state)
{
	switch (state)
	{
	case GameState::ATTRACT:
		ExitAttractMode();
		break;
	case GameState::PLAYING:
		ExitPlayingMode();
		break;
	}
}


void Game::UpdateGame(float deltaSeconds)
{

	float time = m_clock->GetTotalSeconds();
	float FPS = 1.f / m_clock->GetDeltaSeconds();
	// float scale = m_clock->GetTimeScale();
	std::string timeText = Stringf("Time: %.2f", time);
	AABB2 timeBox(1400.f, 950.f, 1600.f, 1000.f);
	DebugAddScreenText(timeText, timeBox, 15.f, Vec2(0.f, 0.5f), 0.f);

	std::string fpsText = Stringf("FPS: %.1f", FPS);
	AABB2 fpsBox = timeBox;
	fpsBox.Translate(Vec2(-180.f, 0.f));
	DebugAddScreenText(fpsText, fpsBox, 15.f, Vec2(0.f, 0.5f), 0.f);

	std::string cameraText = Stringf("Camera Mode: %s", m_player->GetCameraMode().c_str());
	AABB2 cameraBox(10.f, 10.f, 200.f, 25.f);
	DebugAddScreenText(cameraText, cameraBox, 15.f, Vec2(0.f, 0.5f), 0.f, Rgba8::YELLOW);

	UpdateInput(deltaSeconds);

	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		Entity* entity = m_entities[i];
		if (entity)
		{
			entity->Update(deltaSeconds);
		}
	}


	if (m_world)
	{
		m_world->Update(deltaSeconds);
	}

	if (m_isDebugActive && m_world) 
	{
		const Vec3    camPos3 = m_world->m_camera->GetPosition();

		const IntVec3 gcell = GetGlobalCoords(camPos3);

		const IntVec2 ccoord = GetChunkCoords(gcell);

		auto floorMod = [](int a, int m) { int r = a % m; return r < 0 ? r + m : r; };
		const int lx = floorMod(gcell.x, CHUNK_SIZE_X);
		const int ly = floorMod(gcell.y, CHUNK_SIZE_Y);
		int       lz = gcell.z;
		if (lz < 0) lz = 0;
		if (lz > CHUNK_MAX_Z) lz = CHUNK_MAX_Z;

		const int chunkNum = (int)m_world->m_activeChunks.size();
		const int vertsSize = m_world->GetChunksVertsSize();
		const int indicesSize = m_world->GetChunksIndicesSize();

		const std::string hud = Stringf(
			"Pos:(%.2f, %.2f, %.2f) ChunkCoords:(%d,%d) GlobalCoords:(%d,%d,%d) LocalCoords:(%d,%d,%d) Chunks:%d Vertexes:%d Indices:%d",
			camPos3.x, camPos3.y, camPos3.z,
			ccoord.x, ccoord.y,
			gcell.x, gcell.y, gcell.z,
			lx, ly, lz,
			chunkNum, vertsSize, indicesSize);

		DebugAddScreenText(hud, AABB2(0.f, 930.f, 1700.f, 950.f),
			10.f, Vec2(0.f, 0.5f), -1.f, Rgba8::MAGENTA);
	}

	if (m_world)
	{
		std::string controlText = Stringf("[LMB] Dig [RMB] Add %s", m_world->GetPlaceModeText().c_str());
		AABB2 controlBox(0.f, 950.f, 500.f, 1000.f);
		DebugAddScreenText(controlText, controlBox, 15.f, Vec2(0.f, 0.5f), -1.f, Rgba8::YELLOW);
	}
	
	if (m_world && m_player)
	{
		std::string physicsModeText = Stringf("Current Physics Mode: %s", m_player->GetPhysicsMode().c_str());
		AABB2 modeBox(600.f, 10.f, 1000.f, 25.f);
		DebugAddScreenText(physicsModeText, modeBox, 15.f, Vec2(0.5f, 0.5f), -1.f, Rgba8::GREEN);

		std::string weatherText = Stringf("Current Weather: %s", m_world->GetWeatherText().c_str());
		AABB2 weaBox(1000.f, 10.f, 1600.f, 25.f);
		DebugAddScreenText(weatherText, weaBox, 15.f, Vec2(1.0f, 0.5f), -1.f, Rgba8::CYAN);

		std::string worldTimeText = Stringf("Current Time: %s", m_world->GetCurrentTimeText().c_str());
		AABB2 worldTimeBox = weaBox;
		worldTimeBox.Translate(Vec2(0.f, 30.f));
		DebugAddScreenText(worldTimeText, worldTimeBox, 15.f, Vec2(1.0f, 0.5f), -1.f, Rgba8::CYAN);

	}
	

}

void Game::RenderGame() const
{
	if (m_world)
	{
		Vec4 c = m_world->m_currentSkyColor;
		Rgba8 sky(
			(unsigned char)(c.x * 255),
			(unsigned char)(c.y * 255),
			(unsigned char)(c.z * 255),
			(unsigned char)(c.w * 255)
		);

		g_theRenderer->ClearScreen(sky);
	}
	else
	{
		g_theRenderer->ClearScreen(Rgba8(0, 0, 0, 255));
	}

	g_theRenderer->BeginCamera(m_player->GetCamera());

	g_theRenderer->BindTexture(nullptr);


	

	if (m_world)
	{
		m_world->Render();
	}

	RenderWorldBasis();
	
	g_theRenderer->EndCamera(m_player->GetCamera());

}

void Game::RenderUI() const
{
	g_theRenderer->BeginCamera(m_screenCamera);

	

	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderDevConsole() const
{
	g_theRenderer->BeginCamera(m_screenCamera);
	if (g_theDevConsole)
	{
		float screenWidth = g_gameConfigBlackboard.GetValue("screenWidth", 1600.f);
		float screenHeight = g_gameConfigBlackboard.GetValue("screenHeight", 800.f);
		g_theDevConsole->Render(AABB2(0, 0, screenWidth, screenHeight));
		
	}
	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderWorldBasis() const
{
	std::vector<Vertex_PCU> verts;
	Camera cam = m_player->GetCamera();

	const Vec3 camPos = cam.GetPosition();
	const Vec3 camFwd = cam.GetOrientation().GetForwardNormal();
	const Vec3 O = camPos + camFwd * 1.0f;

	const float axisLen = 0.02f;
	const float radius = axisLen * 0.04f;
	const float shaftPct = 0.82f;

	const Vec3 X(1, 0, 0), Y(0, 1, 0), Z(0, 0, 1);

	AddVertsForArrow3D(verts, O, O + X * axisLen, radius, shaftPct, Rgba8(255, 0, 0, 255));
	AddVertsForArrow3D(verts, O, O + Y * axisLen, radius, shaftPct, Rgba8(0, 255, 0, 255));
	AddVertsForArrow3D(verts, O, O + Z * axisLen, radius, shaftPct, Rgba8(0, 0, 255, 255));

	g_theRenderer->SetDepthMode(DepthMode::DISABLED);
	g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);
	g_theRenderer->SetRasterizerMode(RasterizerMode::SOLID_CULL_BACK);
	g_theRenderer->BindTexture(nullptr);

	g_theRenderer->DrawVertexArray((int)verts.size(), verts.data());

	g_theRenderer->SetDepthMode(DepthMode::READ_WRITE_LESS_EQUAL);
}

void Game::EnterAttractMode()
{
}

void Game::ExitAttractMode()
{
}

void Game::EnterPlayingMode()
{
	m_world = new World(this);
}

void Game::ExitPlayingMode()
{
	delete m_world;
	m_world = nullptr;
}


void Game::CreateOriginalWorldBasis()
{
	Vec3 basisPos;
	EulerAngles basisAngle;

	Mat44 basistransform = basisAngle.GetAsMatrix_IFwd_JLeft_KUp();
	basistransform.SetTranslation3D(basisPos);

	DebugAddWorldBasis(basistransform, -1.f, DebugRenderMode::USE_DEPTH);

	std::string xText = "x - forward";
	std::string yText = "y - left";
	std::string zText = "z - up";

	float textHeight = 0.2f;

	float alignment = 0.f;

	float duration = -1.f;

	DebugRenderMode mode = DebugRenderMode::USE_DEPTH;

	Vec3 xTextPos = basisPos + Vec3(0.2f, 0.f, 0.2f);
	Vec3 yTextPos = basisPos + Vec3(0.f, 1.8f, 0.2f);
	Vec3 zTextPos = basisPos + Vec3(0.f, -0.4f, 0.2f);

	Mat44 xTextTransform = Mat44::MakeTranslation3D(xTextPos);
	Mat44 yTextTransform = Mat44::MakeTranslation3D(yTextPos);
	Mat44 zTextTransform = Mat44::MakeTranslation3D(zTextPos);

	xTextTransform.AppendZRotation(-90.f);
	yTextTransform.AppendZRotation(180.f);
	
	zTextTransform.AppendZRotation(180.f);
	zTextTransform.AppendXRotation(90.f);

	DebugAddWorldText(xText, xTextTransform, textHeight, alignment, duration, Rgba8::RED, Rgba8::RED, mode);
	DebugAddWorldText(yText, yTextTransform, textHeight, alignment, duration, Rgba8::GREEN, Rgba8::GREEN, mode);
	DebugAddWorldText(zText, zTextTransform, textHeight, alignment, duration, Rgba8::BLUE, Rgba8::BLUE, mode);
}

void Game::CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols)
{
	float gridWidth = size.x;
	float gridHeight = size.y;

	float cellWidth = gridWidth / (float)numCols;
	float cellHeight = gridHeight / (float)numRows;

	Vec3 startPos = center - Vec3(gridWidth * 0.5f, gridHeight * 0.5f, 0.0f);

	float thinLineThickness = 0.02f;
	float thickLineThickness = 0.05f;
	float originLineThickness = 0.1f;

	Rgba8 normalColor = Rgba8(127, 127, 127, 50);
	Rgba8 thickXColor = Rgba8(255, 50, 50, 50);
	Rgba8 thickYColor = Rgba8(50, 255, 50, 50);
	Rgba8 originLineXColor = Rgba8(255, 0, 0, 255);
	Rgba8 originLineYColor = Rgba8(0, 255, 0, 255);

	for (int row = 0; row <= numRows; ++row)
	{
		float y = startPos.y + row * cellHeight;
		bool isOriginLine = (y == center.y);

		for (int col = 0; col < numCols; ++col)
		{
			float x1 = startPos.x + col * cellWidth;
			float x2 = x1 + cellWidth;

			Rgba8 lineColor = isOriginLine ? originLineXColor : (row % 5 == 0) ? thickXColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (row % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x1, y - lineThickness * 0.5f, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x2, y + lineThickness * 0.5f, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}

	for (int col = 0; col <= numCols; ++col)
	{
		float x = startPos.x + col * cellWidth;
		bool isOriginLine = (x == center.x);

		for (int row = 0; row < numRows; ++row)
		{
			float y1 = startPos.y + row * cellHeight;
			float y2 = y1 + cellHeight;

			Rgba8 lineColor = isOriginLine ? originLineYColor : (col % 5 == 0) ? thickYColor : normalColor;
			float lineThickness = isOriginLine ? originLineThickness : (col % 5 == 0) ? thickLineThickness : thinLineThickness;

			AABB3 lineBounds;
			lineBounds.m_mins = Vec3(x - lineThickness * 0.5f, y1, -lineThickness * 0.5f);
			lineBounds.m_maxs = Vec3(x + lineThickness * 0.5f, y2, lineThickness * 0.5f);

			AddVertsForAABB3D(verts, lineBounds, lineColor);
		}
	}
}


void Game::UpdateAttractMode(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	if (g_theInput->WasKeyJustPressed(' ') || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_A) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_START))
	{
		m_nextState = GameState::PLAYING;
		/*
		SoundID clickSound = g_theAudio->CreateOrGetSound("Data/Audio/click.wav");
		g_theAudio->StartSound(clickSound, false, 1.f, 0.f, 0.5f);
		*/

	}

	if (g_theInput->WasKeyJustPressed(KEYCODE_ESC) || g_theInput->GetController(0).WasButtonJustPressed(XBOX_BUTTON_BACK))
	{
		FireEvent("quit");
	}

	if (m_attractRingTimer->IsStopped())
	{
		m_attractRingTimer->Start();
	}

	if (m_attractRingTimer->DecrementPeriodIfElapsed())
	{
		m_zoomOut = !m_zoomOut;
	}

	if (m_zoomOut)
	{
		m_attractRingThickness += 20.f * deltaSeconds;
		m_attractRingRadius += 200.f * deltaSeconds;
	}
	else
	{
		m_attractRingThickness -= 20.f * deltaSeconds;
		m_attractRingRadius -= 200.f * deltaSeconds;
	}

}



void Game::RenderAttractMode() const
{
	g_theRenderer->ClearScreen(Rgba8(0, 127, 127, 255));
	g_theRenderer->BeginCamera(m_screenCamera);

	RenderAttractRing();


	g_theRenderer->EndCamera(m_screenCamera);
}

void Game::RenderAttractRing() const
{
	DebugDrawRing(Vec2(800.f, 500.f), m_attractRingRadius, m_attractRingThickness, Rgba8(255, 127, 0, 255));
	g_theRenderer->BindTexture(nullptr);
}
void Game::Shutdown()
{
	for (int i = 0; i < (int)m_entities.size(); ++i)
	{
		delete m_entities[i];
		m_entities[i] = nullptr;
	}
	m_entities.clear();

	m_player = nullptr;

	if (m_world)
	{
		delete m_world;
		m_world = nullptr;
	}

	m_gridVerts.clear();
	
	delete m_clock;
	m_clock = nullptr;

	m_jobSystem->Shutdown();
	delete m_jobSystem;
	m_jobSystem = nullptr;

}

bool Game::Event_KeysAndFuncs(EventArgs& args)
{
	UNUSED(args);

	g_theDevConsole->AddLine(DevConsole::INFO_MAJOR, "Controls");

	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Mouse / (R Stick)  - Aim");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "WASD / (L Stick)   - Move");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "QE / (LS)(RS)      - Elevate");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Shift / (LT)(RT)   - Sprint");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "C                  - Toggle Camera Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Y                  - Accelerate World Time while Held");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "R                  - Toggle Raycast Locking");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "V                  - Toggle Physics Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "LMB                - Digs a block");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "RMB                - Places a block");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "1                  - Glowstone");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "2                  - Cobblestone");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "3                  - Chiseled Brick");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "4                  - Water");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "5                  - Glowstone Red");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "6                  - Glowstone Green");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "7                  - Glowstone Blue");

	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "F2 / (A)           - Toggle Debug Draw");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "F8 / (B)           - Reload Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "P                  - Pause Game");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "O                  - Step One Frame");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "T                  - Slow Motion Mode");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "~                  - Open / Close DevConsole");
	g_theDevConsole->AddLine(DevConsole::INFO_MINOR, "Escape             - Exit Game");


	return true;
}


void Game::DebugRender() const
{
	g_theRenderer->BeginCamera(m_player->GetCamera());
	if (m_isDebugActive && m_world)
	{
		m_world->DebugRender();
	}
	g_theRenderer->EndCamera(m_player->GetCamera());

	DebugRenderWorld(m_player->GetCamera());
	DebugRenderScreen(m_screenCamera);
}

void Game::UpdateCameras(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	
}

