#include "M_GoManager.h"

#include "App.h"
#include "RandGen.h"
#include "JsonFile.h"

#include "M_FileSystem.h"

#include "GGOctree.h"

#include "GameObject.h"
#include "Component.h"
#include "Transform.h"
#include "Camera.h"
#include "Mesh.h"

#include "ResourceMesh.h"

#define OCTREE_SIZE 100 / 2

M_GoManager::M_GoManager(const char* name, bool startEnabled) : Module(name, startEnabled)
{
	_LOG(LOG_INFO, "GoManager: Creation.");

	root = new GameObject(nullptr, 0);
	root->SetName("SceneRoot");

	configuration = M_INIT | M_START | M_PRE_UPDATE | M_UPDATE | M_CLEAN_UP | M_SAVE_CONFIG | M_RESIZE_EVENT | M_DRAW_DEBUG;
}


M_GoManager::~M_GoManager()
{
	_LOG(LOG_INFO, "GoManager: Destroying.");
	RELEASE(root);
}

bool M_GoManager::Init(JsonFile * conifg)
{
	_LOG(LOG_INFO, "GoManager: Init.");

	octreeSize = conifg->GetInt("octree_size", OCTREE_SIZE);
	
	octree = new GGOctree();
	octree->Create(AABB::FromCenterAndSize(float3(0, 0, 0), float3(octreeSize, octreeSize, octreeSize)));

	return true;
}

bool M_GoManager::Start()
{
	_LOG(LOG_INFO, "GoManager: Start.");

	GameObject* go = CreateGameObject();
	Mesh* m = (Mesh*)go->CreateComponent(CMP_MESH);
	m->SetResource(2);

	return true;
}

UpdateReturn M_GoManager::PreUpdate(float dt)
{
	if (!objectsToDelete.empty())
	{
		for (auto obj : objectsToDelete)
		{
			if (obj == selected)
				selected = nullptr;

			obj->OnFinish();
			RELEASE(obj);
			//TODO: Event on obj destroyed
		}

		objectsToDelete.clear();
	}

	if (root)
	{
		if (anyGOTransHasChanged)
		{
			root->RecCalcTransform(root->transform->GetLocalTransform());
			root->RecCalcBoxes();
			anyGOTransHasChanged = false;
		}

		if (mustSave)
		{
			SaveSceneNow();
			mustSave = false;
		}

		if (mustLoad)
		{
			LoadSceneNow();
			mustLoad = false;
		}

		for (auto obj : root->childs)
		{
			DoPreUpdate(obj);
		}
	}

	return UPDT_CONTINUE;
}

UpdateReturn M_GoManager::Update(float dt)
{
	if (root)
	{
		for (auto go : root->childs)
		{
			if (go->IsActive())
				DoUpdate(go, dt);
		}
	}

	return UPDT_CONTINUE;
}

bool M_GoManager::CleanUp()
{
	_LOG(LOG_INFO, "GoManager: CleanUp.");
	RELEASE(octree);
	return true;
}

void M_GoManager::DrawDebug()
{
	if (root)
	{
		for (auto go : root->childs)
		{
			if (go->IsActive())
				DoOnDrawDebug(go);
		}
	}
}

GameObject * M_GoManager::GetRoot() const
{
	return root;
}

GameObject * M_GoManager::GetGOFromUid(UID uuid) const
{
	return GetGoFromUID(root, uuid);
}

GameObject * M_GoManager::GetSelected() const
{
	return selected;
}

void M_GoManager::SelectGo(GameObject * go)
{
	selected = go;
}

GameObject * M_GoManager::CreateGameObject(GameObject * parent)
{
	GameObject* ret = nullptr;

	if (parent)
		ret = parent->CreateChild();
	else
		ret = root->CreateChild();

	return ret;
}

void M_GoManager::InsertToTree(GameObject * object)
{
	octree->Insert(object);
}

void M_GoManager::EraseFromTree(GameObject * object)
{
	octree->Erase(object);
}

void M_GoManager::AddDynObject(GameObject * obj)
{
	dynamicGameObjects.push_back(obj);
}

void M_GoManager::EraseDynObj(GameObject * obj)
{
	std::list<GameObject*>::iterator it = std::find(dynamicGameObjects.begin(), dynamicGameObjects.end(), obj);
	if (it != dynamicGameObjects.end()) dynamicGameObjects.erase(it);
}

void M_GoManager::RemoveGameObject(GameObject * obj)
{
	if (obj)
	{
		if (obj->GetParent())
			obj->GetParent()->RemoveChild(obj);
		obj->RemoveAllChilds();

		objectsToDelete.push_back(obj);
	}
}

