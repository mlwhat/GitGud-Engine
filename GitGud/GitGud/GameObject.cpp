#include "GameObject.h"

#include "App.h"
#include "M_GoManager.h"
#include "RandGen.h"
#include "JsonFile.h"

#include "Transform.h"
#include "Mesh.h"
#include "Camera.h"
#include "Material.h"
#include "Light.h"

GameObject::GameObject(GameObject * parent, UID uuid) : parent(parent), uuid(uuid)
{
	name.assign("GameObject");
	transform = (Transform*)CreateComponent(CMP_TRANSFORM);
}

GameObject::~GameObject()
{
	for (auto cmp : components)
		RELEASE(cmp);

	for (auto obj : childs)
		RELEASE(obj);
}

UID GameObject::GetUuid() const
{
	return uuid;
}

const char * GameObject::GetName() const
{
	return name.c_str();
}

void GameObject::SetName(const char * str)
{
	if (str)name.assign(str);
}

GameObject * GameObject::CreateChild()
{
	GameObject* ret = nullptr;

	ret = new GameObject(this, app->random->GetRandInt());
	childs.push_back(ret);
	app->goManager->AddDynObject(ret);

	return ret;
}

Component * GameObject::CreateComponent(ComponentType type)
{
	Component* ret = nullptr;
	//TODO: Send error message if already have unique components

	switch (type)
	{
	case CMP_TRANSFORM:
		if (!transform && !(currentCMPs & CMP_TRANSFORM))
		{
			transform = new Transform(this);
			ret = transform;
			currentCMPs |= CMP_TRANSFORM;
		}
		break;
	case CMP_MESH:
		if (!(currentCMPs & CMP_MESH))
		{
			ret = new Mesh(this);
			currentCMPs |= CMP_MESH;
		}
		break;
	case CMP_MATERIAL:
		if (!(currentCMPs & CMP_MATERIAL))
		{
			ret = new Material(this);
			currentCMPs |= CMP_MATERIAL;
		}
		break;
	case CMP_CAMERA:
	{
		ret = new Camera(this);
		if (!(currentCMPs & CMP_CAMERA))
			currentCMPs |= CMP_CAMERA;
	}
	break;
	case CMP_LIGHT:
	{
		ret = new Light(this);
		if (!(currentCMPs & CMP_LIGHT))
			currentCMPs |= CMP_LIGHT;
	}
		break;
	default:
		_LOG(LOG_WARN, "Invalid component type!");
		break;
	}

	if (ret)
	{
		components.push_back(ret);
	}

	return ret;
}

Component * GameObject::GetComponent(ComponentType type)
{
	for (auto cmp : components)
	{
		if (cmp->GetType() == type)
			return cmp;
	}
	return nullptr;
}

void GameObject::GetComponents(ComponentType types, std::vector<Component*>& cmps)
{
	for (auto cmp : components)
	{
		if (types & cmp->GetType())
			cmps.push_back(cmp);
	}
}

bool GameObject::HasComponent(ComponentType type)
{
	for (auto cmp : components)
	{
		if (cmp->GetType() == type)
			return true;
	}
	return false;
}

uint GameObject::CountComponentsOfType(ComponentType type)
{
	uint ret = 0;

	for (auto cmp : components)
	{
		if (cmp->GetType() == type)
			++ret;
	}

	return ret;
}

void GameObject::RemoveChild(GameObject * obj)
{
	if (obj)
	{
		std::vector<GameObject*>::iterator it = std::find(childs.begin(), childs.end(), obj);
		if (it != childs.end())
			childs.erase(it);
	}
}

void GameObject::RemoveAllChilds()
{
	for (auto go : childs)
	{
		app->goManager->FastRemoveGameObject(go);
	}
	childs.clear();
}

GameObject * GameObject::GetParent() const
{
	return parent;
}

