#pragma once
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Math/Vec3.hpp"
#include "Game/GameCommon.hpp"
#include "Engine/Math/RandomNumberGenerator.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Audio/AudioSystem.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Core/Clock.hpp"
#include "Game/Player.hpp"
#include "Engine/Core/Timer.hpp"
#include "Game/Prop.hpp"
#include "Game/World.hpp"

class SpriteSheet;
class JobSystem;

enum class GameState
{
	NONE,
	ATTRACT,
	PLAYING,

	COUNT
};



class Game 
{
public:

	Game(App* owner);	
	~Game();
	void Startup();

	void Update();
	void Render() const;

	void EnterState(GameState state);
	void ExitState(GameState state);

	void Shutdown();
	
	static bool Event_KeysAndFuncs(EventArgs& args);

public:
	App* m_App = nullptr;
	RandomNumberGenerator* m_rng = nullptr;
	bool m_isDebugActive = false;
	Rgba8 m_startColor = Rgba8(0, 255, 0, 255);
	Camera m_screenCamera;
	bool m_isConsoleOpen = false;
	Clock* m_clock = nullptr;

	World* m_world = nullptr;

	Player* m_player;
	std::vector<Entity*> m_entities;

	GameState m_currentState = GameState::NONE;
	GameState m_nextState = GameState::ATTRACT;

	Texture* m_texture = nullptr;
	SpriteSheet* m_spriteSheet = nullptr;

	JobSystem* m_jobSystem = nullptr;
	
private:
	void UpdateInput(float deltaSeconds);
	void DebugRender() const;
	void UpdateCameras(float deltaSeconds);
	void UpdateAttractMode(float deltaSeconds);
	void RenderAttractMode() const;
	void RenderAttractRing() const;

	void UpdateGame(float deltaSeconds);
	void RenderGame() const;
	void RenderUI() const;
	void RenderDevConsole() const;
	void RenderWorldBasis() const;

	void EnterAttractMode();
	void ExitAttractMode();
	void EnterPlayingMode();
	void ExitPlayingMode();

	void CreateOriginalWorldBasis();
	void CreateAndAddVertsForGrid(std::vector<Vertex_PCU>& verts, const Vec3& center, const Vec2& size, int numRows, int numCols);

private:
	float m_attractRingRadius = 150.f;
	float m_attractRingThickness = 20.f;
	Timer* m_attractRingTimer = nullptr;
	bool m_zoomOut = true;

	bool m_debugDraw = false;

	std::vector<Vertex_PCU> m_gridVerts;
	
};