#include "Game/Player.hpp"
#include "Engine/Core/DevConsole.hpp"
#include "Engine/Input/InputSystem.hpp"
#include "Engine/Core/Clock.hpp"
#include "Engine/Math/MathUtils.hpp"
#include "Engine/Window/Window.hpp"
#include "Game/Game.hpp"
#include "Game/App.hpp"
#include "Engine/Core/VertexUtils.hpp"

extern DevConsole* g_theDevConsole;
extern InputSystem* g_theInput;
extern Window* g_theWindow;

Player::Player(Game* owner)
	:Entity(owner)
{
	Mat44 mat;
	mat.SetIJK3D(Vec3(0.f, 0.f, 1.f), Vec3(-1.f, 0.f, 0.f), Vec3(0.f, 1.f, 0.f));
	m_camera.SetCameraToRenderTransform(mat);

	float fov = 60.f;
	float aspect = g_theWindow->GetAspect();
	float zNear = 0.01f;
	float zFar = 10000.f;
	m_camera.SetMode(Camera::eMode_Perspective);
	m_camera.SetPerspectiveView(aspect, fov, zNear, zFar);

	SetPosition(Vec3(-50.f, -50.f, 100.f));
	m_camera.SetOrientation(EulerAngles(45, 45, 0));

	AddVertsForAABBWireframe3D(m_verts, m_collider, 0.01f, Rgba8::GREEN);
	m_aim = m_orientation;
}

Player::~Player()
{
}

void Player::Update(float deltaSeconds)
{
	deltaSeconds = Clock::GetSystemClock().GetDeltaSeconds();
	if (g_theDevConsole->IsOpen()) return;

	HandleInput(deltaSeconds);
	UpdatePhysics(deltaSeconds);
	UpdateCamera(deltaSeconds);
}


void Player::Render() const
{
	
	if (m_cameraMode != FIRST_PERSON)
	{
		std::vector<Vertex_PCU> worldVerts = m_verts;
		TransformVertexArray3D(worldVerts, GetModelToWorldTransform());

		g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(nullptr);
		g_theRenderer->DrawVertexArray(worldVerts);
	}

	if (m_velocity.GetLengthSquared() > 0.f)
	{
		Vec3 corners[8];
		GetColliderWorldCorners(corners);

		std::vector<Vertex_PCU> velVerts;
		velVerts.reserve(8 * 2 * 6);

		float velScale = 0.05f;                
		Vec3 velDir = m_velocity * velScale;

		for (int i = 0; i < 8; ++i)
		{
			Vec3 start = corners[i];
			Vec3 end = start + velDir;

			AddVertsForArrow3D(
				velVerts,
				start,
				end,
				0.02f,
				0.7f,
				Rgba8(0, 255, 0, 100)
			);
		}

		g_theRenderer->SetModelConstants();
		g_theRenderer->BindTexture(nullptr);
		
		g_theRenderer->DrawVertexArray(velVerts);
	}
}


