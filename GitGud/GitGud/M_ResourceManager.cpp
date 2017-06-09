#include "M_ResourceManager.h"

#include "App.h"
#include "RandGen.h"
#include "M_FileSystem.h"

#include "Resource.h"
#include "ResourceMesh.h"
#include "ResourceTexture.h"
#include "ResourceMaterial.h"
#include "ResourceScene.h"
#include "ResourceShader.h"

#include "ImporterMesh.h"
#include "ImporterTexture.h"
#include "ImporterMaterial.h"
#include "ImporterScene.h"
#include "ImporterShader.h"

#include "JsonFile.h"

#define RESERVED_RESOURCES 20


M_ResourceManager::M_ResourceManager(const char* name, bool startEnabled) : Module(name, startEnabled)
{
	_LOG("Resource manager: Creation.");

	meshImporter = new ImporterMesh();
	textureImporter = new ImporterTexture();
	materialImporter = new ImporterMaterial();
	sceneImporter = new ImporterScene();
	shaderImporter = new ImporterShader();
}


M_ResourceManager::~M_ResourceManager()
{
	_LOG("Resource manager: Destroying.");

	RELEASE(meshImporter);
	RELEASE(textureImporter);
	RELEASE(materialImporter);
	RELEASE(sceneImporter);
	RELEASE(shaderImporter);
}

bool M_ResourceManager::Init(JsonFile * conf)
{
	_LOG("Resource manager: Init.");

	resourceFile = conf->GetString("resource_file", "resources.json");

	return true;
}

bool M_ResourceManager::Start()
{
	_LOG("Resource manager: Start.");

	//ImportFile("Data/Assets/Models/MechaT.fbx");
	LoadBasicResources();
	LoadResources();

	return true;
}

UPDATE_RETURN M_ResourceManager::PreUpdate(float dt)
{
	return UPDT_CONTINUE;
}

bool M_ResourceManager::CleanUp()
{
	_LOG("Resource manager: CleanUp.");

	SaveResources();

	return true;
}

UID M_ResourceManager::ImportFile(const char * fileName, bool checkFirst)
{
	UID ret = 0;

	//TODO: Must do something to check if file is out assets folder and react to that

	std::string original(fileName);
	std::string path, file, ext;
	app->fs->NormalizePath(original);
	app->fs->SplitPath(original.c_str(), &path, &file, &ext);

	if (checkFirst)
	{
		Resource* tmp = FindResourceFromOriginalFileName(file.c_str());
		if (tmp)
			return tmp->GetUID();
	}

	RESOURCE_TYPE type = GetTypeFromExtension(ext.c_str());

	bool success = false;
	std::string exported;
	UID resid = 0;

	switch (type)
	{
	case RES_MESH:
		//TODO
		break;
	case RES_TEXTURE:
		break;
	case RES_MATERIAL:
		break;
	case RES_SCENE:
		//TODO: Paths
		success = sceneImporter->Import(fileName, exported, ext.c_str(), resid);
		break;
	case RES_SHADER:
		break;
	}

	if (success)
	{
		Resource* r = CreateResource(type, resid);
		r->originalFile = file;
		std::string exp;
		app->fs->NormalizePath(exported);
		app->fs->SplitPath(exported.c_str(), nullptr, &exp);
		r->exportedFile = exp;
		ret = r->GetUID();

		_LOG("Imported file [%s] to [%s].", r->GetOriginalFile(), r->GetExportedFile());
	}
	else
	{
		_LOG("Could not import file [%s].", fileName);
	}
	
	return ret;
}

UID M_ResourceManager::ImportBuf(const void * buffer, uint size, RESOURCE_TYPE type, const char * sourceFile)
{
	UID ret = 0;

	if (!buffer)
		return ret;

	bool succes = false;
	std::string output;

	switch (type)
	{
	case RES_MESH:
		succes = meshImporter->ImportMesh((const aiMesh*)buffer, output, ret);
		break;
	case RES_TEXTURE:
		
		break;
	case RES_MATERIAL:
		succes = materialImporter->Import((const aiMaterial*)buffer, output, ret, sourceFile);
		break;
	case RES_SCENE:
		break;
	case RES_SHADER:
		break;
	}

	if (succes && ret != 0)
	{
		Resource* res = CreateResource(type, ret);
		if (sourceFile)
		{
			res->originalFile = sourceFile;
			app->fs->NormalizePath(res->originalFile);
		}
		res->exportedFile = output;
		_LOG("Imported a buffer succesfully [%s].", output.c_str());
	}
	else
	{
		_LOG("ERROR: Could not import the buffer.");
	}

	return ret;
}

Resource * M_ResourceManager::GetResourceFromUID(UID uuid)
{
	std::map<UID, Resource*>::iterator it = resources.find(uuid);
	return it != resources.end() ? it->second : nullptr;
}

