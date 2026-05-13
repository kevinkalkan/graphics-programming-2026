#include "ViewerApplication.h"

#include <ituGL/asset/ShaderLoader.h>
#include <ituGL/asset/ModelLoader.h>
#include <ituGL/asset/Texture2DLoader.h>
#include <ituGL/shader/Material.h>
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/transform.hpp>
#include <imgui.h>

ViewerApplication::ViewerApplication()
    : Application(1024, 1024, "Viewer demo")
    , m_cameraPosition(0, 1.0f, 1.0f)
    , m_cameraTranslationSpeed(20.0f)
    , m_cameraRotationSpeed(0.5f)
    , m_cameraEnabled(false)
    , m_cameraEnablePressed(false)
    , m_mousePosition(GetMainWindow().GetMousePosition(true))
    , m_ambientColor(0.0f)
    , m_lightColor(0.0f)
    , m_lightIntensity(0.0f)
    , m_lightPosition(0.0f)
	, m_roughness(0.0f)
	, m_metallic(0.0f)
	, m_shaderMode(0)
	, m_modeToggled(false)
    , m_time(0.0f)
	, m_effectMode(0)
	, m_effectToggled(false)
	, m_waveSpeed(0.5f)
	, m_waveDirection(0)
	, m_glowIntensity(1.0f)
{
}

void ViewerApplication::Initialize()
{
    Application::Initialize();

    // Initialize DearImGUI
    m_imGui.Initialize(GetMainWindow());
	static float time = 0.0f;

    InitializeMaterials();
    InitializeCamera();
    InitializeLights();
    InitializeModel();

    DeviceGL& device = GetDevice();
    device.EnableFeature(GL_DEPTH_TEST);
    device.SetVSyncEnabled(true);
}

void ViewerApplication::Update()
{
    Application::Update();

    // Update camera controller
    UpdateCamera();

    static float time = 0.0f;
    time += GetDeltaTime();

    Window& window = GetMainWindow();
    bool mPressed = window.IsKeyPressed(GLFW_KEY_M);

    if (mPressed && !m_modeToggled)
    {
        // Toggle between 0 (PBR) and 1 (Blinn-Phong)
        m_shaderMode = (m_shaderMode == 0) ? 1 : 0;
    }
    m_modeToggled = mPressed;

    bool lPressed = window.IsKeyPressed(GLFW_KEY_L);
    if (lPressed && !m_effectToggled)
    {
        m_effectMode++;
        if (m_effectMode > 3) { 
            m_effectMode = 0;
        }
    }
    m_effectToggled = lPressed;
    // Keep PBR sliders updated dynamically
    for (unsigned int i = 0; i < m_modelPBR.GetMaterialCount(); ++i)
    {
        m_modelPBR.GetMaterial(i).SetUniformValue("Roughness", m_roughness);
        m_modelPBR.GetMaterial(i).SetUniformValue("Metallic", m_metallic);
        m_modelPBR.GetMaterial(i).SetUniformValue("Time", time);
        m_modelPBR.GetMaterial(i).SetUniformValue("EffectMode", m_effectMode);
		m_modelPBR.GetMaterial(i).SetUniformValue("WaveSpeed", m_waveSpeed);
        m_modelPBR.GetMaterial(i).SetUniformValue("WaveDirection", m_waveDirection);
		m_modelPBR.GetMaterial(i).SetUniformValue("GlowIntensity", m_glowIntensity);
    }
   
}

void ViewerApplication::Render()
{
    Application::Render();

    // Clear color and depth
    GetDevice().Clear(true, Color(0.2f, 0.2f, 0.2f, 1.0f), true, 1.0f);

    if (m_shaderMode == 0)
    {
        m_modelPBR.Draw();
    }
    else
    {
        m_modelBlinn.Draw();
    }

    // Render the debug user interface
    RenderGUI();
}

void ViewerApplication::Cleanup()
{
    // Cleanup DearImGUI
    m_imGui.Cleanup();

    Application::Cleanup();
}

