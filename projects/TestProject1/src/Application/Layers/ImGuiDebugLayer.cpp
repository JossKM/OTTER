#include "ImGuiDebugLayer.h"
#include "../Application.h"
#include "Utils/ImGuiHelper.h"
#include "../Windows/HierarchyWindow.h"
#include "../Windows/InspectorWindow.h"
#include "Audio/AudioLayer.h"

ImGuiDebugLayer::ImGuiDebugLayer() :
	ApplicationLayer()
{
	Name = "ImGui Debug Layer";
	Overrides = AppLayerFunctions::OnAppLoad | AppLayerFunctions::OnAppUnload | AppLayerFunctions::OnPreRender | AppLayerFunctions::OnRender | AppLayerFunctions::OnPostRender;
}

ImGuiDebugLayer::~ImGuiDebugLayer() = default;

void ImGuiDebugLayer::OnAppLoad(const nlohmann::json& config)
{
	Application& app = Application::Get();

	// Initialize our ImGui helper
	ImGuiHelper::Init(app.GetWindow());

	// Register our windows
	RegisterWindow<HierarchyWindow>();
	RegisterWindow<InspectorWindow>();
}

void ImGuiDebugLayer::OnAppUnload()
{
	ImGuiHelper::Cleanup();
}

void ImGuiDebugLayer::OnPreRender()
{
	ImGuiHelper::StartFrame();
}

void ImGuiDebugLayer::OnRender()
{
	for (const auto& window : _windows) {
		window->Render();
	}
	auto audioEngine = Application::Get().GetLayer<AudioLayer>().get()->engine;
	if(ImGui::Button("hello"))
	{
		audioEngine->loadSound("hit", "audio/hit.wav", true);
		audioEngine->playSoundByName("hit");
	}

	//if (ImGui::Button("world"))
	//{
	//	AudioEmitter myEmitter = AudioEmitter();
	//	myEmitter.SetPosition(glm::vec3(100, 0, 0));
	//
	//	//audioEngine->PlaySound("audio/hit.wav");
	//	audioEngine->PlaySound("audio/hit.wav", myEmitter);
	//}
}

void ImGuiDebugLayer::OnPostRender()
{
	ImGuiHelper::EndFrame();
}
