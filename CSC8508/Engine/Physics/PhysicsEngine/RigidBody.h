#pragma once
#include "btBulletDynamicsCommon.h"
//#include "BulletWorld.h"

#include "../../Common/Vector3.h"
#include "../../Common/Quaternion.h"
#include "../../CSC8508/Engine/Transform.h"

//#include "../../CSC8508/Engine/PhysicsObject.h"

namespace NCL 
{
	namespace CSC8508 
	{
		namespace physics
		{
			//class Transform;

			class RigidBody
			{
			public:
				RigidBody(Transform* parentTransform);//PhysicsObject* parentObj);
				~RigidBody();

				void addBoxShape(NCL::Maths::Vector3 halfExtents);
				void addSphereShape( float radius);
				void addCapsuleShape(float radius, float height);
				void addCylinderShape(NCL::Maths::Vector3 halfExtents);
				void addConeShape(float radius, float height);

				void createBody(NCL::Maths::Vector3 SetPosition,
								NCL::Maths::Quaternion SetRotation,
								float mass,
								float restitution,
								float friction);

				btRigidBody* returnBody() { return body; };

				void updateTransform();
				
				void addForce(NCL::Maths::Vector3 force);
				void addTorque(NCL::Maths::Vector3 torque);


			private:

				//PhysicsObject* parent;
				Transform* transform;
				btRigidBody* body;
				btCollisionShape* colShape;

			};
		}
	}
}