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

        const bool isLooking =
            m_MouseLookEnabled &&
            Input::IsMouseButtonPressed(MouseButton::Right);
        // UE 标准：中键平移只在没有右键环视时生效。
        const bool isPanning =
            m_MouseLookEnabled && !isLooking &&
            Input::IsMouseButtonPressed(MouseButton::Middle);

        const auto [mouseX, mouseY] = Input::GetMousePos();
        const glm::vec2 currentMousePos(mouseX, mouseY);

        if (isLooking)
        {
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
                rotation.y -= mouseDelta.x * m_MouseSensitivity;
                rotation.x -= mouseDelta.y * m_MouseSensitivity;
                rotation.x = std::clamp(rotation.x, -89.0f, 89.0f);
                rotation.y = std::remainder(rotation.y, 360.0f);
                m_Camera.SetRotation(rotation);
            }
        }
        else if (m_WasMouseLooking)
        {
            // UE：松开右键恢复光标并停止环视。
            Input::SetCursorMode(CursorMode::Normal);
            m_WasMouseLooking = false;
        }

        const glm::vec3 forward = CalculateForwardDirection();
        constexpr glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
        const glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
        const glm::vec3 up = glm::normalize(glm::cross(right, forward));

        if (isLooking)
        {
            glm::vec3 movementDirection(0.0f);
            if (Input::IsKeyPressed(KeyCode::W)) movementDirection += forward;
            if (Input::IsKeyPressed(KeyCode::S)) movementDirection -= forward;
            if (Input::IsKeyPressed(KeyCode::D)) movementDirection += right;
            if (Input::IsKeyPressed(KeyCode::A)) movementDirection -= right;
            if (Input::IsKeyPressed(KeyCode::E)) movementDirection += worldUp;
            if (Input::IsKeyPressed(KeyCode::Q)) movementDirection -= worldUp;

            if (glm::dot(movementDirection, movementDirection) > 0.0f)
            {
                movementDirection = glm::normalize(movementDirection);
                constexpr float fastMoveMultiplier = 4.0f;
                const float speedMultiplier =
                    Input::IsKeyPressed(KeyCode::LeftShift)
                        ? fastMoveMultiplier
                        : 1.0f;
                glm::vec3 position = m_Camera.GetPosition();
                position += movementDirection * m_TranslationSpeed * dt * speedMultiplier;
                m_Camera.SetPosition(position);
            }
        }
        else if (isPanning)
        {
            // UE：中键拖拽平移，画面跟随鼠标（抓手感觉）。
            if (!m_WasPanning)
            {
                m_LastMousePos = currentMousePos;
                m_WasPanning = true;
            }
            else
            {
                const glm::vec2 mouseDelta = currentMousePos - m_LastMousePos;
                m_LastMousePos = currentMousePos;
                glm::vec3 position = m_Camera.GetPosition();
                position -= right * (mouseDelta.x * m_PanSensitivity);
                position += up * (mouseDelta.y * m_PanSensitivity);
                m_Camera.SetPosition(position);
            }
        }
        else if (m_WasPanning)
        {
            m_WasPanning = false;
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

    void PerspectiveCameraController::SetPanSensitivity(const float sensitivity)
    {
        m_PanSensitivity = std::max(sensitivity, 0.0f);
    }

    float PerspectiveCameraController::GetPanSensitivity() const
    {
        return m_PanSensitivity;
    }

    void PerspectiveCameraController::SetDollySpeed(const float speed)
    {
        m_DollySpeed = std::max(speed, 0.0f);
    }

    float PerspectiveCameraController::GetDollySpeed() const
    {
        return m_DollySpeed;
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
        if (!m_MouseLookEnabled)
            return false;

        // UE：飞行中滚轮调整飞行速度。
        if (Input::IsMouseButtonPressed(MouseButton::Right))
        {
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

        // UE：非飞行状态滚轮沿视线推拉。
        const glm::vec3 forward = CalculateForwardDirection();
        glm::vec3 position = m_Camera.GetPosition();
        position += forward * (event.GetYOffset() * m_DollySpeed);
        m_Camera.SetPosition(position);
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
