#include "Game/Prop.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include "Engine/Core/Rgba8.hpp"
#include "Engine/Renderer/Texture.hpp"
#include "Engine/Renderer/Renderer.hpp"
#include "Engine/Core/VertexUtils.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Game/Game.hpp"

extern Renderer* g_theRenderer;

struct ModelConstants
{
	Mat44 ModelToWorldTransform;
};

Prop::Prop(Game* owner, PropShape shape)
	: Entity(owner)
{
	m_shape = shape;

	m_blinkTimer = new Timer(2.0f, m_game->m_clock);

	m_orientation = EulerAngles();
	m_angularVelocity = EulerAngles();

	InitializeVerts();
}

Prop::~Prop()
{
	m_vertexes.clear();
	m_texture = nullptr;
}

void Prop::Update(float deltaSeconds)
{
	deltaSeconds = m_game->m_clock->GetDeltaSeconds();
	if (m_isBlink && m_blinkTimer->IsStopped())
	{
		m_blinkTimer->Start();
	}

	UpdateBlink();

	UpdateRotation(deltaSeconds);

	m_position += m_velocity * deltaSeconds;
	
}

void Prop::Render() const
{

	g_theRenderer->SetBlendMode(BlendMode::Blend_OPAQUE);


	g_theRenderer->BindTexture(m_texture);
	g_theRenderer->SetModelConstants(GetModelToWorldTransform(), m_color);

	g_theRenderer->DrawVertexArray(m_vertexes);
	g_theRenderer->BindTexture(nullptr);
}

void Prop::SetPosition(Vec3 position)
{
	m_position = position;
}

void Prop::SetOrientation(EulerAngles orientation)
{
	m_orientation = orientation;
}

void Prop::UpdateBlink()
{
	if (m_isBlink)
	{
		float t = m_blinkTimer->GetElapsedFraction();

		float rgb;
		if (!m_isFading)
		{
			rgb = RangeMapClamped(t, 0.f, 1.f, 0.f, 255.f);
		}
		else
		{
			rgb = RangeMapClamped(t, 0.f, 1.f, 255.f, 0.f);
		}

		m_color = Rgba8((unsigned char)rgb, (unsigned char)rgb, (unsigned char)rgb, 255);

		while (m_blinkTimer->DecrementPeriodIfElapsed())
		{
			m_isFading = !m_isFading;
		}
	}
}

void Prop::UpdateRotation(float deltaSeconds)
{
	float yawDelta = m_angularVelocity.m_yawDegrees * deltaSeconds;
	float pitchDelta = m_angularVelocity.m_pitchDegrees * deltaSeconds;
	float rollDelta = m_angularVelocity.m_rollDegrees * deltaSeconds;

	m_orientation.m_yawDegrees += yawDelta;
	m_orientation.m_pitchDegrees += pitchDelta;
	m_orientation.m_rollDegrees += rollDelta;

	
}



void Prop::InitializeVerts()
{
	if (m_shape == PropShape::CUBE)
	{
		Vec3 FBL = Vec3(-0.5f, -0.5f, 0.5f);
		Vec3 FBR = Vec3(0.5f, -0.5f, 0.5f);
		Vec3 FTR = Vec3(0.5f, 0.5f, 0.5f);
		Vec3 FTL = Vec3(-0.5f, 0.5f, 0.5f);

		Vec3 BBL = Vec3(-0.5f, -0.5f, -0.5f);
		Vec3 BBR = Vec3(0.5f, -0.5f, -0.5f);
		Vec3 BTR = Vec3(0.5f, 0.5f, -0.5f);
		Vec3 BTL = Vec3(-0.5f, 0.5f, -0.5f);

		AddVertsForQuad3D(m_vertexes, BBR, BTR, FTR, FBR, Rgba8::RED);
		AddVertsForQuad3D(m_vertexes, BTL, BBL, FBL, FTL, Rgba8::CYAN);

		AddVertsForQuad3D(m_vertexes, BTR, BTL, FTL, FTR, Rgba8::GREEN);
		AddVertsForQuad3D(m_vertexes, BBL, BBR, FBR, FBL, Rgba8::MAGENTA);

		AddVertsForQuad3D(m_vertexes, FTL, FBL, FBR, FTR, Rgba8::BLUE);
		AddVertsForQuad3D(m_vertexes, BTR, BBR, BBL, BTL, Rgba8::YELLOW);
	}
	else
	{
		AddVertsForSphere3D(m_vertexes, m_position, m_radius, m_color);
	}
}