void GameObject::SetNewParent(GameObject * parent, bool force)
{
	if (parent == this->parent) return;

	if (parent)
	{
		auto it = std::find(this->parent->childs.begin(), this->parent->childs.end(), this);
		if (it != this->parent->childs.end())
			this->parent->childs.erase(it);
	}

	this->parent = parent;

	if (parent)
		parent->childs.push_back(this);

	wasDirty = true;

	if (force && transform && parent && parent->transform)
	{
		float4x4 tmp = transform->GetGlobalTransform();
		transform->SetLocalTransform(tmp * parent->transform->GetLocalTransform().Inverted());
	}
}

void GameObject::OnDrawDebug()
{
	for (auto it : components)
	{
		if (it->IsActive())
			it->OnDebugDraw();
	}
}

void GameObject::Destroy()
{
	app->goManager->RemoveGameObject(this);
}

void GameObject::RemoveComponent(Component * cmp)
{
	componentsToRemove.push_back(cmp);
}

bool GameObject::IsStatic() const
{
	return isStatic;
}

void GameObject::SetStatic(bool set)
{
	if (set != isStatic)
	{
		if (set)
		{
			app->goManager->EraseDynObj(this);
			app->goManager->InsertToTree(this);
		}
		else
		{
			app->goManager->EraseFromTree(this);
			app->goManager->AddDynObject(this);
		}
		isStatic = set;

		for (auto go : childs)
		{
			go->SetStatic(set);
		}
	}
}

bool GameObject::IsActive() const
{
	return selfActive;
}

void GameObject::SetActive(bool set)
{
	if (set != selfActive)
	{
		if (set) Enable();
		else Disable();
	}
}

void GameObject::SwitchActive()
{
	if (selfActive) Disable();
	else Enable();
}

void GameObject::Enable()
{
	if (!selfActive)
	{
		selfActive = true;
		OnEnable();
	}
}

void GameObject::Disable()
{
	if (selfActive)
	{
		selfActive = false;
		OnDisable();
	}
}

void GameObject::PreUpdate()
{
	if (!componentsToRemove.empty())
	{
		for (auto cmp : componentsToRemove)
		{
			std::vector<Component*>::iterator it = std::find(components.begin(), components.end(), cmp);
			if (it != components.end())
			{
				components.erase(it);
				cmp->OnFinish();

				switch (cmp->GetType())
				{
				case CMP_TRANSFORM:
					currentCMPs &= ~CMP_TRANSFORM;
					break;
				case CMP_MESH:
					currentCMPs &= ~CMP_MESH;
					break;
				case CMP_MATERIAL:
					currentCMPs &= ~CMP_MATERIAL;
					break;
				case CMP_CAMERA:
					if (CountComponentsOfType(CMP_CAMERA) == 1)
						currentCMPs &= ~CMP_CAMERA;
					break;
				case CMP_LIGHT:
					if (CountComponentsOfType(CMP_LIGHT) == 1)
						currentCMPs &= ~CMP_LIGHT;
					break;
				}

				RELEASE(cmp);
			}
		}
		componentsToRemove.clear();
	}
}

void GameObject::Update(float dt)
{
	for (auto cmp : components)
		if(cmp->IsActive())
			cmp->OnPreUpdate(dt);

	for (auto cmp : components)
		if (cmp->IsActive())
			cmp->OnUpdate(dt);
}

void GameObject::OnStart()
{
	for (auto cmp : components)
	{
		cmp->OnStart();
	}
}

void GameObject::OnFinish()
{
	if (isStatic)
	{
		app->goManager->EraseFromTree(this);
	}
	else
	{
		app->goManager->EraseDynObj(this);
	}

	for (auto cmp : components)
	{
		cmp->OnFinish();
	}
}

void GameObject::OnEnable()
{
	for (auto cmp : components)
	{
		cmp->OnEnable();
	}

	for (auto go : childs)
	{
		go->Enable();
	}
}

void GameObject::OnDisable()
{
	for (auto cmp : components)
	{
		cmp->OnDisable();
	}

	for (auto go : childs)
	{
		go->Disable();
	}
}

void GameObject::OnPlay()
{
	for (auto cmp : components)
	{
		cmp->OnPlay();
	}
}

void GameObject::OnStop()
{
	for (auto cmp : components)
	{
		cmp->OnStop();
	}
}

void GameObject::OnPause()
{
	for (auto cmp : components)
	{
		cmp->OnPause();
	}
}

