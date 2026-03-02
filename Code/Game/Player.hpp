#pragma once
#include "Game/Entity.hpp"
#include "Engine/Renderer/Camera.hpp"
#include "Engine/Math/AABB3.hpp"
#include "Engine/Core/Vertex_PCU.hpp"
#include <string>
#include <vector>

enum PhysicsMode
{
	WALK,
	FLY,
	NOCLIP
};

enum CameraMode
{
	FIRST_PERSON,
	OVER_THE_SHOULDER,
	FIXED_ANGLE_TRACKING,
	INDEPENDENT,
	SPECTATOR_FULL,
	SPECTATOR_XY
};

class Player : public Entity
{
public:
	Player(Game* owner);
	virtual ~Player();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	void UpdatePhysics(float deltaSeconds);
	void UpdateCamera(float deltaSeconds);

	void HandleInput(float deltaSeconds);

	/*
	void MoveByKeyboard(float deltaSeconds);
	void RotateByKeyboard(float deltaSeconds);

	void MoveByController(float deltaSeconds);
	void RotateByController(float deltaSeconds);
	*/

	void HandleControllerButtons();

	void SetPosition(Vec3 pos);
	void SetOrientation(EulerAngles ori);

	Camera GetCamera() const { return m_camera; }
	Camera* GetCameraPtr() { return &m_camera; }
	Vec3 GetPosition() const { return m_position; }
	EulerAngles GetOrientation() const { return m_orientation; }
	Vec3 GetFwdNormal() const;
	Vec3 GetLeftNormal() const;
	Vec3 GetUpNormal() const;
	
	std::string GetPhysicsMode() const;
	void TogglePhysicsMode();

	std::string GetCameraMode() const;
	void ToggleCameraMode();

	void HandlePlayerInput(float deltaSeconds);
	void HandleSpectatorCameraInput(float deltaSeconds);

	void GetColliderWorldCorners(Vec3 outCorners[8]) const;
	void UpdateGroundedStateFromWorld();
	void ResolveVerticalCollisionUsingRaycast(float deltaSeconds);

	bool IsInFixedAngleTrackingMode() const { return m_cameraMode == FIXED_ANGLE_TRACKING; }
	EulerAngles GetAim() const { return m_aim; }
	Vec3 GetEyePosition() const { return m_position + Vec3(0.f, 0.f, m_eyeHeight); }

	Camera m_camera;

private:
	float m_moveSpeed = 4.0f;
	float m_sprintMultiplier = 20.0f;
	float m_sensitivity = 0.075f; 
	float m_rollSpeed = 90.0f;
	
	PhysicsMode m_physicsMode = PhysicsMode::WALK;
	CameraMode m_cameraMode = FIRST_PERSON;
	AABB3 m_collider = AABB3(Vec3(-0.3f, -0.3f, 0.f), Vec3(0.3f, 0.3f, 1.8f));
	std::vector<Vertex_PCU> m_verts;

	bool  m_isGrounded = false;

	float m_walkMaxSpeed = 4.f;
	float m_flyMaxSpeed = 200.f;
	float m_walkAccel = 30.f;
	float m_airAccel = 10.f;
	float m_walkFriction = 20.f;
	float m_airFriction = 1.f; 
	float m_gravity = -9.8f;
	float m_jumpSpeed = 5.f;
	float m_eyeHeight = 1.65f;

	EulerAngles m_aim;

	float m_overShoulderDistance = 4.0f;
	float m_overShoulderHeight = 0.f;

	float m_fixedYawDegrees = 40.0f;
	float m_fixedPitchDegrees = 30.0f;
	float m_fixedTrackDistance = 10.0f;
};