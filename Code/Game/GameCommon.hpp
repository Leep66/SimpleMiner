#pragma once

#include "Engine/Core/EngineCommon.hpp"

class Renderer;
class Window;
class InputSystem;
class AudioSystem;
class EventSystem;
class DevConsole;
class App;
struct Vec2;
struct Rgba8;

extern Renderer*		g_theRenderer;
extern Window*			g_theWindow;
extern InputSystem*		g_theInput;
extern AudioSystem*		g_theAudio;
extern App*				g_theApp;

extern bool g_isDebugDraw;


constexpr float SCREEN_SIZE_X = 1600;
constexpr float SCREEN_SIZE_Y = 800;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color);

void DebugDrawLine(Vec2 const& S, Vec2 const& E, float thickness, Rgba8 const& color);


