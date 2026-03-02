#pragma once
#include "Game/Entity.hpp"
#include "Engine/Core/Rgba8.hpp"
#include <vector>

struct Vertex_PCU;
class Texture;
class Timer;

enum class PropShape
{
	CUBE,
	SPHERE,

	COUNT
};

class Prop : public Entity
{
public:
	Prop(Game* owner, PropShape shape);
	virtual ~Prop();

	virtual void Update(float deltaSeconds) override;
	virtual void Render() const override;

	void SetPosition(Vec3 position);
	void SetOrientation(EulerAngles orientation);
	void UpdateBlink();
	void UpdateRotation(float deltaSeconds);

	void InitializeVerts();

public:
	bool m_isBlink = false;
	Texture* m_texture = nullptr;

private:
	std::vector<Vertex_PCU> m_vertexes;
	Rgba8 m_color = Rgba8::WHITE;
	Timer* m_blinkTimer;
	PropShape m_shape = PropShape::CUBE;
	float m_radius = 1.f;

	bool m_isFading = true;
};