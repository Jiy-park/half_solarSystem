#include "context.h"
#include "image.h"
#include <imgui.h>
ContextUPtr Context::Create(){
    auto context = ContextUPtr(new Context());
    if (!context->Init())
        return nullptr;
    return std::move(context);
}

void Context::ProcessInput(GLFWwindow* window) {
    if (!m_cameraControl)
        return;
    const float cameraSpeed = 0.05f;
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * m_cameraFront;
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * m_cameraFront;

    auto cameraRight = glm::normalize(glm::cross(m_cameraUp, -m_cameraFront));
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraRight;
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraRight;

    auto cameraUp = glm::normalize(glm::cross(-m_cameraFront, cameraRight));
    if (glfwGetKey(window, GLFW_KEY_E) == GLFW_PRESS)
        m_cameraPos += cameraSpeed * cameraUp;
    if (glfwGetKey(window, GLFW_KEY_Q) == GLFW_PRESS)
        m_cameraPos -= cameraSpeed * cameraUp;
}

void Context::Reshape(int width, int height) {
    m_width = width;
    m_height = height;
    glViewport(0, 0, m_width, m_height);
    
    m_framebuffer = Framebuffer::Create({Texture::Create(width, height, GL_RGBA)});

    m_deferGeoFramebuffer = Framebuffer::Create({
        Texture::Create(width, height, GL_RGBA16F, GL_FLOAT),
        Texture::Create(width, height, GL_RGBA16F, GL_FLOAT),
        Texture::Create(width, height, GL_RGBA, GL_UNSIGNED_BYTE),
    });
    m_ssaoFramebuffer = Framebuffer::Create({Texture::Create(width, height, GL_RED),});

    m_ssaoBlurFramebuffer = Framebuffer::Create({Texture::Create(width, height, GL_RED),});

}

void Context::MouseMove(double x, double y) {   
    if (!m_cameraControl)
        return;
    auto pos = glm::vec2((float)x, (float)y);
    auto deltaPos = pos - m_prevMousePos;

    const float cameraRotSpeed = 0.3f;
    m_cameraYaw -= deltaPos.x * cameraRotSpeed;
    m_cameraPitch -= deltaPos.y * cameraRotSpeed;

    if (m_cameraYaw < 0.0f)      m_cameraYaw += 360.0f;     
    if (m_cameraYaw > 360.0f)      m_cameraYaw -= 360.0f;

    if (m_cameraPitch > 89.0f)    m_cameraPitch = 89.0f;  
    if (m_cameraPitch < -89.0f)    m_cameraPitch = -89.0f;
    

    m_prevMousePos = pos;
}

void Context::MouseButton(int button, int action, double x, double y) {
    if (button == GLFW_MOUSE_BUTTON_RIGHT) {
        if (action == GLFW_PRESS){
            m_prevMousePos = glm::vec2((float)x, (float)y);
            m_cameraControl = true;
        }
    else if (action == GLFW_RELEASE) {
        m_cameraControl = false;
    }
  }
}

