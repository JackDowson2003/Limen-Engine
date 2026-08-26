//
// Created by chenlong on 2026/8/22.
//
#include "Limen/Renderer/PerspectiveCameraController.h"

#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/ApplicationEvent.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

namespace Limen
{
    PerspectiveCameraController::PerspectiveCameraController(float verticalFOV, float aspectRatio, float nearPlane,
                                                             float farPlane)
        : m_Camera(verticalFOV, aspectRatio, nearPlane, farPlane)
    {
    }

    void PerspectiveCameraController::OnUpdate(const DeltaTime &deltaTime)
    {
        const float dt = deltaTime.GetSeconds();

        const bool isNavigating =
            m_MouseLookEnabled &&
            Input::IsMouseButtonPressed(MouseButton::Right);

        // 只有按住鼠标右键时才锁定光标，并响应飞行相机输入。
        if (!isNavigating)
        {
            if (m_WasMouseLooking)
                Input::SetCursorMode(CursorMode::Normal);

            m_WasMouseLooking = false;
            return;
        }

        const auto [mouseX, mouseY] = Input::GetMousePos();
        const glm::vec2 currentMousePos(mouseX, mouseY);

        // 右键刚按下时只记录起点，避免第一次旋转发生跳变。
        if (!m_WasMouseLooking)
        {
            m_LastMousePos = currentMousePos;
            m_WasMouseLooking = true;
            Input::SetCursorMode(CursorMode::Locked);
        }
        else
        {
            const glm::vec2 mouseDelta = currentMousePos - m_LastMousePos;
            m_LastMousePos = currentMousePos;

            glm::vec3 rotation = m_Camera.GetRotation();

            /*
             * 当前相机默认朝向-Z，使用右手坐标系：
             * 绕+Y的正角度会转向世界-X。
             * 因此鼠标向右时要减小Yaw，才能像UE一样向世界+X转动。
             */
            rotation.y -= mouseDelta.x * m_MouseSensitivity;

            // 窗口坐标Y轴向下，所以鼠标向上时通过减法增加Pitch。
            rotation.x -= mouseDelta.y * m_MouseSensitivity;

            // 避免Forward与WorldUp平行，导致Right无法计算。
            rotation.x = std::clamp(rotation.x, -89.0f, 89.0f);
            rotation.y = std::remainder(rotation.y, 360.0f);

            m_Camera.SetRotation(rotation);
        }

        // 使用 Rodrigues 公式得到旋转后的相机前向。
        const glm::vec3 forward = CalculateForwardDirection();
        constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        // 前向改变后重新计算相机局部右方向。
        const glm::vec3 rightAxisVec = glm::normalize(glm::cross(forward, worldUp));

        glm::vec3 movementDirection(0.0f);

        // 编辑器飞行相机：WASD前后左右，E/Q上升下降。
        if (Input::IsKeyPressed(KeyCode::W))
            movementDirection += forward;
        if (Input::IsKeyPressed(KeyCode::S))
            movementDirection -= forward;
        if (Input::IsKeyPressed(KeyCode::D))
            movementDirection += rightAxisVec;
        if (Input::IsKeyPressed(KeyCode::A))
            movementDirection -= rightAxisVec;

        if (Input::IsKeyPressed(KeyCode::E))
            movementDirection += worldUp;
        if (Input::IsKeyPressed(KeyCode::Q))
            movementDirection -= worldUp;

        if (glm::dot(movementDirection, movementDirection) > 0.0f)
        {
            movementDirection = glm::normalize(movementDirection);

            // Shift 对应编辑器飞行相机的临时加速导航。
            constexpr float fastMoveMultiplier = 4.0f;
            const float speedMultiplier =
                Input::IsKeyPressed(KeyCode::LeftShift)
                    ? fastMoveMultiplier
                    : 1.0f;

            glm::vec3 position = m_Camera.GetPosition();
            position += movementDirection
                * m_TranslationSpeed
                * dt
                * speedMultiplier;

            m_Camera.SetPosition(position);
        }
    }

    void PerspectiveCameraController::OnEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>([this](const MouseScrolledEvent &event)
        {
            return OnMouseScrolled(event);
        });

    }

    void PerspectiveCameraController::OnResize(const float width, const float height)
    {
        // 窗口最小化时可能收到零尺寸，不能据此计算宽高比。
        if (width <= 0.0f || height <= 0.0f)
            return;

        const float aspectRatio = width / height;
        m_Camera.SetAspectRatio(aspectRatio);
    }

    void PerspectiveCameraController::SetTranslationSpeed(const float speed)
    {
        m_TranslationSpeed = std::max(speed, 0.0f);
    }

    void PerspectiveCameraController::SetMouseSensitivity(const float sensitivity)
    {
        m_MouseSensitivity = std::max(0.0f, sensitivity);
    }

    void PerspectiveCameraController::SetSpeedAdjustmentStep(const float step)
    {
        m_SpeedAdjustmentStep = std::max(step, 0.0f);
    }

    void PerspectiveCameraController::SetMouseLookEnabled(const bool enabled)
    {
        if (!enabled && m_WasMouseLooking)
        {
            Input::SetCursorMode(CursorMode::Normal);
            m_WasMouseLooking = false;
        }

        m_MouseLookEnabled = enabled;
    }

    float PerspectiveCameraController::GetTranslationSpeed() const
    {
        return m_TranslationSpeed;
    }

    float PerspectiveCameraController::GetMouseSensitivity() const
    {
        return m_MouseSensitivity;
    }

    PerspectiveCamera &PerspectiveCameraController::GetCamera()
    {
        return m_Camera;
    }

    const PerspectiveCamera &PerspectiveCameraController::GetCamera() const
    {
        return m_Camera;
    }

    bool PerspectiveCameraController::OnMouseScrolled(const MouseScrolledEvent &event)
    {
        // 按住右键滚轮调整飞行速度，而不是改变FOV。
        if (!m_MouseLookEnabled || !Input::IsMouseButtonPressed(MouseButton::Right))
            return false;

        const float speedFactor = std::pow(
            1.0f + m_SpeedAdjustmentStep,
            event.GetYOffset()
        );

        constexpr float minSpeed = 0.05f;
        constexpr float maxSpeed = 100.0f;
        SetTranslationSpeed(std::clamp(
            m_TranslationSpeed * speedFactor,
            minSpeed,
            maxSpeed
        ));

        return false;
    }


    glm::vec3 PerspectiveCameraController::RotateAroundAxis(const glm::vec3 &vector, const glm::vec3 &axis,
                                                            const float angleRadians)
    {
        const glm::vec3 rotationAxis = glm::normalize(axis);

        const float cosTheta = glm::cos(angleRadians);
        const float sinTheta = glm::sin(angleRadians);

        return vector * cosTheta
               + glm::cross(rotationAxis, vector) * sinTheta
               + rotationAxis * glm::dot(rotationAxis, vector)
               * (1.0f - cosTheta);
    }

    glm::vec3 PerspectiveCameraController::CalculateForwardDirection() const
    {
        const glm::vec3 rotation = m_Camera.GetRotation();

        const float pitch = glm::radians(rotation.x);
        const float yaw = glm::radians(rotation.y);

        constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);

        glm::vec3 forward(0.0f, 0.0f, -1.0f);

        // Yaw 绕世界 Y 轴旋转。
        forward = RotateAroundAxis(forward, worldUp, yaw);

        // Yaw 后，相机的局部右方向也发生了变化。
        const glm::vec3 right = glm::normalize(
            glm::cross(forward, worldUp)
        );

        forward = RotateAroundAxis(forward, right, pitch);

        return glm::normalize(forward);
    }
}
