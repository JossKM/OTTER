#include "Scene.h"

#include <GLFW/glfw3.h>
#include <locale>
#include <codecvt>

#include "Utils/FileHelpers.h"
#include "Utils/GlmBulletConversions.h"

#include "Gameplay/Physics/RigidBody.h"
#include "Gameplay/Physics/TriggerVolume.h"
#include "Gameplay/MeshResource.h"

#include "Graphics/DebugDraw.h"
#include "Graphics/TextureCube.h"
#include "Graphics/VertexArrayObject.h"
#include "Application/Application.h"

namespace Gameplay {
	Scene::Scene() :
		_objects(std::vector<GameObject::Sptr>()),
		_deletionQueue(std::vector<std::weak_ptr<GameObject>>()),
		Lights(std::vector<Light>()),
		IsPlaying(false),
		MainCamera(nullptr),
		DefaultMaterial(nullptr),
		_isAwake(false),
		_filePath(""),
		_skyboxShader(nullptr),
		_skyboxMesh(nullptr),
		_skyboxTexture(nullptr),
		_skyboxRotation(glm::mat3(1.0f)),
		_gravity(glm::vec3(0.0f, 0.0f, -9.81f))
	{
		_lightingUbo = std::make_shared<UniformBuffer<LightingUboStruct>>();
		_lightingUbo->GetData().AmbientCol = glm::vec3(0.1f);
		_lightingUbo->Update();
		_lightingUbo->Bind(LIGHT_UBO_BINDING_SLOT);

		_InitPhysics();

	}

	Scene::~Scene() {
		_objects.clear();
		_CleanupPhysics();
	}

	void Scene::SetPhysicsDebugDrawMode(BulletDebugMode mode) {
		_bulletDebugDraw->setDebugMode((btIDebugDraw::DebugDrawModes)mode);
	}

	void Scene::SetSkyboxShader(const std::shared_ptr<ShaderProgram>& shader) {
		_skyboxShader = shader;
	}

	std::shared_ptr<ShaderProgram> Scene::GetSkyboxShader() const {
		return _skyboxShader;
	}

	void Scene::SetSkyboxTexture(const std::shared_ptr<TextureCube>& texture) {
		_skyboxTexture = texture;
	}

	std::shared_ptr<TextureCube> Scene::GetSkyboxTexture() const {
		return _skyboxTexture;
	}

	void Scene::SetSkyboxRotation(const glm::mat3& value) {
		_skyboxRotation = value;
		_lightingUbo->GetData().EnvironmentRotation = value;
		_lightingUbo->Update();
	}

	const glm::mat3& Scene::GetSkyboxRotation() const {
		return _skyboxRotation;
	}

	GameObject::Sptr Scene::CreateGameObject(const std::string& name)
	{
		GameObject::Sptr result(new GameObject());
		result->Name = name;
		result->_scene = this;
		result->_selfRef = result;
		_objects.push_back(result);
		return result;
	}

	void Scene::RemoveGameObject(const GameObject::Sptr& object) {
		_deletionQueue.push_back(object);
	}