bool Context::Init(){
    glEnable(GL_MULTISAMPLE);
    m_box=Mesh::CreateBox();

    m_earthTexture = Texture::CreateFromImage(Image::Load("./image/earth_texture.jpg").get());
    m_planet = Model::Load("./model/planet.obj");///////model_planet
    if (!m_planet){
        SPDLOG_ERROR("failed to load model");
        return false;
    }




    m_simpleProgram = Program::Create("./shader/simple.vs", "./shader/simple.fs");
    if (!m_simpleProgram)
        return false;

    m_program = Program::Create("./shader/lighting.vs", "./shader/lighting.fs");
    if (!m_program)
        return false;

    m_textureProgram = Program::Create("./shader/texture.vs", "./shader/texture.fs");
    if (!m_textureProgram)
        return false;

    m_postProgram = Program::Create("./shader/texture.vs", "./shader/gamma.fs");                                  
    if (!m_postProgram)
        return false;

    glClearColor(0.5f,1.0f,0.8f,0.5f);
                             

    auto cubeRight = Image::Load("./image/space_Map/space_right.jpeg", false);
    auto cubeLeft = Image::Load("./image/space_Map/space_left.jpeg", false);
    auto cubeTop = Image::Load("./image/space_Map/space_top.jpeg", false);
    auto cubeBottom = Image::Load("./image/space_Map/space_bottom.jpeg", false);
    auto cubeFront = Image::Load("./image/space_Map/space_front.jpeg", false);
    auto cubeBack = Image::Load("./image/space_Map/space_back.jpeg", false);
    m_cubeTexture = CubeTexture::CreateFromImages({
        cubeRight.get(),
        cubeLeft.get(),
        cubeTop.get(),
        cubeBottom.get(),
        cubeFront.get(),
        cubeBack.get(),
    });
    m_skyboxProgram = Program::Create("./shader/skybox.vs", "./shader/skybox.fs");
    m_envMapProgram = Program::Create("./shader/env_map.vs", "./shader/env_map.fs");

    m_shadowMap = ShadowMap::Create(1024, 1024);
    m_lightingShadowProgram = Program::Create("./shader/lighting_shadow.vs", "./shader/lighting_shadow.fs");
    
    m_normalProgram = Program::Create("./shader/normal.vs", "./shader/normal.fs");
    m_deferGeoProgram = Program::Create("./shader/defer_geo.vs", "./shader/defer_geo.fs"); 
    m_deferLightProgram = Program::Create( "./shader/defer_light.vs", "./shader/defer_light.fs");
   

    m_ssaoProgram = Program::Create("./shader/ssao.vs", "./shader/ssao.fs");
    m_blurProgram = Program::Create("./shader/blur_5x5.vs", "./shader/blur_5x5.fs");
       

    std::vector<glm::vec3> ssaoNoise;
    ssaoNoise.resize(16);
    for (size_t i = 0; i < ssaoNoise.size(); i++){
        // randomly selected tangent direction
        glm::vec3 sample(RandomRange(-1.0f, 1.0f), RandomRange(-1.0f, 1.0f), 0.0f);                   
        ssaoNoise[i] = sample;
    }

    m_ssaoNoiseTexture = Texture::Create(4, 4, GL_RGB16F, GL_FLOAT);
    m_ssaoNoiseTexture->Bind();
    m_ssaoNoiseTexture->SetFilter(GL_NEAREST, GL_NEAREST);
    m_ssaoNoiseTexture->SetWrap(GL_REPEAT, GL_REPEAT);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 4, 4, GL_RGB, GL_FLOAT, ssaoNoise.data());
                    

    m_ssaoSamples.resize(64);
    for (size_t i = 0; i < m_ssaoSamples.size(); i++) {
        // uniformly randomized point in unit hemisphere
        glm::vec3 sample(
            RandomRange(-1.0f, 1.0f),
            RandomRange(-1.0f, 1.0f),
            RandomRange(0.0f, 1.0f));
        sample = glm::normalize(sample) * RandomRange();

        // scale for slightly shift to center
        float t = (float)i / (float)m_ssaoSamples.size();
        float t2 = t * t;
        float scale = (1.0f - t2) * 0.1f + t2 * 1.0f;

        m_ssaoSamples[i] = sample * scale;
    }               
    return true;
}

