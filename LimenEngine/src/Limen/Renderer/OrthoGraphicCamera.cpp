//
// Created by chenlong on 2026/8/12.
//
#include "Renderer/OrthoGraphicCamera.h"

#include "glm/ext/matrix_clip_space.hpp"
#include <glm/ext/matrix_transform.hpp>

namespace Limen
{
    OrthographicCamera::OrthographicCamera(const float left, const float right, const float bottom, const float top,
                                           const float near, const float far)
        : m_ProjectionMatrix(glm::ortho(left, right, bottom, top, near, far))
    {
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix; //vp =>p * v
    }

    void OrthographicCamera::SetPosition(const glm::vec3 &position)
    {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void OrthographicCamera::SetRotation(float rotation)
    {
        m_Rotation = rotation;
        RecalculateViewMatrix();
    }

    void OrthographicCamera::RecalculateViewMatrix()
    {
        //把世界平移 "-m_Position"
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);
        //以Y轴旋转 trans * rotate
        transform = glm::rotate(transform, glm::radians(m_Rotation), glm::vec3(0.0f, 0.0f, 1.0f));
        //同时在相机视图矩阵中应该先旋转再平移 因为执行顺序：先反向平移整个世界，再反向旋转整个世界
        m_ViewMatrix = glm::inverse(transform); //逆矩阵
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix; // p * v * m MVP
    }
}
