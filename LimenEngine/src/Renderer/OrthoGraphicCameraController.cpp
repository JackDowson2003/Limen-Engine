//
// Created by chenlong on 2026/8/22.
//
#include "Limen/Renderer/OrthoGraphicCameraController.h"

#include <algorithm>
#include <cmath>

#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "Limen/Events/ApplicationEvent.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"

namespace Limen
{
    OrthoGraphicCameraController::OrthoGraphicCameraController(const float aspectRatio, const bool enableRotation)
        : m_AspectRatio(aspectRatio > 0.0f ? aspectRatio : 1.0f),
          m_RotationEnabled(enableRotation),
          m_Camera(-1.0f, 1.0f, -1.0f, 1.0f)
    {
        RecalculateProjection();
    }

    void OrthoGraphicCameraController::OnUpdate(const DeltaTime &deltaTime)
    {
        const float deltaSeconds = deltaTime.GetSeconds();

        glm::vec3 cameraPos = m_Camera.GetPosition();

        const float rotationRadians = glm::radians(m_Camera.GetRotation());

        // 根据当前旋转求相机局部右方向。
        const glm::vec2 cameraRight(
            std::cos(rotationRadians),
            std::sin(rotationRadians)
        );

        // 根据当前旋转求相机局部上方向。
        const glm::vec2 cameraUp(
            -std::sin(rotationRadians),
             std::cos(rotationRadians)
        );

        glm::vec2 movementDirection(0.0f);

        if (Input::IsKeyPressed(KeyCode::W))
            movementDirection += cameraUp;

        if (Input::IsKeyPressed(KeyCode::S))
            movementDirection -= cameraUp;

        if (Input::IsKeyPressed(KeyCode::D))
            movementDirection += cameraRight;

        if (Input::IsKeyPressed(KeyCode::A))
            movementDirection -= cameraRight;


        // 归一化后，斜向移动不会比单方向移动更快。
        if (glm::dot(movementDirection, movementDirection) > 0.0f)
        {
            movementDirection = glm::normalize(movementDirection);

            const float moveDistance = m_TranslationSpeed * deltaSeconds;

            cameraPos.x += movementDirection.x * moveDistance;
            cameraPos.y += movementDirection.y * moveDistance;

            m_Camera.SetPosition(cameraPos);
        }

        if (!m_RotationEnabled)
            return;

        float rotationInput = 0.0f;

        if (Input::IsKeyPressed(KeyCode::Q))
            rotationInput += 1.0f;

        if (Input::IsKeyPressed(KeyCode::E))
            rotationInput -= 1.0f;

        if (rotationInput != 0.0f)
        {
            float cameraRotation = m_Camera.GetRotation();

            cameraRotation += rotationInput * m_RotationSpeed * deltaSeconds;

            // 把角度保持在[-180, 180]范围附近。
            cameraRotation = std::remainder(cameraRotation, 360.0f);
            m_Camera.SetRotation(cameraRotation);
        }
    }

    void OrthoGraphicCameraController::OnEvent(Event &event)
    {
        EventDispatcher dispatcher(event);
        dispatcher.Dispatch<MouseScrolledEvent>(
            [this](const MouseScrolledEvent& mouseEvent)
            {
                return OnMouseScrolled(mouseEvent);
            }
        );

        dispatcher.Dispatch<WindowResizeEvent>(
            [this](const WindowResizeEvent& resizeEvent)
            {
                return OnViewportResized(resizeEvent);
            }
        );
    }

    void OrthoGraphicCameraController::OnResize(const float width, const float height)
    {
        // 窗口最小化时尺寸可能为 0，此时不能计算宽高比。
        if (width <= 0.0f || height <= 0.0f)
            return;

        m_AspectRatio = width / height;
        RecalculateProjection();
    }

    void OrthoGraphicCameraController::SetZoomSpeed(const float zoomSpeed)
    {
        m_ZoomSpeed = std::max(zoomSpeed, 0.0f);
    }

    void OrthoGraphicCameraController::RecalculateProjection()
    {
        /*
         * ZoomLevel 是垂直可见范围的一半；
         * 乘宽高比得到水平方向的一半。
         */
        const float horizontalHalfSize =
            m_AspectRatio * m_ZoomLevel;

        m_Camera.SetProjection(
            -horizontalHalfSize,
             horizontalHalfSize,
            -m_ZoomLevel,
             m_ZoomLevel
        );
    }

    bool OrthoGraphicCameraController::OnMouseScrolled(const MouseScrolledEvent &event)
    {
        SetZoomLevel(
            m_ZoomLevel - event.GetYOffset() * m_ZoomSpeed
        );

        // 允许其他 Layer 继续处理滚轮事件。
        return false;
    }

    bool OrthoGraphicCameraController::OnViewportResized(const WindowResizeEvent &event)
    {
        OnResize(
            static_cast<float>(event.GetWidth()),
            static_cast<float>(event.GetHeight())
        );

        // Resize 通常需要被多个系统共同处理。
        return false;
    }



    void OrthoGraphicCameraController::SetZoomLevel(const float zoomLevel)
    {
        m_ZoomLevel = std::max(zoomLevel, m_MinZoomLevel);
        RecalculateProjection();
    }

    void OrthoGraphicCameraController::SetTranslationSpeed(const float translationSpeed)
    {
        m_TranslationSpeed = std::max(translationSpeed, 0.0f);
    }

    void OrthoGraphicCameraController::SetRotationSpeed(const float rotationSpeed)
    {
        m_RotationSpeed = std::max(rotationSpeed, 0.0f);
    }
}