Model ViewerApplication::LoadModelWithShader(const char* vertPath, const char* fragPath)
{
    Shader vertexShader = ShaderLoader::Load(Shader::VertexShader, vertPath);
    Shader fragmentShader = ShaderLoader::Load(Shader::FragmentShader, fragPath);
    std::shared_ptr<ShaderProgram> shaderProgram = std::make_shared<ShaderProgram>();
    shaderProgram->Build(vertexShader, fragmentShader);

    ShaderUniformCollection::NameSet filteredUniforms;
    filteredUniforms.insert("WorldMatrix");
    filteredUniforms.insert("ViewProjMatrix");
    filteredUniforms.insert("AmbientColor");
    filteredUniforms.insert("LightColor");
    filteredUniforms.insert("LightPosition");
    filteredUniforms.insert("CameraPosition");

    std::shared_ptr<Material> material = std::make_shared<Material>(shaderProgram, filteredUniforms);

    // Add default variables for loader
    material->SetUniformValue("AmbientReflection", 1.0f);
    material->SetUniformValue("DiffuseReflection", 1.0f);
    material->SetUniformValue("SpecularReflection", 1.0f);
    material->SetUniformValue("SpecularExponent", 1.0f);

    ShaderProgram::Location worldMatrixLocation = shaderProgram->GetUniformLocation("WorldMatrix");
    ShaderProgram::Location viewProjMatrixLocation = shaderProgram->GetUniformLocation("ViewProjMatrix");
    ShaderProgram::Location ambientColorLocation = shaderProgram->GetUniformLocation("AmbientColor");
    ShaderProgram::Location lightColorLocation = shaderProgram->GetUniformLocation("LightColor");
    ShaderProgram::Location lightPositionLocation = shaderProgram->GetUniformLocation("LightPosition");
    ShaderProgram::Location cameraPositionLocation = shaderProgram->GetUniformLocation("CameraPosition");

    material->SetShaderSetupFunction([=](ShaderProgram& shaderProgram)
        {
            shaderProgram.SetUniform(worldMatrixLocation, glm::scale(glm::vec3(1.0f)));
            shaderProgram.SetUniform(viewProjMatrixLocation, m_camera.GetViewProjectionMatrix());
            shaderProgram.SetUniform(ambientColorLocation, m_ambientColor);
            shaderProgram.SetUniform(lightColorLocation, m_lightColor * m_lightIntensity);
            shaderProgram.SetUniform(lightPositionLocation, m_lightPosition);
            shaderProgram.SetUniform(cameraPositionLocation, m_cameraPosition);
        });

    ModelLoader loader(material);
    loader.SetCreateMaterials(true);
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Position, "VertexPosition");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Normal, "VertexNormal");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::TexCoord0, "VertexTexCoord");
    loader.SetMaterialAttribute(VertexAttribute::Semantic::Tangent, "VertexTangent");

    return loader.Load("models/keyboard/Keyboard2.obj");
}

void ViewerApplication::InitializeModel()
{
    // 1. Load the two models
    m_modelPBR = LoadModelWithShader("shaders/cook-torrance.vert", "shaders/cook-torrance.frag");
    m_modelBlinn = LoadModelWithShader("shaders/blinn-phong.vert", "shaders/blinn-phong.frag");

    // 2. Load textures
    Texture2DLoader textureLoader(TextureObject::FormatRGBA, TextureObject::InternalFormatRGBA8);
    textureLoader.SetFlipVertical(true);
    auto colorTexture = textureLoader.LoadShared("models/keyboard/Keyboard2_DefaultMaterial_BaseColor.png");
    auto roughnessTexture = textureLoader.LoadShared("models/keyboard/Keyboard2_DefaultMaterial_Roughness.png");
    auto normalTexture = textureLoader.LoadShared("models/keyboard/Keyboard2_DefaultMaterial_Normal.png");
    auto emissiveTex = textureLoader.LoadShared("models/keyboard/Keyboard2_DefaultMaterial_Emissive.png");

    // 3. Apply variables to PBR Model
    m_modelPBR.GetMaterial(0).SetUniformValue("ColorTexture", colorTexture);
    m_modelPBR.GetMaterial(0).SetUniformValue("RoughnessTexture", roughnessTexture);
    m_modelPBR.GetMaterial(0).SetUniformValue("NormalTexture", normalTexture);
    m_modelPBR.GetMaterial(0).SetUniformValue("Color", glm::vec4(1.0f));
    m_modelPBR.GetMaterial(0).SetUniformValue("EmissiveTexture", emissiveTex);

    // 4. Apply variables to Blinn-Phong Model
    m_modelBlinn.GetMaterial(0).SetUniformValue("ColorTexture", colorTexture);
    m_modelBlinn.GetMaterial(0).SetUniformValue("RoughnessTexture", roughnessTexture);
    m_modelBlinn.GetMaterial(0).SetUniformValue("Color", glm::vec4(1.0f));
    
}
void ViewerApplication::InitializeCamera()
{
    // Set view matrix, from the camera position looking to the origin
    m_camera.SetViewMatrix(m_cameraPosition, glm::vec3(0.0f));

    // Set perspective matrix
    float aspectRatio = GetMainWindow().GetAspectRatio();
    m_camera.SetPerspectiveProjectionMatrix(1.0f, aspectRatio, 0.1f, 1000.0f);
}

void ViewerApplication::InitializeLights()
{
    // Initialize light variables
    m_ambientColor = glm::vec3(1.0f);
    m_lightColor = glm::vec3(1.0f);
    m_lightIntensity = 1.0f;
    m_lightPosition = glm::vec3(-10.0f, 20.0f, 10.0f);
}

void ViewerApplication::InitializeMaterials()
{
    // Initialize material variables
    m_roughness = 0.3f;
    m_metallic = 0.0f;
}