void Player::UpdatePhysics(float deltaSeconds)
{
	if (deltaSeconds <= 0.f) return;

	switch (m_physicsMode)
	{
	case WALK:
	{
		if (!m_isGrounded)
		{
			m_velocity.z += m_gravity * deltaSeconds;
		}

		Vec3 horizontalVel(m_velocity.x, m_velocity.y, 0.f);
		float horSpeed = horizontalVel.GetLength();
		float friction = m_isGrounded ? m_walkFriction : m_airFriction;

		if (horSpeed > 0.f && friction > 0.f)
		{
			float drop = friction * deltaSeconds;
			float newSpeed = horSpeed - drop;
			if (newSpeed < 0.f) newSpeed = 0.f;

			if (horSpeed > 0.f)
			{
				horizontalVel *= (newSpeed / horSpeed);
				m_velocity.x = horizontalVel.x;
				m_velocity.y = horizontalVel.y;
			}
		}

		horizontalVel = Vec3(m_velocity.x, m_velocity.y, 0.f);
		horSpeed = horizontalVel.GetLength();
		if (horSpeed > m_walkMaxSpeed)
		{
			horizontalVel = horizontalVel.GetNormalized() * m_walkMaxSpeed;
			m_velocity.x = horizontalVel.x;
			m_velocity.y = horizontalVel.y;
		}

		ResolveVerticalCollisionUsingRaycast(deltaSeconds);

		UpdateGroundedStateFromWorld();
		break;
	}

	case FLY:
	{
		float speed = m_velocity.GetLength();
		if (speed > 0.f && m_airFriction > 0.f)
		{
			float drop = m_airFriction * deltaSeconds;
			float newSpeed = speed - drop;
			if (newSpeed < 0.f) newSpeed = 0.f;

			m_velocity = m_velocity.GetNormalized() * newSpeed;
		}

		speed = m_velocity.GetLength();
		if (speed > m_flyMaxSpeed)
		{
			m_velocity = m_velocity.GetNormalized() * m_flyMaxSpeed;
		}

		ResolveVerticalCollisionUsingRaycast(deltaSeconds);

		m_isGrounded = false;
		break;
	}
	case NOCLIP:
	{
		float speed = m_velocity.GetLength();
		if (speed > 0.f && m_airFriction > 0.f)
		{
			float drop = m_airFriction * deltaSeconds;
			float newSpeed = speed - drop;
			if (newSpeed < 0.f) newSpeed = 0.f;

			m_velocity = m_velocity.GetNormalized() * newSpeed;
		}
 
		speed = m_velocity.GetLength();
		if (speed > m_flyMaxSpeed)
		{
			m_velocity = m_velocity.GetNormalized() * m_flyMaxSpeed;
		}

		m_position += m_velocity * deltaSeconds;
		m_isGrounded = false;
		break;
	}

	}
}

void Player::UpdateCamera(float deltaSeconds)
{
	UNUSED(deltaSeconds);
	Vec3 eyePos = m_position + Vec3(0.f, 0.f, m_eyeHeight);

	switch (m_cameraMode)
	{
	case FIRST_PERSON:
	{
		m_camera.SetPosition(eyePos);
		m_camera.SetOrientation(m_aim);
		break;
	}

	case OVER_THE_SHOULDER:
	{
		Vec3 pivotPos = eyePos;

		Vec3 fwd, left, up;
		m_aim.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

		float dist = m_overShoulderDistance;
		float side = 0.0f;
		float height = m_overShoulderHeight;

		Vec3 localOffset(-dist, side, height);

		Vec3 desiredCamPos =
			pivotPos
			+ fwd * localOffset.x
			+ left * localOffset.y
			+ up * localOffset.z;

		Vec3 camPos = desiredCamPos;

		if (m_game && m_game->m_world)
		{
			Vec3 toCam = desiredCamPos - pivotPos;
			float maxDist = toCam.GetLength();

			if (maxDist > 0.001f)
			{
				Vec3 dir = toCam / maxDist;
				GameRaycastResult3D hit;
				if (m_game->m_world->RaycastVsBlocks(pivotPos, dir, maxDist, hit)
					&& hit.m_didImpact)
				{
					float padding = 0.1f;
					camPos = hit.m_impactPos - dir * padding;
				}
			}
		}

		m_camera.SetPosition(camPos);
		m_camera.SetOrientation(m_aim);
		break;
	}



	case FIXED_ANGLE_TRACKING:
	{
		EulerAngles camAngles;
		camAngles.m_yawDegrees = m_fixedYawDegrees;
		camAngles.m_pitchDegrees = m_fixedPitchDegrees;
		camAngles.m_rollDegrees = 0.f;

		Vec3 camFwd, camLeft, camUp;
		camAngles.GetAsVectors_IFwd_JLeft_KUp(camFwd, camLeft, camUp);

		float dist = m_fixedTrackDistance;
		Vec3 camPos = eyePos - camFwd * dist;

		m_camera.SetPosition(camPos);
		m_camera.SetOrientation(camAngles);
		break;
	}

	case INDEPENDENT:
	case SPECTATOR_FULL:
	case SPECTATOR_XY:
		break;

	default:
		m_camera.SetPosition(eyePos);
		m_camera.SetOrientation(m_aim);
		break;
	}
}




