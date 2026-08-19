//
// Created by chenlong on 2026/8/20.
//
#include "Renderer/PerspectiveCamera.h"

#include "Log.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

namespace Limen
{
    PerspectiveCamera::PerspectiveCamera(const float fov, const float aspectRation, const float near, const float far)
        : m_VerticalFOV(fov), m_AspectRatio(aspectRation), m_Near(near), m_Far(far)
    {
        LM_CORE_ASSERT(aspectRation > 0.f, "Perspective camera aspect must be greater than 0.");
        LM_CORE_ASSERT(near > 0.f, "Perspective camera near must be greater than 0.");
        LM_CORE_ASSERT(far > near, "Perspective camera far plane must be greater than near plane.");

        //顺序可变换
        RecalculateProjectionMatrix();
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetPosition(const glm::vec3 &position)
    {
        m_Position = position;
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetRotation(const glm::vec3 &rotation)
    {
        m_Rotation = rotation;
        RecalculateViewMatrix();
    }

    void PerspectiveCamera::SetFOV(const float fov)
    {
        LM_CORE_ASSERT(
            fov > 0.0f && fov < 180.0f,
            "Perspective camera FOV must be between 0 and 180 degrees"
        );

        if (fov <= 0.0f || fov >= 180.0f)
            return;

        m_VerticalFOV = fov;
        RecalculateProjectionMatrix();
    }

    void PerspectiveCamera::SetAspectRatio(const float aspectRatio)
    {
        LM_CORE_ASSERT(aspectRatio > 0, "Perspective camera aspect ratio must be greater than 0.");
        if (aspectRatio <= 0.f)
            return;
        m_AspectRatio = aspectRatio;
        RecalculateProjectionMatrix();
    }

    /**
         * 镜头如何把3D压扁到2D屏幕上”
         */
    void PerspectiveCamera::RecalculateProjectionMatrix()
    {
        m_ProjectionMatrix = glm::perspective(
            glm::radians(m_VerticalFOV),
            m_AspectRatio,
            m_Near,
            m_Far
        );
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix; //p v => MVP = P V M
    }


    /**
    * 镜头在哪里 朝向哪里 怎么旋转
    */
    void PerspectiveCamera::RecalculateViewMatrix()
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);

        //旋转顺序  yaw(Y) → pitch(X) → roll(Z)
        transform = glm::rotate(transform, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

        transform = glm::rotate(transform, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

        transform = glm::rotate(transform, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        //因为CameraTransform = Translation * Rotation; 在global world中需要inverse
        m_ViewMatrix = glm::inverse(transform);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}