void Context::Render(){
    if (ImGui::Begin("UI_WINDOW")){
        if(ImGui::ColorEdit4("clear color",glm::value_ptr(m_clearColor)))
            glClearColor(m_clearColor.x,m_clearColor.y,m_clearColor.z,m_clearColor.w);  
        ImGui::DragFloat("gamma", &m_gamma, 0.01f, 0.0f, 2.0f);
        ImGui::Separator();
        ImGui::DragFloat3("camera pos",glm::value_ptr(m_cameraPos),0.01f);
        ImGui::DragFloat("camera yaw",&m_cameraYaw,0.5f);
        ImGui::DragFloat("camera pitch",&m_cameraPitch, 0.5f, -89.0f, 89.0f);
        ImGui::Separator();
        if(ImGui::Button("reset camera")){
                m_cameraYaw = 0.0f;
                m_cameraPitch = 0.0f;
                m_cameraPos = glm::vec3(0.0f,0.0f,3.0f);
        }

        if (ImGui::CollapsingHeader("light", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("l.directional", &m_light.directional);
            ImGui::DragFloat3("l.position", glm::value_ptr(m_light.position), 0.01f);
            ImGui::DragFloat3("l.direction", glm::value_ptr(m_light.direction), 0.01f);
            ImGui::DragFloat("1.distance",&m_light.distance, 0.5f, 0.0f, 3000.0f);
            ImGui::DragFloat2("1.cutoff", glm::value_ptr(m_light.cutoff), 0.5f, 0.0f, 180.0f);
            ImGui::ColorEdit3("l.ambient", glm::value_ptr(m_light.ambient));
            ImGui::ColorEdit3("l.diffuse", glm::value_ptr(m_light.diffuse));
            ImGui::ColorEdit3("l.specular", glm::value_ptr(m_light.specular));
            ImGui::Checkbox("flash light",&m_flashLightMode);
            ImGui::Checkbox("l.blinn", &m_blinn);
            ImGui::DragFloat("ssao radius",&m_ssaoRadius, 0.01f, 0.0f, 5.0f);
            ImGui::Checkbox("use ssao", &m_useSsao);
        }
        ImGui::Checkbox("animation",&m_animation);

        ImGui::Image((ImTextureID)m_shadowMap->GetShadowMap()->Get(), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
                     
    }
    ImGui::End();

    if (ImGui::Begin("planet_setting")){
        ImGui::DragFloat("planet_distance", &m_planetDistance, 1.0f, 100.0f, 400.0f);
        ImGui::DragFloat("sun_size", &m_sunSize, 0.1f, 0.1f, 10.0f);
        ImGui::DragFloat("planet_size", &m_planetSize, 0.1f, 1.0f, 10.0f);
        ImGui::DragFloat("time_speed", &m_timeSpeed, 0.1f, 1.0f, 10.0f);            
    }
    ImGui::End();
    m_cameraFront =
        glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraYaw), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraPitch), glm::vec3(1.0f, 0.0f, 0.0f)) *
        glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

    // m_light.position=m_cameraPos;
    // m_light.direction=m_cameraFront;

    auto projection = glm::perspective(glm::radians(45.0f), (float)m_width / (float)m_height, 0.1f, 100.0f);
    auto view = glm::lookAt(m_cameraPos, m_cameraPos + m_cameraFront, m_cameraUp);

    auto lightView = glm::lookAt(m_light.position,m_light.position + m_light.direction,glm::vec3(0.0f, 1.0f, 0.0f));

    auto lightProjection = m_light.directional ? 
                            glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 1.0f, 30.0f) : 
                            glm::perspective(glm::radians((m_light.cutoff[0] + m_light.cutoff[1]) * 2.0f), 1.0f, 1.0f, 20.0f);

    auto skyboxModelTransform =
        glm::translate(glm::mat4(1.0), m_cameraPos) *
        glm::scale(glm::mat4(1.0), glm::vec3(50.0f));
    m_skyboxProgram->Use();
    m_cubeTexture->Bind();
    m_skyboxProgram->SetUniform("skybox", 0);
    m_skyboxProgram->SetUniform("transform", projection * view * skyboxModelTransform);
    m_box->Draw(m_skyboxProgram.get());

    glm::vec3 lightPos = m_light.position;
    glm::vec3 lightDir = m_light.direction; 
    if(m_flashLightMode){
        lightPos = m_cameraPos;
        lightDir = m_cameraFront;
    }
    else{
        auto lightModelTransform = glm::translate(glm::mat4(1.0), m_light.position) *glm::scale(glm::mat4(1.0), glm::vec3(0.1f));
        m_simpleProgram->Use();
        m_simpleProgram->SetUniform("color", glm::vec4(m_light.ambient + m_light.diffuse, 1.0f));
        m_simpleProgram->SetUniform("transform", projection * view * lightModelTransform);
        m_box->Draw(m_simpleProgram.get());
    }

    m_lightingShadowProgram->Use();
    m_lightingShadowProgram->SetUniform("viewPos", m_cameraPos);
    m_lightingShadowProgram->SetUniform("light.directional", m_light.directional ? 1:0);
    m_lightingShadowProgram->SetUniform("light.position", m_light.position);
    m_lightingShadowProgram->SetUniform("light.direction", m_light.direction);
    m_lightingShadowProgram->SetUniform("light.cutoff", glm::vec2(
                                                            cosf(glm::radians(m_light.cutoff[0])),
                                                            cosf(glm::radians(m_light.cutoff[0] + m_light.cutoff[1]))));
    m_lightingShadowProgram->SetUniform("light.attenuation", GetAttenuationCoeff(m_light.distance));
    m_lightingShadowProgram->SetUniform("light.ambient", m_light.ambient);
    m_lightingShadowProgram->SetUniform("light.diffuse", m_light.diffuse);
    m_lightingShadowProgram->SetUniform("light.specular", m_light.specular);
    m_lightingShadowProgram->SetUniform("blinn", (m_blinn ? 1 : 0));
    m_lightingShadowProgram->SetUniform("lightTransform", lightProjection * lightView);
    glActiveTexture(GL_TEXTURE3);
    m_shadowMap->GetShadowMap()->Bind();
    m_lightingShadowProgram->SetUniform("shadowMap", 3);

    

    glActiveTexture(GL_TEXTURE0);

    DrawScene(view, projection, m_lightingShadowProgram.get());
}

