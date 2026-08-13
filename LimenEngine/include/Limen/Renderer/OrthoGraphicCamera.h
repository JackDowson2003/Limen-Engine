//
// Created by chenlong on 2026/8/12.
//
#pragma once

#include <glm/glm.hpp>

#pragma once

namespace Limen
{
    //For 2D, UI, Editor to use
    class OrthoGraphicCamera
    {
    public:
        OrthoGraphicCamera(float left, float right, float bottom, float top, float near = -1.f, float far = 1.f);

        void SetPosition(const glm::vec3 &position) { m_Position = position; RecalculateViewMatrix();}
        void SetRotation(const float rotation) { m_Rotation = rotation; RecalculateViewMatrix();}
        float GetRotation() const { return m_Rotation; }
        const glm::vec3& GetPosition() const { return m_Position; }

        const glm::mat4& GetProjectionMatrix() const { return m_ProjectionMatrix; }
        const glm::mat4& GetViewMatrix() const { return m_ViewMatrix; }
        const glm::mat4& GetViewProjectionMatrix() const { return m_ViewProjMatrix; }
    private:
        void RecalculateViewMatrix();

        glm::mat4 m_ProjectionMatrix; //相机使用什么镜头、能看到多大范围。
        glm::mat4 m_ViewMatrix; //相机站在哪里、朝哪里看。
        glm::mat4 m_ViewProjMatrix; //proj * view


        glm::vec3 m_Position = glm::vec3(0.0f);
        float m_Rotation = 0.0f;
    };
}
