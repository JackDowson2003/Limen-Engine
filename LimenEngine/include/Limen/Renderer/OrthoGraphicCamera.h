#pragma once

#include "Camera.h"

namespace Limen
{
    class LIMEN_API OrthographicCamera final : public Camera
    {
    public:
        OrthographicCamera(
            float left,
            float right,
            float bottom,
            float top,
            float near = -1.0f,
            float far = 1.0f
        );

        void SetPosition(const glm::vec3& position);
        void SetRotation(float rotation);

        [[nodiscard]]
        float GetRotation() const
        {
            return m_Rotation;
        }

        [[nodiscard]]
        const glm::vec3& GetPosition() const override
        {
            return m_Position;
        }

        [[nodiscard]]
        const glm::mat4& GetProjectionMatrix() const override
        {
            return m_ProjectionMatrix;
        }

        [[nodiscard]]
        const glm::mat4& GetViewMatrix() const override
        {
            return m_ViewMatrix;
        }

        [[nodiscard]]
        const glm::mat4& GetViewProjectionMatrix() const override
        {
            return m_ViewProjectionMatrix;
        }

    private:
        void RecalculateViewMatrix();

        glm::mat4 m_ProjectionMatrix{1.0f};
        glm::mat4 m_ViewMatrix{1.0f};
        glm::mat4 m_ViewProjectionMatrix{1.0f};

        glm::vec3 m_Position{0.0f};
        float m_Rotation = 0.0f;
    };
}