	GameObject::Sptr Scene::FindObjectByName(const std::string name) const {
		auto it = std::find_if(_objects.begin(), _objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->Name == name;
		});
		return it == _objects.end() ? nullptr : *it;
	}

	GameObject::Sptr Scene::FindObjectByGUID(Guid id) const {
		auto it = std::find_if(_objects.begin(), _objects.end(), [&](const GameObject::Sptr& obj) {
			return obj->_guid == id;
		});
		return it == _objects.end() ? nullptr : *it;
	}

	void Scene::SetAmbientLight(const glm::vec3& value) {
		_lightingUbo->GetData().AmbientCol = glm::vec3(0.1f);
		_lightingUbo->Update();
	}

	const glm::vec3& Scene::GetAmbientLight() const { 
		return _lightingUbo->GetData().AmbientCol;
	}

	void Scene::Awake() {
		// Not a huge fan of this, but we need to get window size to notify our camera
		// of the current screen size
		Application& app = Application::Get();
		glm::ivec2 windowSize = app.GetWindowSize();
		MainCamera->ResizeWindow(windowSize.x, windowSize.y);

		if (_skyboxMesh == nullptr) {
			_skyboxMesh = ResourceManager::CreateAsset<MeshResource>();
			_skyboxMesh->AddParam(MeshBuilderParam::CreateCube(glm::vec3(0.0f), glm::vec3(1.0f)));
			_skyboxMesh->AddParam(MeshBuilderParam::CreateInvert());
			_skyboxMesh->GenerateMesh();
		}

		// Call awake on all gameobjects
		for (auto& obj : _objects) {
			obj->Awake();
		}
		// Set up our lighting 
		SetupShaderAndLights();

		_isAwake = true;
	}

	void Scene::DoPhysics(float dt) {
		ComponentManager::Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
			body->PhysicsPreStep(dt);
		});
		ComponentManager::Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
			body->PhysicsPreStep(dt);
		});

		if (IsPlaying) {

			_physicsWorld->stepSimulation(dt, 15);

			ComponentManager::Each<Gameplay::Physics::RigidBody>([=](const std::shared_ptr<Gameplay::Physics::RigidBody>& body) {
				body->PhysicsPostStep(dt);
			});
			ComponentManager::Each<Gameplay::Physics::TriggerVolume>([=](const std::shared_ptr<Gameplay::Physics::TriggerVolume>& body) {
				body->PhysicsPostStep(dt);
			});
		}
	}

	void Scene::DrawPhysicsDebug() {
		if (_bulletDebugDraw->getDebugMode() != btIDebugDraw::DBG_NoDebug) {
			_physicsWorld->debugDrawWorld();
			DebugDrawer::Get().FlushAll();
		}
	}

	void Scene::Update(float dt) {
		_FlushDeleteQueue();
		if (IsPlaying) {
			for (auto& obj : _objects) {
				obj->Update(dt);
			}
		}
		_FlushDeleteQueue();
	}

	void Scene::PreRender() {
		_lightingUbo->Bind(LIGHT_UBO_BINDING);
	}

	void Scene::RenderGUI()
	{
		for (auto& obj : _objects) {
			// Parents handle rendering for children, so ignore parented objects
			if (obj->GetParent() == nullptr) {
				obj->RenderGUI();
			}
		}
	}

	void Scene::SetShaderLight(int index, bool update /*= true*/) {
		if (index >= 0 && index < Lights.size() && index < MAX_LIGHTS) {
			// Get a reference to the light UBO data so we can update it
			LightingUboStruct& data = _lightingUbo->GetData();
			Light& light = Lights[index];

			// Copy to the ubo data
			data.Lights[index].Position = light.Position;
			data.Lights[index].Color = light.Color;
			data.Lights[index].Attenuation = 1.0f / (1.0f + light.Range);

			// If requested, send the new data to the UBO
			if (update)	_lightingUbo->Update();
		}
	}

	void Scene::SetupShaderAndLights() {
		// Get a reference to the light UBO data so we can update it
		LightingUboStruct& data = _lightingUbo->GetData();
		// Send in how many active lights we have and the global lighting settings
		data.AmbientCol = glm::vec3(0.1f);
		data.NumLights = Lights.size();

		// Iterate over all lights that are enabled and configure them
		for (int ix = 0; ix < Lights.size(); ix++) {
			SetShaderLight(ix, false);
		}

		// Send updated data to OpenGL
		_lightingUbo->Update();
	}

	btDynamicsWorld* Scene::GetPhysicsWorld() const {
		return _physicsWorld;
	}

	Scene::Sptr Scene::FromJson(const nlohmann::json& data)
	{
		Scene::Sptr result = std::make_shared<Scene>();
		result->DefaultMaterial = ResourceManager::Get<Material>(Guid(data["default_material"]));

		if (data.contains("ambient")) {
			result->SetAmbientLight(ParseJsonVec3(data["ambient"]));
		}

		if (data.contains("skybox") && data["skybox"].is_object()) {
			nlohmann::json& blob = data["skybox"].get<nlohmann::json>();
			result->_skyboxMesh = ResourceManager::Get<MeshResource>(Guid(blob["mesh"]));
			result->SetSkyboxShader(ResourceManager::Get<ShaderProgram>(Guid(blob["shader"])));
			result->SetSkyboxTexture(ResourceManager::Get<TextureCube>(Guid(blob["texture"])));
			result->SetSkyboxRotation(glm::mat3_cast(ParseJsonQuat(blob["orientation"])));
		}

		// Make sure the scene has objects, then load them all in!
		LOG_ASSERT(data["objects"].is_array(), "Objects not present in scene!");
		for (auto& object : data["objects"]) {
			GameObject::Sptr obj = GameObject::FromJson(object);
			obj->_scene = result.get();
			obj->_parent.SceneContext = result.get();
			obj->_selfRef = obj;
			result->_objects.push_back(obj);
		}

		// Re-build the parent hierarchy 
		for (const auto& object : result->_objects) {
			if (object->GetParent() != nullptr) {
				object->GetParent()->AddChild(object);
			}
		}

		// Make sure the scene has lights, then load all
		LOG_ASSERT(data["lights"].is_array(), "Lights not present in scene!");
		for (auto& light : data["lights"]) {
			result->Lights.push_back(Light::FromJson(light));
		}

		// Create and load camera config
		result->MainCamera = ComponentManager::GetComponentByGUID<Camera>(Guid(data["main_camera"]));
	
		return result;
	}

	nlohmann::json Scene::ToJson() const
	{
		nlohmann::json blob;
		// Save the default shader (really need a material class)
		blob["default_material"] = DefaultMaterial ? DefaultMaterial->GetGUID().str() : "null";

		blob["ambient"] = GlmToJson(GetAmbientLight());

		blob["skybox"] = nlohmann::json();
		blob["skybox"]["mesh"] = _skyboxMesh ? _skyboxMesh->GetGUID().str() : "null";
		blob["skybox"]["shader"] = _skyboxShader ? _skyboxShader->GetGUID().str() : "null";
		blob["skybox"]["texture"] = _skyboxTexture ? _skyboxTexture->GetGUID().str() : "null";
		blob["skybox"]["orientation"] = GlmToJson(_skyboxRotation);

		// Save renderables
		std::vector<nlohmann::json> objects;
		objects.resize(_objects.size());
		for (int ix = 0; ix < _objects.size(); ix++) {
			objects[ix] = _objects[ix]->ToJson();
		}
		blob["objects"] = objects;

		// Save lights
		std::vector<nlohmann::json> lights;
		lights.resize(Lights.size());
		for (int ix = 0; ix < Lights.size(); ix++) {
			lights[ix] = Lights[ix].ToJson();
		}
		blob["lights"] = lights;

		// Save camera info
		blob["main_camera"] = MainCamera != nullptr ? MainCamera->GetGUID().str() : "null";

		return blob;
	}

	void Scene::Save(const std::string& path) {
		_filePath = path;
		// Save data to file
		FileHelpers::WriteContentsToFile(path, ToJson().dump(1, '\t'));
		LOG_INFO("Saved scene to \"{}\"", path);
	}

	Scene::Sptr Scene::Load(const std::string& path)
	{
		LOG_INFO("Loading scene from \"{}\"", path);
		std::string content = FileHelpers::ReadFile(path);
		nlohmann::json blob = nlohmann::json::parse(content);
		Scene::Sptr result = FromJson(blob);
		result->_filePath = path;
		return result;
	}

	int Scene::NumObjects() const {
		return _objects.size();
	}

	GameObject::Sptr Scene::GetObjectByIndex(int index) const {
		return _objects[index];
	}

	void Scene::_InitPhysics() {
		_collisionConfig = new btDefaultCollisionConfiguration();
		_collisionDispatcher = new btCollisionDispatcher(_collisionConfig);
		_broadphaseInterface = new btDbvtBroadphase();
		_ghostCallback = new btGhostPairCallback();
		_broadphaseInterface->getOverlappingPairCache()->setInternalGhostPairCallback(_ghostCallback);
		_constraintSolver = new btSequentialImpulseConstraintSolver();
		_physicsWorld = new btDiscreteDynamicsWorld(
			_collisionDispatcher,
			_broadphaseInterface,
			_constraintSolver,
			_collisionConfig
		);
		_physicsWorld->setGravity(ToBt(_gravity));
		// TODO bullet debug drawing
		_bulletDebugDraw = new BulletDebugDraw();
		_physicsWorld->setDebugDrawer(_bulletDebugDraw);
		_bulletDebugDraw->setDebugMode(btIDebugDraw::DBG_NoDebug);
	}

	void Scene::_CleanupPhysics() {
		delete _physicsWorld;
		delete _constraintSolver;
		delete _broadphaseInterface;
		delete _ghostCallback;
		delete _collisionDispatcher;
		delete _collisionConfig;
	}


	void Scene::_FlushDeleteQueue() {
		for (auto& weakPtr : _deletionQueue) {
			if (weakPtr.expired()) continue;
			auto& it = std::find(_objects.begin(), _objects.end(), weakPtr.lock());
			if (it != _objects.end()) {
				_objects.erase(it);
			}
		}
		_deletionQueue.clear();
	}

	void Scene::DrawAllGameObjectGUIs()
	{
		for (auto& object : _objects) {
			object->DrawImGui();
		}

		static char buffer[256];
		ImGui::InputText("", buffer, 256);
		ImGui::SameLine();
		if (ImGui::Button("Add Object")) {
			CreateGameObject(buffer);
			memset(buffer, 0, 256);
		}
	}

	void Scene::DrawSkybox()
	{
		if (_skyboxShader != nullptr &&
			_skyboxMesh != nullptr &&
			_skyboxMesh->Mesh != nullptr &&
			_skyboxTexture != nullptr &&
			MainCamera != nullptr) {
			
			glDepthMask(false);
			glDisable(GL_CULL_FACE);
			glDepthFunc(GL_LEQUAL);

			_skyboxShader->Bind();
			_skyboxShader->SetUniformMatrix("u_View", MainCamera->GetProjection() * glm::mat4(glm::mat3(MainCamera->GetView())));
			_skyboxShader->SetUniformMatrix("u_EnvironmentRotation", _skyboxRotation);
			_skyboxTexture->Bind(0);
			_skyboxMesh->Mesh->Draw();

			glDepthFunc(GL_LESS);
			glEnable(GL_CULL_FACE);
			glDepthMask(true);

		}
	}

}