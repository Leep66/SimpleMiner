#include "Game/GameCommon.hpp"
#include "Engine/Math/Vec2.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/EventSystem.hpp"
#include "Engine/Core/DevConsole.hpp"

extern EventSystem* g_theEventSystem;
extern DevConsole* g_theDevConsole;
extern Renderer* g_theRenderer;

void DebugDrawRing(Vec2 const& center, float radius, float thickness, Rgba8 const& color)
{
	float halfThickness = 0.5f * thickness;

	float innerRadius = radius - halfThickness;
	float outerRadius = radius + halfThickness;

	constexpr int NUM_SIDES = 32;
	constexpr int NUM_TRIS = 2 * NUM_SIDES;
	constexpr int NUM_VERTS = 3 * NUM_TRIS;

	Vertex_PCU verts[NUM_VERTS];

	constexpr float DEGREES_PER_SIDE = 360.f / static_cast<float>(NUM_SIDES);

	for (int sideNum = 0; sideNum < NUM_SIDES; ++sideNum)
	{
		float startDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum);
		float endDegrees = DEGREES_PER_SIDE * static_cast<float>(sideNum + 1);

		float cosStart = CosDegrees(startDegrees);
		float sinStart = SinDegrees(startDegrees);
		float cosEnd = CosDegrees(endDegrees);
		float sinEnd = SinDegrees(endDegrees);


		Vec3 inneerStartPos = Vec3(center.x + innerRadius * cosStart, center.y + innerRadius * sinStart, 0.f);
		Vec3 outerStartPos = Vec3(center.x + outerRadius * cosStart, center.y + outerRadius * sinStart, 0.f);
		Vec3 outerEndPos = Vec3(center.x + outerRadius * cosEnd, center.y + outerRadius * sinEnd, 0.f);
		Vec3 innerEndPos = Vec3(center.x + innerRadius * cosEnd, center.y + innerRadius * sinEnd, 0.f);

		int vertIndexA = (6 * sideNum) + 0;
		int vertIndexB = (6 * sideNum) + 1;
		int vertIndexC = (6 * sideNum) + 2;
		int vertIndexD = (6 * sideNum) + 3;
		int vertIndexE = (6 * sideNum) + 4;
		int vertIndexF = (6 * sideNum) + 5;

		verts[vertIndexA].m_position = innerEndPos;
		verts[vertIndexB].m_position = inneerStartPos;
		verts[vertIndexC].m_position = outerStartPos;

		verts[vertIndexA].m_color = color;
		verts[vertIndexB].m_color = color;
		verts[vertIndexC].m_color = color;


		verts[vertIndexD].m_position = innerEndPos;
		verts[vertIndexE].m_position = outerStartPos;
		verts[vertIndexF].m_position = outerEndPos;

		verts[vertIndexD].m_color = color;
		verts[vertIndexE].m_color = color;
		verts[vertIndexF].m_color = color;
	}

	g_theRenderer->DrawVertexArray(NUM_VERTS, verts);
}

void DebugDrawLine(Vec2 const& startPos, Vec2 const& endPos, float thickness, Rgba8 const& color)
{
	float h = thickness * 0.5f;

	Vec2 startToEnd = endPos - startPos;
	Vec2 stepFwd = h * startToEnd.GetNormalized();

	Vec2 stepLeft = stepFwd.GetRotated90Degrees();

	Vec2 topLeft = endPos + stepFwd + stepLeft;
	Vec2 topRight = endPos + stepFwd - stepLeft;
	Vec2 bottomLeft = startPos - stepFwd + stepLeft;
	Vec2 bottomRight = startPos - stepFwd - stepLeft;

	Vertex_PCU verts[6];

	verts[0].m_position = Vec3(bottomLeft.x, bottomLeft.y, 0.f);
	verts[1].m_position = Vec3(topLeft.x, topLeft.y, 0.f);
	verts[2].m_position = Vec3(topRight.x, topRight.y, 0.f);

	verts[3].m_position = Vec3(bottomLeft.x, bottomLeft.y, 0.f);
	verts[4].m_position = Vec3(topRight.x, topRight.y, 0.f);
	verts[5].m_position = Vec3(bottomRight.x, bottomRight.y, 0.f);

	for (int i = 0; i < 6; ++i)
	{
		verts[i].m_color = color;
	}

	g_theRenderer->DrawVertexArray(6, verts);
}

