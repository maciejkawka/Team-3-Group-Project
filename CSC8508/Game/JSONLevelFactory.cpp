#include "JSONLevelFactory.h"
#include "JSONComponentFactory.h"
#include "JSONShared.h"
#include "Game.h"

#include "../Engine/GameObject.h"
#include "../Engine/CollisionDetection.h"
#include "../../Plugins/json/json.hpp"
#include "../../Common/Assets.h"

#include <fstream>

using namespace NCL;
using namespace CSC8508;

using json = nlohmann::json;

void SetTransformFromJson(Transform& transform, json transformJson)
{
	if (!transformJson.is_object())
		return;

	transform.SetPosition(JSONShared::JsonToVector3(transformJson["position"]));
	transform.SetOrientation(JSONShared::JsonToQuaternion(transformJson["orientation"]));
	transform.SetScale(JSONShared::JsonToVector3(transformJson["scale"]));
}

void SetRenderObjectFromJson(GameObject* gameObject, json renderObjectJson, Game* game)
{
	if (!renderObjectJson.is_object())
		return;

	ResourceManager* resourceManager = game->GetResourceManager();

	MeshGeometry* mesh = resourceManager->LoadMesh(renderObjectJson["mesh"]);

	MeshMaterial* meshMat = nullptr;
	if (renderObjectJson["material"].is_string())
		meshMat = resourceManager->LoadMaterial(renderObjectJson["material"]);

	TextureBase* tex = nullptr;

	if (renderObjectJson["texture"].is_string())
		tex = resourceManager->LoadTexture(renderObjectJson["texture"]);
	else
		tex = resourceManager->LoadTexture("checkerboard.png");

	ShaderBase* shader = resourceManager->LoadShader("GameTechVert.glsl", "GameTechFrag.glsl");//renderObjectJson["vertex"],renderObjectJson["fragment"]);
	gameObject->GetTransform().SetScale(gameObject->GetTransform().GetScale() * (renderObjectJson["renderScale"].is_number() ?renderObjectJson["renderScale"] : 1));

	gameObject->SetRenderObject(new RenderObject(&gameObject->GetTransform(), mesh, meshMat, tex, shader));
}

void SetPhysicsObjectFromJson(GameObject* gameObject, json physicsObjectJson)
{
	if (!physicsObjectJson.is_object() || physicsObjectJson["mass"] == -1)
		return;

<<<<<<< Updated upstream
	PhysicsObject* po = new PhysicsObject(&gameObject->GetTransform(), gameObject->GetBoundingVolume());
	po->SetInverseMass(physicsObjectJson["invMass"]);

	if (physicsObjectJson["inertia"] == "sphere")
		po->InitSphereInertia();
	else if (physicsObjectJson["inertia"] == "cube")
		po->InitCubeInertia();
	else
		po->InitCubeInertia();

	if (physicsObjectJson["isKinematic"] == true)
		gameObject->SetIsStatic(true);
=======
	Transform& transform = gameObject->GetTransform();

	PhysicsObject* po = new PhysicsObject(&gameObject->GetTransform(), gameObject->GetBoundingVolume());
	
	float scaleX = transform.GetScale().x;
	float scaleY = transform.GetScale().x;
	float scaleZ = transform.GetScale().z;
	float capsuleheight = scaleY - scaleZ;
	if (colliderObjectJson["type"] == "box")
		po->body->addBoxShape(transform.GetScale());
	else if (colliderObjectJson["type"] == "sphere")
		po->body->addSphereShape(scaleX);
	else if (colliderObjectJson["type"] == "capsule")
		po->body->addCapsuleShape(scaleZ, capsuleheight);


	float mass = physicsObjectJson["mass"];
	if (abs(mass) < 0.001f || physicsObjectJson["isKinematic"]) mass = 0.0f;

	po->body->createBody(mass, 0.4f, 0.4f, game->GetPhysics());
	po->body->setUserPointer(gameObject);
>>>>>>> Stashed changes

	gameObject->SetPhysicsObject(po);
}

void SetColliderFromJson(GameObject* gameObject, json colliderJson)
{
	if (!colliderJson.is_object())
		return;

	CollisionVolume* volume = nullptr;
	Transform& transform = gameObject->GetTransform();

	if (colliderJson["type"] == "box")
		volume = new AABBVolume(transform.GetScale());
	else if (colliderJson["type"] == "obbbox")
		volume = new OBBVolume(transform.GetScale());
	else if (colliderJson["type"] == "sphere")
		volume = new SphereVolume(transform.GetScale().x);
	else if (colliderJson["type"] == "capsule")
		volume = new CapsuleVolume(colliderJson["halfHeight"],colliderJson["radius"]);

	gameObject->SetBoundingVolume(volume);
}

GameObject* CreateObjectFromJson(json objectJson, Game* game)
{
	if (!objectJson.is_object())
		return nullptr;
	
	GameObject* go = (objectJson["name"].is_string() ? new GameObject(objectJson["name"]) : new GameObject("unnamed"));
	Transform& transform = go->GetTransform();

	SetTransformFromJson(transform, objectJson["transform"]);

	SetColliderFromJson(go, objectJson["collider"]);

	SetRenderObjectFromJson(go, objectJson["render"], game);

	SetPhysicsObjectFromJson(go, objectJson["physics"]);

	go->SetIsStatic(objectJson["isStatic"].is_boolean() ? objectJson["isStatic"] : false);

	for (auto component : objectJson["components"])
	  JSONComponentFactory::AddComponentFromJson(component, go, game);

	return go;
}

void JSONLevelFactory::ReadLevelFromJson(std::string fileName, Game* game)
{
	std::ifstream input{ Assets::LEVELSDIR + fileName };

	json level {};

	input >> level;

	if (!level.is_array())
		throw std::exception("Unable to read level json");

	for (auto obj : level)
		game->AddGameObject(CreateObjectFromJson(obj,game));
}