void ViewerApplication::RenderGUI()
{
    m_imGui.BeginFrame();

    // Add debug controls for light properties
    ImGui::ColorEdit3("Ambient color", &m_ambientColor[0]);
    ImGui::Separator();
    ImGui::DragFloat3("Light position", &m_lightPosition[0], 0.1f);
    ImGui::ColorEdit3("Light color", &m_lightColor[0]);
    ImGui::DragFloat("Light intensity", &m_lightIntensity, 0.05f, 0.0f, 100.0f);
    ImGui::Separator();


	ImGui::Text("Material Properties");
    ImGui::SliderFloat("Roughness", &m_roughness, 0.0f, 1.0f);
    ImGui::SliderFloat("Metallic", &m_metallic, 0.0f, 1.0f);
   // if (ImGui::SliderFloat("Roughness", &m_roughness, 0.0f, 1.0f))
       // m_modelPBR.GetMaterial(0).SetUniformValue("Roughness", m_roughness);
//	if (ImGui::SliderFloat("Metallic", &m_metallic, 0.0f, 1.0f))
      //  m_modelPBR.GetMaterial(0).SetUniformValue("Metallic", m_metallic);
    ImGui::Separator();

    ImGui::Text("Shader Swap (Press 'M')");
    ImGui::RadioButton("Cook-Torrance (PBR)", &m_shaderMode, 0);
    ImGui::RadioButton("Blinn-Phong", &m_shaderMode, 1);
    ImGui::Separator();

    ImGui::Text("RGB Effects (Press 'L')");
    ImGui::RadioButton("Off", &m_effectMode, 0);
    ImGui::RadioButton("Breathing", &m_effectMode, 1);
    ImGui::RadioButton("Rainbow Wave", &m_effectMode, 2);
	ImGui::RadioButton("Cycle", &m_effectMode, 3);
	if (m_effectMode == 2) 
    {
		ImGui::Text("Wave Direction");
		ImGui::RadioButton("Horizontal", &m_waveDirection, 0);
		ImGui::SameLine();
		ImGui::RadioButton("Vertical", &m_waveDirection, 1);
    }
    if (ImGui::SliderFloat("Wave Speed", &m_waveSpeed, 0.0f, 1.0f))
        m_modelPBR.GetMaterial(0).SetUniformValue("WaveSpeed", m_waveSpeed);
   
    ImGui::SliderFloat("Glow Intensity", &m_glowIntensity, 0.0f, 5.0f);
    ImGui::Separator();

    m_imGui.EndFrame();
}

void ViewerApplication::UpdateCamera()
{
    Window& window = GetMainWindow();

    // Update if camera is enabled (controlled by SPACE key)
    {
        bool enablePressed = window.IsKeyPressed(GLFW_KEY_SPACE);
        if (enablePressed && !m_cameraEnablePressed)
        {
            m_cameraEnabled = !m_cameraEnabled;

            window.SetMouseVisible(!m_cameraEnabled);
            m_mousePosition = window.GetMousePosition(true);
        }
        m_cameraEnablePressed = enablePressed;
    }

    if (!m_cameraEnabled)
        return;

    glm::mat4 viewTransposedMatrix = glm::transpose(m_camera.GetViewMatrix());
    glm::vec3 viewRight = viewTransposedMatrix[0];
    glm::vec3 viewForward = -viewTransposedMatrix[2];

    // Update camera translation
    {
        glm::vec2 inputTranslation(0.0f);

        if (window.IsKeyPressed(GLFW_KEY_A))
            inputTranslation.x = -0.2f;
        else if (window.IsKeyPressed(GLFW_KEY_D))
            inputTranslation.x = 0.2f;

        if (window.IsKeyPressed(GLFW_KEY_W))
            inputTranslation.y = 0.2f;
        else if (window.IsKeyPressed(GLFW_KEY_S))
            inputTranslation.y = -0.2f;

        inputTranslation *= m_cameraTranslationSpeed;
        inputTranslation *= GetDeltaTime();

        // Double speed if SHIFT is pressed
        if (window.IsKeyPressed(GLFW_KEY_LEFT_SHIFT))
            inputTranslation *= 2.0f;

        m_cameraPosition += inputTranslation.x * viewRight + inputTranslation.y * viewForward;
    }

    // Update camera rotation
   {
        glm::vec2 mousePosition = window.GetMousePosition(true);
        glm::vec2 deltaMousePosition = mousePosition - m_mousePosition;
        m_mousePosition = mousePosition;

        glm::vec3 inputRotation(-deltaMousePosition.x, deltaMousePosition.y, 0.0f);

        inputRotation *= m_cameraRotationSpeed;

        viewForward = glm::rotate(inputRotation.x, glm::vec3(0,1,0)) * glm::rotate(inputRotation.y, glm::vec3(viewRight)) * glm::vec4(viewForward, 0);
    }

   // Update view matrix
   m_camera.SetViewMatrix(m_cameraPosition, m_cameraPosition + viewForward);
}