void Player::HandleInput(float deltaSeconds)
{

	/*if (g_theInput->WasKeyJustPressed('H'))
	{
		m_position = Vec3();
		m_orientation = EulerAngles();
	}*/

	if (g_theInput->WasKeyJustPressed('C')) 
	{
		ToggleCameraMode();
	}
	if (g_theInput->WasKeyJustPressed('V')) 
	{
		TogglePhysicsMode();
	}

	switch (m_cameraMode)
	{
	case FIRST_PERSON:
	case OVER_THE_SHOULDER:
	case FIXED_ANGLE_TRACKING:
	case INDEPENDENT:
		HandlePlayerInput(deltaSeconds);
		break;

	case SPECTATOR_FULL:
	case SPECTATOR_XY:
		HandleSpectatorCameraInput(deltaSeconds);
		break;
	}

	/*MoveByController(deltaSeconds);
	RotateByController(deltaSeconds);
	HandleControllerButtons();*/
}

/*
void Player::MoveByKeyboard(float deltaSeconds)
{
	Vec3 cameraFwd, cameraLeft, cameraUp;

	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);

	Vec3 moveDirection;

	Vec3 worldUp = Vec3(0, 0, 1);

	Vec3 fwd = cameraFwd;

	if (g_theInput->IsKeyDown('W')) moveDirection += fwd;
	if (g_theInput->IsKeyDown('S')) moveDirection -= fwd;

	if (g_theInput->IsKeyDown('A')) moveDirection += cameraLeft;
	if (g_theInput->IsKeyDown('D')) moveDirection -= cameraLeft;

	if (g_theInput->IsKeyDown('Q')) moveDirection += worldUp;
	if (g_theInput->IsKeyDown('E')) moveDirection -= worldUp;
	

	if (moveDirection != Vec3())
	{
		moveDirection = moveDirection.GetNormalized();
	}

	float speed = m_moveSpeed;
	if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDirection * speed * deltaSeconds;
}

void Player::RotateByKeyboard(float deltaSeconds)
{
	UNUSED(deltaSeconds);

	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();

	m_orientation.m_yawDegrees -= mouseDelta.x * m_sensitivity;

	m_orientation.m_pitchDegrees += mouseDelta.y * m_sensitivity;

	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f);

	/ *
	float rollDelta = 0.0f;
	if (g_theInput->IsKeyDown('Q')) rollDelta -= m_rollSpeed * deltaSeconds;
	if (g_theInput->IsKeyDown('E')) rollDelta += m_rollSpeed * deltaSeconds;

	m_orientation.m_rollDegrees += rollDelta;

	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f);
	* /
}

void Player::MoveByController(float deltaSeconds)
{
	XboxController controller = g_theInput->GetController(0);

	Vec3 cameraFwd, cameraLeft, cameraUp;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(cameraFwd, cameraLeft, cameraUp);
	cameraUp = Vec3(0.f, 0.f, 1.f); 

	Vec2 ls = controller.GetLeftStick().GetPosition(); 
	Vec3 moveDir = cameraFwd * ls.y - cameraLeft * ls.x;

	if (controller.IsButtonDown(XBOX_BUTTON_LB)) moveDir -= cameraUp;
	if (controller.IsButtonDown(XBOX_BUTTON_RB)) moveDir += cameraUp;

	if (moveDir.GetLengthSquared() > 0.f)
		moveDir = moveDir.GetNormalized();

	float speed = m_moveSpeed;

	if (controller.GetLeftTrigger() > 0.f || controller.GetRightTrigger() > 0.f)
	{
		speed *= m_sprintMultiplier;
	}

	m_position += moveDir * speed * deltaSeconds;
	m_camera.SetPosition(m_position);
}

void Player::RotateByController(float deltaSeconds)
{
	XboxController controller = g_theInput->GetController(0); 
	Vec2 rightStick = controller.GetRightStick().GetPosition();
	m_orientation.m_yawDegrees -= rightStick.x * 180.f * deltaSeconds;
	m_orientation.m_pitchDegrees -= rightStick.y * 180.f * deltaSeconds;
	m_orientation.m_pitchDegrees = GetClamped(m_orientation.m_pitchDegrees, -85.0f, 85.0f); 
	/ * float leftTrigger = controller.GetLeftTrigger(); 
	float rightTrigger = controller.GetRightTrigger(); 
	float rollDelta = (rightTrigger - leftTrigger) * m_rollSpeed * deltaSeconds;
	m_orientation.m_rollDegrees += rollDelta; 
	m_orientation.m_rollDegrees = GetClamped(m_orientation.m_rollDegrees, -45.0f, 45.0f) * / 
	m_camera.SetOrientation(m_orientation);
}*/