void M_GoManager::FastRemoveGameObject(GameObject * obj)
{
	if (obj)
	{
		obj->RemoveAllChilds();
		objectsToDelete.push_back(obj);
	}
}

void M_GoManager::GetToDrawStaticObjects(std::vector<GameObject*>& objects, Camera * cam)
{
	if(cam)
		octree->CollectCandidates(objects, cam->frustum);
}

std::list<GameObject*>* M_GoManager::GetDynamicObjects()
{
	return &dynamicGameObjects;
}

void M_GoManager::AddLight(Light * l)
{
	lights.push_back(l);
}

void M_GoManager::RemoveLight(Light * l)
{
	std::list<Light*>::iterator it;
	for (it = lights.begin(); it != lights.end(); ++it)
	{
		if (*it == l)
			break;
	}
	if(it != lights.end())
		lights.erase(it);
}

std::list<Light*>* M_GoManager::GetLightsList()
{
	return &lights;
}

void M_GoManager::SaveScene()
{
	mustSave = true;
}

void M_GoManager::LoadScene()
{
	mustLoad = true;
}

GameObject * M_GoManager::CastRay(const LineSegment & segment, float & distance) const
{
	distance = inf;
	GameObject* candidate = nullptr;
	RecursiveTestRay(segment, distance, &candidate);
	return candidate;
}

GameObject * M_GoManager::CastRay(const Ray & ray, float & distance) const
{
	distance = inf;
	GameObject* candidate = nullptr;
	RecursiveTestRay(ray, distance, &candidate);
	return candidate;
}

void M_GoManager::OnPlay()
{
	for (auto obj : root->childs)
	{
		DoOnPlay(obj);
	}
}

void M_GoManager::DoOnPlay(GameObject * obj)
{
	if (obj && obj->IsActive())
	{
		obj->OnPlay();

		for (auto go : obj->childs)
		{
			DoOnPlay(go);
		}
	}
}

void M_GoManager::OnStop()
{
	for (auto obj : root->childs)
	{
		DoOnPlay(obj);
	}
}

void M_GoManager::DoOnStop(GameObject * obj)
{
	if (obj && obj->IsActive())
	{
		obj->OnStop();

		for (auto go : obj->childs)
		{
			DoOnStop(go);
		}
	}
}

void M_GoManager::OnPause()
{
	for (auto obj : root->childs)
	{
		DoOnPause(obj);
	}
}

void M_GoManager::DoOnPause(GameObject * obj)
{
	if (obj && obj->IsActive())
	{
	obj->OnPause();

	for (auto go : obj->childs)
	{
		DoOnPause(go);
	}
	}
}

void M_GoManager::OnUnPause()
{
	for (auto obj : root->childs)
	{
		DoOnUnPause(obj);
	}
}

void M_GoManager::DoOnUnPause(GameObject * obj)
{
	if (obj && obj->IsActive())
	{
		obj->OnUnPause();

		for (auto go : obj->childs)
		{
			DoOnUnPause(go);
		}
	}
}

void M_GoManager::DoPreUpdate(GameObject * obj)
{
	if (obj)
	{
		obj->PreUpdate();

		for (auto go : obj->childs)
		{
			DoPreUpdate(go);
		}
	}
}

void M_GoManager::DoUpdate(GameObject * obj, float dt)
{
	if (obj && obj->IsActive())
	{
		obj->Update(dt);

		for (auto go : obj->childs)
		{
			DoUpdate(go, dt);
		}
	}
}

void M_GoManager::DoOnDrawDebug(GameObject* obj)
{
	if (obj && obj->IsActive())
	{
		obj->OnDrawDebug();

		for (auto go : obj->childs)
			DoOnDrawDebug(go);
	}
}

void M_GoManager::RecursiveTestRay(const LineSegment & segment, float & distance, GameObject ** best)const
{
	std::map<float, GameObject*> objects;
	octree->CollectIntersections(objects, segment);

	for (const auto& it : dynamicGameObjects)
	{
		float nearHit, farHit;
		if (segment.Intersects(it->enclosingBox, nearHit, farHit))
			objects[nearHit] = it;
	}

	for (std::map<float, GameObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject* go = it->second;
		std::vector<Component*> meshes;
		go->GetComponents(CMP_MESH, meshes);

		if (meshes.size() > 0)
		{
			const Mesh* m = (const Mesh*)meshes[0];
			const ResourceMesh* r = (const ResourceMesh*)m->GetResource();

			if (r)
			{
				LineSegment localSeg(segment);
				localSeg.Transform(go->transform->GetGlobalTransform().Inverted());

				Triangle tri;
				for (uint i = 0; i < r->numIndices;)
				{
					tri.a.Set(&r->vertices[r->indices[i++]*3]);
					tri.b.Set(&r->vertices[r->indices[i++] * 3]);
					tri.c.Set(&r->vertices[r->indices[i++] * 3]);

					float dist;
					float3 hitPoint;
					if (localSeg.Intersects(tri, &dist, &hitPoint))
					{
						if (dist < distance)
						{
							distance = dist;
							*best = go;
						}
					}
				}
			}
		}
	}
}

