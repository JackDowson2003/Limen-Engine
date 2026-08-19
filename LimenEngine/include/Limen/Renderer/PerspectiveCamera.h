#pragma once
#include "Camera.h"
#include "glm/glm.hpp"

namespace Limen
{
    class LIMEN_API PerspectiveCamera : public Camera
    {
    public:
        PerspectiveCamera(float fov, float aspectRation, float near = 0.1f, float far = 100.0f);

        void SetPosition(const glm::vec3 &position);

        void SetRotation(const glm::vec3 &rotation);

        void SetFOV(float fov);

        void SetAspectRatio(float aspectRatio);

        [[nodiscard]] const glm::vec3 &GetPosition() const override
        {
            return m_Position;
        }

        [[nodiscard]] const glm::vec3 &GetRotation() const
        {
            return m_Rotation;
        }

        [[nodiscard]] const glm::mat4 &GetProjectionMatrix() const override
        {
            return m_ProjectionMatrix;
        }

        [[nodiscard]] const glm::mat4 &GetViewProjectionMatrix() const override
        {
            return m_ViewProjectionMatrix;
        }

        [[nodiscard]] const glm::mat4 &GetViewMatrix() const override
        {
            return m_ViewMatrix;
        }

    private:
        void RecalculateProjectionMatrix();
        void RecalculateViewMatrix();


        // 默认站在世界空间z=3处，看向OpenGL默认的-z方向。
        glm::vec3 m_Position{0.0f, 0.0f, 3.0f};
        // x=pitch，y=yaw，z=roll，单位为角度。
        glm::vec3 m_Rotation{0.0f};

        glm::mat4 m_ProjectionMatrix;
        glm::mat4 m_ViewMatrix;
        glm::mat4 m_ViewProjectionMatrix;


        float m_VerticalFOV = 45.0f;
        float m_AspectRatio = 16.0f / 9.0f;
        float m_Near = 0.1f;
        float m_Far = 100.0f;

    };
}
