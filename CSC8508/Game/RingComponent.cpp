#include "RingComponenet.h"
#include "../Engine/GameObject.h"

using namespace NCL;
using namespace CSC8508;

RingComponent::RingComponent(GameObject* object) : Component(object)
{
	//active = true;
	physicsObject = object->GetPhysicsObject();
	physicsObject->body->makeTrigger();
}

void RingComponent::OnCollisionBegin(GameObject* other)
{
	if (other->HasTag("Player") && this->gameObject->IsActive())
	{
		//add score

		this->gameObject->SetIsActive(false);
	}
}