void Player::HandleControllerButtons()
{
	XboxController controller = g_theInput->GetController(0);


	if (controller.WasButtonJustPressed(XBOX_BUTTON_A)) 
	{
		m_game->m_isDebugActive = !m_game->m_isDebugActive;
	}

	if (controller.WasButtonJustPressed(XBOX_BUTTON_B)) 
	{
		g_theApp->ReloadGame();
	}
}


void Player::SetPosition(Vec3 pos)
{
	m_position = pos;
}

void Player::SetOrientation(EulerAngles ori)
{
	m_orientation = ori;
}

Vec3 Player::GetFwdNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return fwd;
}

Vec3 Player::GetLeftNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return left;
}

Vec3 Player::GetUpNormal() const
{
	Vec3 fwd, left, up;
	m_orientation.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);

	return up;
}

std::string Player::GetPhysicsMode() const
{
	switch (m_physicsMode)
	{
	case PhysicsMode::WALK:
		return "Walk";
	case PhysicsMode::FLY:
		return "Fly";
	case PhysicsMode::NOCLIP:
		return "No Clip";
	default:
		return "Walk";
	}
}

void Player::TogglePhysicsMode()
{
	switch (m_physicsMode)
	{
	case WALK:
		m_physicsMode = FLY;
		break;
	case FLY:
		m_physicsMode = NOCLIP;
		break;
	case NOCLIP:
		m_physicsMode = WALK;
		break;
	}
}

std::string Player::GetCameraMode() const
{
	switch (m_cameraMode)
	{
	case FIRST_PERSON:
		return "First Person";
	case OVER_THE_SHOULDER:
		return "Over the Shoulder";
	case FIXED_ANGLE_TRACKING:
		return "Fixed Angle Tracking";
	case INDEPENDENT:
		return "Independent";
	case SPECTATOR_FULL:
		return "Spectator Full";
	case SPECTATOR_XY:
		return "Spectator XY";
	default:
		return "Invalid";
	}
}

void Player::ToggleCameraMode()
{
	switch (m_cameraMode)
	{
	case FIRST_PERSON:
		m_cameraMode = OVER_THE_SHOULDER;
		break;

	case OVER_THE_SHOULDER:
		m_cameraMode = FIXED_ANGLE_TRACKING;
		break;

	case FIXED_ANGLE_TRACKING:
		m_cameraMode = SPECTATOR_FULL;
		break;

	case SPECTATOR_FULL:
		m_cameraMode = SPECTATOR_XY;
		break;

	case SPECTATOR_XY:
		m_cameraMode = INDEPENDENT;
		break;

	case INDEPENDENT:
		m_cameraMode = FIRST_PERSON;
		break;

	default:
		m_cameraMode = FIRST_PERSON;
		break;
	}
}



