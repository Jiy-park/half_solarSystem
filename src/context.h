#ifndef __CONTEXT_H__
#define __CONTEXT_H__

#include "common.h"
#include "shader.h"
#include "program.h"
#include "buffer.h"
#include "vertex_layout.h"
#include "texture.h"
#include "mesh.h"
#include "model.h"
#include "framebuffer.h"
#include "shadow_map.h"

CLASS_PTR(Context)
class Context{
public:
    static ContextUPtr Create();
    void Render();
    void ProcessInput(GLFWwindow* window);
    void Reshape(int width, int height);
    void MouseMove(double x, double y);
    void MouseButton(int button, int action, double x, double y);
    void DrawScene(const glm::mat4 &view, const glm::mat4 &projection, const Program *program);

private:
    Context() {}
    bool Init();
    ProgramUPtr m_program;
    ProgramUPtr m_simpleProgram;
    ProgramUPtr m_textureProgram;
    ProgramUPtr m_postProgram;
    float m_gamma {1.0f};

    MeshUPtr m_box;  
    MeshUPtr m_plane; 	

    // animation
    bool m_animation{true};

    // clear color
    glm::vec4 m_clearColor{glm::vec4(0.1f, 0.2f, 0.3f, 0.0f)};

    // light parameter
    struct Light{
        bool directional { false };
        glm::vec3 position{glm::vec3(2.0f, 4.0f, 4.0f)};
        glm::vec3 direction{glm::vec3(-0.5f, -1.5f, -1.0f)};
        glm::vec2 cutoff{glm::vec2(50.0f, 5.0f)};
        float distance{150.0f};
        glm::vec3 ambient{glm::vec3(0.1f, 0.1f, 0.1f)};
        glm::vec3 diffuse{glm::vec3(0.8f, 0.8f, 0.8f)};
        glm::vec3 specular{glm::vec3(1.0f, 1.0f, 1.0f)};
    };
    Light m_light;
    bool m_flashLightMode {false};
    bool m_blinn { true };


    // camera parameter

    bool m_cameraControl{false};
    glm::vec2 m_prevMousePos{glm::vec2(0.0f)};
    float m_cameraPitch{-20.0f};
    float m_cameraYaw{0.0f};
    glm::vec3 m_cameraPos { glm::vec3(0.0f, 2.5f, 8.0f) };
    glm::vec3 m_cameraFront{glm::vec3(0.0f, 0.0f, -1.0f)};
    glm::vec3 m_cameraUp{glm::vec3(0.0f, 1.0f, 0.0f)};
    // framebuffer
    FramebufferUPtr m_framebuffer;

    // cubemap
    CubeTextureUPtr m_cubeTexture;
    ProgramUPtr m_skyboxProgram;
    ProgramUPtr m_envMapProgram;

    TexturePtr m_grassTexture;
    ProgramUPtr m_grassProgram;
    std::vector<glm::vec3> m_grassPos;

    BufferUPtr m_grassPosBuffer;
    VertexLayoutUPtr m_grassInstance;
    
    //shadowMap
    ShadowMapUPtr m_shadowMap;
    ProgramUPtr m_lightingShadowProgram;

    // normal map
    TextureUPtr m_brickDiffuseTexture;
    TextureUPtr m_brickNormalTexture;
    ProgramUPtr m_normalProgram;

    int m_width{WINDOW_WIDTH};
    int m_height{WINDOW_HEIGHT};

    // deferred shading
    FramebufferUPtr m_deferGeoFramebuffer;
    ProgramUPtr m_deferGeoProgram;

    ProgramUPtr m_deferLightProgram;

    struct DeferLight{
        glm::vec3 position;
        glm::vec3 color;
    };
    std::vector<DeferLight> m_deferLights;

    // ssao
    FramebufferUPtr m_ssaoFramebuffer;
    ProgramUPtr m_ssaoProgram;
    ModelUPtr m_model;  // for test rendering
    TextureUPtr m_ssaoNoiseTexture;
    std::vector<glm::vec3> m_ssaoSamples;
    float m_ssaoRadius { 1.0f };
    
    ProgramUPtr m_blurProgram;
    FramebufferUPtr m_ssaoBlurFramebuffer;
    bool m_useSsao { true };


    //planet
    ModelUPtr m_planet;
    TexturePtr m_earthTexture;
    float m_planetDistance { 300.0f };
    float m_planetSize { 1.0f };
    float m_sunSize { 1.0f };
    float m_time { 0.0f };
    float m_timeSpeed { 1.0f };
};

#endif // __CONTEXT_H__