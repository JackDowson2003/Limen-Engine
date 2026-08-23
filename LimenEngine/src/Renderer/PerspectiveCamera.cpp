//
// Created by chenlong on 2026/8/20.
//
#include "Limen/Renderer/PerspectiveCamera.h"

#include "Limen/Core/Log.h"
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

        // 两个矩阵互不依赖初始化顺序；第二次计算会得到完整的 VP 矩阵。
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

    void PerspectiveCamera::RecalculateProjectionMatrix()
    {
        m_ProjectionMatrix = glm::perspective(
            glm::radians(m_VerticalFOV),
            m_AspectRatio,
            m_Near,
            m_Far
        );
        // 顶点最终按 Projection * View * Model 的顺序变换。
        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }


    void PerspectiveCamera::RecalculateViewMatrix()
    {
        glm::mat4 transform = glm::translate(glm::mat4(1.0f), m_Position);

        // 相机局部变换按 Yaw(Y) -> Pitch(X) -> Roll(Z) 组合。
        transform = glm::rotate(transform, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));

        transform = glm::rotate(transform, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));

        transform = glm::rotate(transform, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));

        // View 矩阵需要执行相机世界变换的逆变换。
        m_ViewMatrix = glm::inverse(transform);

        m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
    }
}