void Player::HandlePlayerInput(float deltaSeconds)
{

	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();
	m_aim.m_yawDegrees -= mouseDelta.x * m_sensitivity;
	m_aim.m_pitchDegrees += mouseDelta.y * m_sensitivity;
	m_aim.m_pitchDegrees = GetClamped(m_aim.m_pitchDegrees, -85.0f, 85.0f);

	EulerAngles moveOrient;

	if (m_cameraMode == FIXED_ANGLE_TRACKING)
	{
		moveOrient.m_yawDegrees = m_fixedYawDegrees;
		moveOrient.m_pitchDegrees = 0.f;
		moveOrient.m_rollDegrees = 0.f;
	}
	else
	{
		moveOrient = m_aim;
		moveOrient.m_pitchDegrees = 0.f;
		moveOrient.m_rollDegrees = 0.f;
	}

	Vec3 fwd, left, up;
	moveOrient.GetAsVectors_IFwd_JLeft_KUp(fwd, left, up);
	Vec3 worldUp(0.f, 0.f, 1.f);

	Vec3 moveDir = Vec3::ZERO;

	if (g_theInput->IsKeyDown('W')) moveDir += fwd;
	if (g_theInput->IsKeyDown('S')) moveDir -= fwd;
	if (g_theInput->IsKeyDown('A')) moveDir += left;
	if (g_theInput->IsKeyDown('D')) moveDir -= left;

	if (m_physicsMode != WALK)
	{
		if (g_theInput->IsKeyDown('Q')) moveDir += worldUp;
		if (g_theInput->IsKeyDown('E')) moveDir -= worldUp;
	}

	if (moveDir.GetLengthSquared() > 0.f)
	{
		moveDir = moveDir.GetNormalized();

		float accel = 0.f;
		if (m_physicsMode == WALK)
		{
			accel = m_isGrounded ? m_walkAccel : m_airAccel;
		}
		else
		{
			accel = m_airAccel;
		}

		if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
		{
			accel *= m_sprintMultiplier;
		}

		m_velocity += moveDir * accel * deltaSeconds;
	}

	if (m_physicsMode == WALK && m_isGrounded)
	{
		if (g_theInput->WasKeyJustPressed(' '))
		{
			m_velocity.z = m_jumpSpeed;
			m_isGrounded = false;
		}
	}
}


void Player::HandleSpectatorCameraInput(float deltaSeconds)
{
	Vec2 mouseDelta = g_theInput->GetCursorClientDelta();

	EulerAngles camOri = m_camera.GetOrientation();
	camOri.m_yawDegrees -= mouseDelta.x * m_sensitivity;
	camOri.m_pitchDegrees += mouseDelta.y * m_sensitivity;
	camOri.m_pitchDegrees = GetClamped(camOri.m_pitchDegrees, -85.0f, 85.0f);

	Vec3 camFwd, camLeft, camUp;
	camOri.GetAsVectors_IFwd_JLeft_KUp(camFwd, camLeft, camUp);
	Vec3 worldUp(0.f, 0.f, 1.f);

	Vec3 moveDir = Vec3::ZERO;
	Vec3 fwd = camFwd;

	if (m_cameraMode == SPECTATOR_XY)
	{
		fwd.z = 0.f;
		if (fwd.GetLengthSquared() > 0.f)
		{
			fwd = fwd.GetNormalized();
		}
	}

	if (g_theInput->IsKeyDown('W')) moveDir += fwd;
	if (g_theInput->IsKeyDown('S')) moveDir -= fwd;
	if (g_theInput->IsKeyDown('A')) moveDir += camLeft;
	if (g_theInput->IsKeyDown('D')) moveDir -= camLeft;

	if (m_cameraMode == SPECTATOR_FULL)
	{
		if (g_theInput->IsKeyDown('Q')) moveDir += worldUp;
		if (g_theInput->IsKeyDown('E')) moveDir -= worldUp;
	}

	Vec3 camPos = m_camera.GetPosition();

	if (moveDir.GetLengthSquared() > 0.f)
	{
		moveDir = moveDir.GetNormalized();
		float speed = m_moveSpeed;
		if (g_theInput->IsKeyDown(KEYCODE_SHIFT))
		{
			speed *= m_sprintMultiplier;
		}

		camPos += moveDir * speed * deltaSeconds;
	}

	m_camera.SetPosition(camPos);
	m_camera.SetOrientation(camOri);
}