void Context::DrawScene(const glm::mat4 &view, const glm::mat4 &projection, const Program *program) {
    m_time += m_timeSpeed;
    program->Use();
    auto modelTransform = //sun
        glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 10.0f, 10.0f) * m_sunSize);
    auto transform = projection * view * modelTransform;
    program->SetUniform("transform", transform);
    program->SetUniform("modelTransform", modelTransform);
    glActiveTexture(GL_TEXTURE4);
    m_earthTexture->Bind();
    m_lightingShadowProgram->SetUniform("tex", 4);
    m_planet->Draw(program);

    modelTransform = //mercury
        glm::translate(glm::mat4(1.0f),
                       0.33f * m_planetDistance * glm::vec3(cosf(glm::radians(2 * m_time / 88.0f)), 0.0f, sinf(glm::radians(2 * m_time / 88.0f)))) *
        glm::rotate(glm::mat4(1.0f), glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.34f, 0.34f, 0.34f) * m_planetSize);
    transform = projection * view * modelTransform;
    program->SetUniform("transform", transform);
    program->SetUniform("modelTransform", modelTransform);
    m_planet->Draw(program);

    modelTransform = //venus
        glm::translate(glm::mat4(1.0f),
                       0.66f * m_planetDistance * glm::vec3(cosf(glm::radians(2 * m_time / 225.0f)), 0.0f, sinf(glm::radians(2 * m_time / 225.0f)))) *
        glm::rotate(glm::mat4(1.0f), glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f) * m_planetSize);
    transform = projection * view * modelTransform;
    program->SetUniform("transform", transform);
    program->SetUniform("modelTransform", modelTransform);
    m_planet->Draw(program);

    modelTransform = //earth
        glm::translate(glm::mat4(1.0f),
                       1.0f * m_planetDistance * glm::vec3(cosf(glm::radians(2 * m_time / 365.0f)), 0.0f, sinf(glm::radians(2 * m_time / 365.0f)))) *
        glm::rotate(glm::mat4(1.0f), glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(1.0f, 1.0f, 1.0f) * m_planetSize);
    transform = projection * view * modelTransform;
    program->SetUniform("transform", transform);
    program->SetUniform("modelTransform", modelTransform);
    
    m_planet->Draw(program);

    // modelTransform = //moon
    //     glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f)) *
    //     glm::rotate(glm::mat4(1.0f), glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
    //     glm::scale(glm::mat4(1.0f), glm::vec3(1.5f, 1.5f, 1.5f));
    // transform = projection * view * modelTransform;
    // program->SetUniform("transform", transform);
    // program->SetUniform("modelTransform", modelTransform);
    // m_planet->Draw(program);

    modelTransform = //mars
        glm::translate(glm::mat4(1.0f),
                       1.4f * m_planetDistance * glm::vec3(cosf(glm::radians(2 * m_time / 687.0f)), 0.0f, sinf(glm::radians(2 * m_time / 687.0f)))) *
        glm::rotate(glm::mat4(1.0f), glm::radians(50.0f), glm::vec3(0.0f, 1.0f, 0.0f)) *
        glm::scale(glm::mat4(1.0f), glm::vec3(0.5f, 0.5f, 0.5f) * m_planetSize);
    transform = projection * view * modelTransform;
    program->SetUniform("transform", transform);
    program->SetUniform("modelTransform", modelTransform);
    m_planet->Draw(program);

}