Resource * M_ResourceManager::CreateResource(RESOURCE_TYPE type, UID forceUID)
{
	Resource* ret = nullptr;

	if (forceUID == 0 || GetResourceFromUID(forceUID))
		forceUID = GetNewUID();

	switch (type)
	{
	case RES_MESH:
		ret = (Resource*)new ResourceMesh(forceUID);
		break;
	case RES_TEXTURE:
		ret = (Resource*)new ResourceTexture(forceUID);
		break;
	case RES_MATERIAL:
		ret = (Resource*)new ResourceMaterial(forceUID);
		break;
	case RES_SCENE:
		ret = (Resource*)new ResourceScene(forceUID);
		break;
	case RES_SHADER:
		ret = (Resource*)new ResourceShader(forceUID);
		break;
	}

	if (ret)
		resources[forceUID] = ret;

	return ret;
}

Resource * M_ResourceManager::FindResourceFromOriginalFileName(const char * name)
{
	for (auto it : resources)
	{
		if (it.second->originalFile == name)
			return it.second;
	}

	return nullptr;
}

RESOURCE_TYPE M_ResourceManager::GetTypeFromExtension(const char * ext) const
{
	RESOURCE_TYPE ret = RES_NONE;

	if (strcmp(ext, "png") == 0)
		ret = RES_TEXTURE;
	else if (strcmp(ext, "jpg") == 0)
		ret = RES_TEXTURE;
	else if (strcmp(ext, "tga") == 0)
		ret = RES_TEXTURE;
	else if (strcmp(ext, "ggtex") == 0)
		ret = RES_TEXTURE;
	else if (strcmp(ext, "fbx") == 0)
		ret = RES_SCENE;
	else if (strcmp(ext, "obj") == 0)
		ret = RES_SCENE;
	else if (strcmp(ext, "ggmesh") == 0)
		ret = RES_MESH;
	else if (strcmp(ext, "ggmat") == 0)
		ret = RES_MATERIAL;

	//TODO

	return ret;
}

UID M_ResourceManager::GetNewUID() const
{
	return app->random->GetRandInt();
}

void M_ResourceManager::GetResourcesOfType(std::vector<Resource*>& res, RESOURCE_TYPE type) const
{
	for (auto it : resources)
	{
		if (it.second->GetType() == type)
			res.push_back(it.second);
	}
}

void M_ResourceManager::LoadResources()
{
	char* buffer = nullptr;
	uint size = app->fs->Load((RESOURCES_PATH + resourceFile).c_str(), &buffer);

	if (buffer && size > 0)
	{
		JsonFile file(buffer);

		uint count = file.GetArraySize("resources");
		for (uint i = 0; i < count; ++i)
		{
			JsonFile r(file.GetArray("resources", i));
			RESOURCE_TYPE type = (RESOURCE_TYPE)r.GetInt("type", RES_NONE);
			UID id = r.GetInt("UID", 0);

			if (GetResourceFromUID(id))
				continue;

			Resource* res = CreateResource(type, id);
			res->Load(r);
		}
	}

	RELEASE_ARRAY(buffer);
}

void M_ResourceManager::SaveResources()
{
	JsonFile save;
	save.AddArray("resources");

	std::map<UID, Resource*>::iterator it = resources.begin();
	for (; it != resources.end(); ++it)
	{
		if (it->first > RESERVED_RESOURCES)
		{
			JsonFile res;
			it->second->Save(res);
			save.AddArrayEntry(res);
		}
	}

	char* buffer = nullptr;
	uint size = save.WriteJson(&buffer, false);

	if (app->fs->Save((RESOURCES_PATH + resourceFile).c_str(), buffer, size) != size)
	{
		_LOG("Resource manager ERROR: Could no save resources!");
	}

	RELEASE_ARRAY(buffer);
}

void M_ResourceManager::LoadBasicResources()
{
	checkers = (ResourceTexture*)CreateResource(RES_TEXTURE, 1);
	textureImporter->LoadChequers(checkers);
	checkers->AddInstance();

	cube = (ResourceMesh*)CreateResource(RES_MESH, 2);
	meshImporter->LoadCube(cube);
	cube->AddInstance();

	quad = (ResourceMesh*)CreateResource(RES_MESH, 3);
	meshImporter->LoadQuad(quad);
	quad->AddInstance();

	plane = (ResourceMesh*)CreateResource(RES_MESH, 4);
	meshImporter->LoadPlane(plane);
	plane->AddInstance();

	sphere = (ResourceMesh*)CreateResource(RES_MESH, 5);
	meshImporter->LoadSphere(sphere);
	sphere->AddInstance();


}