void Player::GetColliderWorldCorners(Vec3 outCorners[8]) const
{
	Vec3 localMins = m_collider.m_mins;
	Vec3 localMaxs = m_collider.m_maxs;

	Vec3 mins = localMins + m_position;
	Vec3 maxs = localMaxs + m_position;

	outCorners[0] = Vec3(mins.x, mins.y, mins.z);
	outCorners[1] = Vec3(maxs.x, mins.y, mins.z);
	outCorners[2] = Vec3(mins.x, maxs.y, mins.z);
	outCorners[3] = Vec3(maxs.x, maxs.y, mins.z);

	outCorners[4] = Vec3(mins.x, mins.y, maxs.z);
	outCorners[5] = Vec3(maxs.x, mins.y, maxs.z);
	outCorners[6] = Vec3(mins.x, maxs.y, maxs.z);
	outCorners[7] = Vec3(maxs.x, maxs.y, maxs.z);
}

void Player::UpdateGroundedStateFromWorld()
{
	m_isGrounded = false;

	if (m_physicsMode != WALK)
		return;

	World* world = m_game->m_world;
	if (!world)
		return;

	Vec3 corners[8];
	GetColliderWorldCorners(corners);

	const float probeDist = 0.1f;
	const Vec3  downDir(0.f, 0.f, -1.f);

	for (int i = 0; i < 4; ++i)
	{
		Vec3 start = corners[i] + Vec3(0.f, 0.f, 0.01f); 
		GameRaycastResult3D result;
		if (world->RaycastVsBlocks(start, downDir, probeDist, result))
		{
			m_isGrounded = true;
			return;
		}
	}
}

