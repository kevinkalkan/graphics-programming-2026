#pragma once

#include <ituGL/application/Application.h>

#include <ituGL/camera/Camera.h>
#include <ituGL/geometry/Model.h>
#include <ituGL/utils/DearImGui.h>

class Texture2DObject;

class KeyboardApplication : public Application
{
public:
    KeyboardApplication();

protected:
    void Initialize() override;
    void Update() override;
    void Render() override;
    void Cleanup() override;

private:
    void InitializeModel();
    void InitializeCamera();
    void InitializeLights();
	void InitializeMaterials(); 
	Model LoadModelWithShader(const char* vertPath, const char* fragPath); // Helper function to load a model with a specific shader and set up the material

    void UpdateCamera();

    void RenderGUI();

private:
    // Helper object for debug GUI
    DearImGui m_imGui;

    // Mouse position for camera controller
    glm::vec2 m_mousePosition;

    // Camera controller parameters
    Camera m_camera;
    glm::vec3 m_cameraPosition;
    float m_cameraTranslationSpeed;
    float m_cameraRotationSpeed;
    bool m_cameraEnabled;
    bool m_cameraEnablePressed;

    // Loaded model
    Model m_modelPBR;
    Model m_modelBlinn;

    // Add light variables
    glm::vec3 m_ambientColor;
    glm::vec3 m_lightColor;
    float m_lightIntensity;
    glm::vec3 m_lightPosition;

	// Add material variables
    float m_roughness;
	float m_metallic;

    // Shader modes
	int m_shaderMode;
	bool m_modeToggled;

	// Time variable for RGB effects
    float m_time;

	// RGB effect modes
	int m_effectMode;
	bool m_effectToggled;
	float m_effectSpeed;
	int m_waveDirection;
	float m_glowIntensity;
    glm::vec3 m_breathingColor;

	// Height scale for parallax mapping
    float m_heightScale;
};
