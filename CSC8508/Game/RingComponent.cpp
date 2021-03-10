#include "RingComponenet.h"
#include "../Engine/GameObject.h"

using namespace NCL;
using namespace CSC8508;

RingComponent::RingComponent(GameObject* object, int bonus) : Component(object)
{
	//active = true;
	physicsObject = object->GetPhysicsObject();
	physicsObject->body->makeTrigger();
	this->bonus = bonus;
}