void Player::ResolveVerticalCollisionUsingRaycast(float deltaSeconds)
{
	if (m_physicsMode == NOCLIP)
	{
		m_position += m_velocity * deltaSeconds;
		return;
	}

	World* world = m_game->m_world;
	if (!world)
	{
		m_position += m_velocity * deltaSeconds;
		return;
	}

	const float skin = 0.02f;

	{
		float deltaX = m_velocity.x * deltaSeconds;
		if (fabsf(deltaX) > 1e-6f)
		{
			Vec3 dir = (deltaX > 0.f) ? Vec3(1.f, 0.f, 0.f) : Vec3(-1.f, 0.f, 0.f);
			float moveDist = fabsf(deltaX);

			Vec3 corners[8];
			GetColliderWorldCorners(corners);

			float closestHit = moveDist + skin;
			bool  hitSomething = false;
			Vec3  hitNormal = Vec3::ZERO;

			int sideIndices[4];
			if (deltaX > 0.f)
			{
				sideIndices[0] = 1;
				sideIndices[1] = 3;
				sideIndices[2] = 5;
				sideIndices[3] = 7;
			}
			else
			{
				sideIndices[0] = 0;
				sideIndices[1] = 2;
				sideIndices[2] = 4;
				sideIndices[3] = 6;
			}

			for (int i = 0; i < 4; ++i)
			{
				Vec3 start = corners[sideIndices[i]];
				GameRaycastResult3D result;
				if (world->RaycastVsBlocks(start, dir, moveDist + skin, result))
				{
					if (result.m_didImpact && result.m_impactDist < closestHit)
					{
						closestHit = result.m_impactDist;
						hitSomething = true;
						hitNormal = result.m_impactNormal;
					}
				}
			}

			if (hitSomething)
			{
				float allowed = Max(0.f, closestHit - skin);
				float sign = (deltaX > 0.f) ? 1.f : -1.f;
				m_position.x += sign * allowed;

				if (hitNormal.x * sign < 0.f)
				{
					m_velocity.x = 0.f;
				}
			}
			else
			{
				m_position.x += deltaX;
			}
		}
	}

	{
		float deltaY = m_velocity.y * deltaSeconds;
		if (fabsf(deltaY) > 1e-6f)
		{
			Vec3 dir = (deltaY > 0.f) ? Vec3(0.f, 1.f, 0.f) : Vec3(0.f, -1.f, 0.f);
			float moveDist = fabsf(deltaY);

			Vec3 corners[8];
			GetColliderWorldCorners(corners);

			float closestHit = moveDist + skin;
			bool  hitSomething = false;
			Vec3  hitNormal = Vec3::ZERO;

			int sideIndices[4];
			if (deltaY > 0.f)
			{
				sideIndices[0] = 2;
				sideIndices[1] = 3;
				sideIndices[2] = 6;
				sideIndices[3] = 7;
			}
			else
			{
				sideIndices[0] = 0;
				sideIndices[1] = 1;
				sideIndices[2] = 4;
				sideIndices[3] = 5;
			}

			for (int i = 0; i < 4; ++i)
			{
				Vec3 start = corners[sideIndices[i]];
				GameRaycastResult3D result;
				if (world->RaycastVsBlocks(start, dir, moveDist + skin, result))
				{
					if (result.m_didImpact && result.m_impactDist < closestHit)
					{
						closestHit = result.m_impactDist;
						hitSomething = true;
						hitNormal = result.m_impactNormal;
					}
				}
			}

			if (hitSomething)
			{
				float allowed = Max(0.f, closestHit - skin);
				float sign = (deltaY > 0.f) ? 1.f : -1.f;
				m_position.y += sign * allowed;

				if (hitNormal.y * sign < 0.f)
				{
					m_velocity.y = 0.f;
				}
			}
			else
			{
				m_position.y += deltaY;
			}
		}
	}

	{
		float deltaZ = m_velocity.z * deltaSeconds;
		if (fabsf(deltaZ) < 1e-6f)
		{
			return;
		}

		Vec3 dir = (deltaZ > 0.f) ? Vec3(0.f, 0.f, 1.f) : Vec3(0.f, 0.f, -1.f);
		float moveDist = fabsf(deltaZ);

		Vec3 corners[8];
		GetColliderWorldCorners(corners); 

		float closestHit = moveDist + skin;
		bool  hitSomething = false;
		Vec3  hitNormal = Vec3::ZERO;

		int startIndex = (deltaZ > 0.f) ? 4 : 0; 
		for (int i = 0; i < 4; ++i)
		{
			Vec3 start = corners[startIndex + i];
			GameRaycastResult3D result;
			if (world->RaycastVsBlocks(start, dir, moveDist + skin, result))
			{
				if (result.m_didImpact && result.m_impactDist < closestHit)
				{
					closestHit = result.m_impactDist;
					hitSomething = true;
					hitNormal = result.m_impactNormal;
				}
			}
		}

		if (hitSomething)
		{
			float allowed = Max(0.f, closestHit - skin);
			float sign = (deltaZ > 0.f) ? 1.f : -1.f;
			m_position.z += sign * allowed;

			if (hitNormal.z * sign < 0.f)
			{
				m_velocity.z = 0.f;

				if (deltaZ < 0.f)
				{
					m_isGrounded = true;
				}
			}
		}
		else
		{
			m_position.z += deltaZ;
			m_isGrounded = false;
		}
	}
}