void M_GoManager::RecursiveTestRay(const Ray & ray, float & distance, GameObject ** best) const
{
	std::map<float, GameObject*> objects;
	octree->CollectIntersections(objects, ray);

	for (const auto& it : dynamicGameObjects)
	{
		float nearHit, farHit;
		if (ray.Intersects(it->enclosingBox, nearHit, farHit))
			objects[nearHit] = it;
	}

	for (std::map<float, GameObject*>::const_iterator it = objects.begin(); it != objects.end(); ++it)
	{
		GameObject* go = it->second;
		std::vector<Component*> meshes;
		go->GetComponents(CMP_MESH, meshes);

		if (meshes.size() > 0)
		{
			const Mesh* m = (const Mesh*)meshes[0];
			const ResourceMesh* r = (const ResourceMesh*)m->GetResource();

			if (r)
			{
				Ray localRay(ray);
				localRay.Transform(go->transform->GetGlobalTransform().Inverted());
				localRay.dir.Normalize();

				Triangle tri;
				for (uint i = 0; i < r->numIndices;)
				{
					tri.a.Set(&r->vertices[r->indices[i++] * 3]);
					tri.b.Set(&r->vertices[r->indices[i++] * 3]);
					tri.c.Set(&r->vertices[r->indices[i++] * 3]);

					float dist;
					float3 hitPoint;
					if (localRay.Intersects(tri, &dist, &hitPoint))
					{
						if (dist < distance)
						{
							distance = dist;
							*best = go;
						}
					}
				}
			}
		}
	}
}

GameObject * M_GoManager::GetGoFromUID(GameObject * obj, UID uuid) const
{
	if (!obj || uuid == 0)
		return nullptr;

	if (obj->GetUuid() == uuid)
		return obj;

	GameObject* ret = nullptr;

	for (std::vector<GameObject*>::iterator it = obj->childs.begin(); it != obj->childs.end() && ret == nullptr; ++it)
	{
		ret = GetGoFromUID((*it), uuid);
	}

	return ret;
}

void M_GoManager::SaveSceneNow()
{
	bool ret = false;

	//TODO: Resource scene organitzation!!

	JsonFile scene;

	for (auto it : root->childs)
	{
		if (it)
			ret = it->OnSaveGo(scene);
	}

	if (ret)
	{
		auto buffer = scene.Write(true); // TODO: fast
		if (buffer.size() > 0)
		{
			std::string path(SCENE_SAVE_PATH);
			path.append("test_scene.json");

			if (app->fs->Save(path.c_str(), buffer.c_str(), buffer.size()) != buffer.size())
			{
				_LOG(LOG_ERROR, "Error while saving scene.");
			}
			else
			{
				ret = true;
				_LOG(LOG_INFO, "Just saved scene into [%s].", path.c_str());
			}
		}
	}

}

void M_GoManager::LoadSceneNow()
{
	bool ret = false;

	//TODO: Resource scene organitzation!!

	std::string path(SCENE_SAVE_PATH);
	path.append("test_scene.json");

	char* buffer = nullptr;
	uint size = app->fs->Load(path.c_str(), &buffer);

	if (buffer && size > 0)
	{
		JsonFile scene(buffer);

		int goCount = scene.GetArraySize("game_objects");
		std::map<GameObject*, uint> relations;
		for (int i = 0; i < goCount; ++i)
		{
			GameObject* go = CreateGameObject();
			go->OnLoadGo(&scene.GetObjectFromArray("game_objects", i), relations);
		}

		for (auto it : relations)
		{
			UID parentID = it.second;
			GameObject* go = it.first;

			if (parentID == 0)
			{
				go->SetNewParent(root);
			}
			else
			{
				GameObject* dad = GetGOFromUid(parentID);
				if (dad) go->SetNewParent(dad);
			}
		}

		root->RecCalcTransform(root->transform->GetLocalTransform(), true);
		root->RecalcBox();

		for (auto it : relations)
			if (it.first) it.first->OnStart();
	}

	RELEASE_ARRAY(buffer);

	_LOG(LOG_INFO, "Scene loaded [%s].", path.c_str());
}
