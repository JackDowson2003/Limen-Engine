#pragma once

#include "Limen/Renderer/Camera.h"

namespace Limen
{
    class LIMEN_API OrthoGraphicCamera final : public Camera
    {
    public:
        OrthoGraphicCamera(
    float left,
    float right,
    float bottom,
    float top,
    float nearPlane = -1.0f,
    float farPlane = 1.0f
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

        /**
         * @brief 重新设置正交投影范围。
         *
         * 当窗口尺寸或相机缩放等级发生变化时，通过该函数重新计算投影矩阵。
         *
         * @param left
         * 相机可见区域的左边界。
         *
         * @param right
         * 相机可见区域的右边界。
         *
         * @param bottom
         * 相机可见区域的下边界。
         *
         * @param top
         * 相机可见区域的上边界。
         *
         * @param nearPlane
         * 相机空间中的近裁剪面。
         *
         * @param farPlane
         * 相机空间中的远裁剪面。
         */
        void SetProjection(
            float left,
            float right,
            float bottom,
            float top,
            float nearPlane = -1.0f,
            float farPlane = 1.0f
        );

    private:
        void RecalculateViewMatrix();

        glm::mat4 m_ProjectionMatrix{1.0f};
        glm::mat4 m_ViewMatrix{1.0f};
        glm::mat4 m_ViewProjectionMatrix{1.0f};

        glm::vec3 m_Position{0.0f};
        float m_Rotation = 0.0f;
    };
}