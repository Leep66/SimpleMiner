#include "Game/Entity.hpp"
#include "Game/Game.hpp"

Entity::Entity(Game* owner)
	:m_game(owner)
{
}

Entity::~Entity()
{
	m_game = nullptr;
}

Mat44 Entity::GetModelToWorldTransform() const
{
	Mat44 modelMatrix;

	Mat44 translationMatrix = Mat44::MakeTranslation3D(m_position);
	Mat44 rotationMatrix = m_orientation.GetAsMatrix_IFwd_JLeft_KUp();
	

	modelMatrix.Append(translationMatrix);
	modelMatrix.Append(rotationMatrix);

	return modelMatrix;
}


