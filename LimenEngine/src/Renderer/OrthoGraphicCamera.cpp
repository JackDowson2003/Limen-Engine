//
// Created by chenlong on 2026/8/12.
//
#include "Limen/Renderer/OrthoGraphicCamera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace Limen
{
    OrthoGraphicCamera::OrthoGraphicCamera(const float left, const float right, const float bottom, const float top,
                                           const float nearPlane, const float farPlane)
        : m_ProjectionMatrix(glm::ortho(left, right, bottom, top, nearPlane, farPlane))
    {
        SetProjection(left, right, bottom, top, nearPlane, farPlane);
    }

    void OrthoGraphicCamera::SetPosition(const glm::vec3 &position)
    {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void OrthoGraphicCamera::SetRotation(float rotation)
    {
        m_Rotation = rotation;
        RecalculateViewMatrix();
    }

    void OrthoGraphicCamera::SetProjection(const float left, const float right, const float bottom, const float top, const float nearPlane,
        const float farPlane)
    {
        m_ProjectionMatrix = glm::ortho(
            left, right, bottom, top, nearPlane, farPlane);
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }

    void OrthoGraphicCamera::RecalculateViewMatrix()
    {
        // 先构造相机变换，再取逆得到观察矩阵。
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), -m_Position);
        transform = glm::rotate(transform, glm::radians(m_Rotation), glm::vec3(0.0f, 1.0f, 0.0f));
        m_ViewMatrix = glm::inverse(transform);

        // Model 矩阵由提交绘制时再乘入。
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}
