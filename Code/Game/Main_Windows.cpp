#include "Game/App.hpp"
#include <Engine/Core/EngineCommon.hpp>
#include "Engine/Input/InputSystem.hpp"

#define WIN32_LEAN_AND_MEAN		// Always #define this before #including <windows.h>
#include <windows.h>			// #include this (massive, platform-specific) header in VERY few places (and .CPPs only)
#include <math.h>
#include <cassert>
#include <crtdbg.h>
#pragma warning(disable: 28251)

extern App* g_theApp;

//-----------------------------------------------------------------------------------------------
int WINAPI WinMain( HINSTANCE applicationInstanceHandle, HINSTANCE, LPSTR commandLineString, int )
{
	UNUSED(applicationInstanceHandle);
	UNUSED( commandLineString );
	
	g_theApp = new App();
	g_theApp->Startup();

	// Program main loop; keep running frames until it's time to quit
	while(!g_theApp->IsQuitting())			
	{

		g_theApp->RunFrame();

	}
	g_theApp->Shutdown();
	delete g_theApp;
	g_theApp = nullptr;

	return 0;
}