void GameObject::OnUnPause()
{
	for (auto cmp : components)
	{
		cmp->OnUnPause();
	}
}

void GameObject::OnGameObjectDestroyed()
{
	for (auto cmp : components)
	{
		cmp->OnGameObjectDestroyed();
	}
}

void GameObject::RecCalcTransform(const float4x4 & parentTrans, bool force)
{
	if (transform && (transform->localTransformHasChanged || force))
	{
		if (isStatic)
			SetStatic(false);

		force = true;
		wasDirty = true;
		transform->UpdateTransform(parentTrans);

		for (auto cmp : components)
		{
			cmp->OnTransformUpdate(transform);
		}
	}
	else
	{
		wasDirty = false;
	}

	for (auto go : childs)
	{
		if (go && transform)
		{
			go->RecCalcTransform(transform->GetGlobalTransform(), force);
		}
	}
}

void GameObject::RecCalcBoxes()
{
	if (wasDirty)
	{
		RecalcBox();

		orientedBox = enclosingBox;
		if (orientedBox.IsFinite() && transform)
		{
			orientedBox.Transform(transform->GetGlobalTransform());
			enclosingBox.SetFrom(orientedBox);
		}
	}

	for (auto go : childs)
	{
		if (go && transform)
			go->RecCalcBoxes();
	}
}

void GameObject::RecalcBox()
{
	enclosingBox.SetNegativeInfinity();

	for (auto cmp : components)
	{
		if (cmp->IsActive())
			cmp->GetBox(enclosingBox);
	}
}

bool GameObject::OnSaveGo(JsonFile & sect, std::map<uint, uint>* duplicate) const
{
	bool ret = true;

	JsonFile file;

	UID idToSave = uuid;
	UID parentID = parent ? parent->GetUuid() : 0;

	if (duplicate)
	{
		if (duplicate->find(idToSave) != duplicate->end())
		{
			idToSave = app->random->GetRandInt();
			(*duplicate)[uuid] = idToSave;

			auto it = duplicate->find(parentID);
			if (it != duplicate->end())
				parentID = it->second;
		}
	}

	file.AddInt("UID", idToSave);
	file.AddInt("parent_id", parentID);

	file.AddString("name", name.c_str());

	for (auto it : components)
	{
		if (it)
		{
			JsonFile cmp;
			cmp.AddInt("cmp_type", (int)it->GetType());
			cmp.AddBool("active", it->IsActive());
			cmp.AddBool("go_id", it->GetGameObject()->GetUuid());

			it->OnSaveCmp(cmp);
			file.AppendArrayValue("components", cmp.Value());
		}
	}

	sect.AppendArrayValue("game_objects", file.Value());

	for (auto it : childs)
		if (it)
			it->OnSaveGo(sect, duplicate);

	return ret;
}

bool GameObject::OnLoadGo(JsonFile * sect, std::map<GameObject*, uint>& relations)
{
	bool ret = true;

	uuid = sect->GetInt("UID", 0);
	UID dad = sect->GetInt("parent_id", 0);
	relations[this] = dad;

	name = sect->GetString("name", "unnamed");

	int cmpCount = sect->GetArraySize("components");
	for (uint i = 0; i < cmpCount; ++i)
	{
		JsonFile cmp = sect->GetObjectFromArray("components", i);
		ComponentType type = (ComponentType)cmp.GetInt("cmp_type", 0);
		if (type != CMP_UNKNOWN)
		{
			if (type == CMP_TRANSFORM)
				transform->OnLoadCmp(&cmp);
			else
				CreateComponent(type)->OnLoadCmp(&cmp);
		}
		else
		{
			_LOG(LOG_WARN, "Unknown component type for game object [%s].", name.c_str());
		}
	}

	return ret;
}

const char * GameObject::GetTag() const
{
	return tag.c_str();
}

void GameObject::SetTag(const char * _tag)
{
	tag = _tag;
}

Layer GameObject::GetLayer() const
{
	return layer;
}

void GameObject::SetLayer(Layer _layer)
{
	layer = _layer;
}
