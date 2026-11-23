//
// Created by chenlong on 2026/11/21.
//
#include "SceneViewportPanel.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/matrix.hpp>

#include "Limen/Application/Window.h"
#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/EditorViewportProjection.h"


namespace LimenEditor
{
    SceneViewportPanel::~SceneViewportPanel() = default;

    bool SceneViewportPanel::Init()
    {
        Limen::SceneRendererSpecification spec;
        spec.Width = 1;
        spec.Height = 1;
        spec.DebugName = "Editor Scene Viewport";

        m_Renderer = Limen::CreateScope<Limen::SceneRenderer>(spec);
        m_CameraController = Limen::CreateScope<
            Limen::PerspectiveCameraController>(45.0f, 1.0f, 0.1f, 100.f);

        if (!m_Renderer || !m_CameraController)
        {
            LM_CORE_ERROR(
                "[SceneViewportPanel::Init]: Error, SceneRender or PerspectiveCameraController creating failed");
            Shutdown();
            return false;
        }
        // 初始相机放在立方体前方，保证新建对象可见。
        m_CameraController->GetCamera().SetPosition({0.0f, 0.0f, 4.0f});
        return true;
    }

    void SceneViewportPanel::Shutdown()
    {
        m_CameraController.reset();
        m_Renderer.reset();
    }

    void SceneViewportPanel::Update(
        const EditorSession &session,
        const Limen::Window &window,
        const Limen::DeltaTime &deltaTime,
        const UIRect &centerRect)
    {
        if (!m_Renderer || !m_CameraController)
        {
            LM_CORE_ERROR("[SceneViewportPanel::Update]: SceneRender or CameraController is null");
            return;
        }
        m_CenterRect = centerRect;

        // hover 使用逻辑窗口坐标，离开视口后由控制器恢复光标
        const auto [mouseX,mouseY] = Limen::Input::GetMousePos();

        m_IsHovered = m_CenterRect.Contains(mouseX, mouseY);
        m_CameraController->SetMouseLookEnabled(m_IsHovered || m_IsNavigating);
        m_CameraController->OnUpdate(deltaTime);

        const float logicalWidth = static_cast<float>(window.GetWindowWidth());
        const float logicalHeight = static_cast<float>(window.GetWindowHeight());
        if (logicalWidth <= 0.0f || logicalHeight <= 0.0f)
            return;

        const ViewportPixelSize pixelSize = CalculateViewportPixelSize(
            logicalWidth,
            logicalHeight,
            m_CenterRect,
            window.GetWidth(),
            window.GetHeight()
        );
        const auto &spec = m_Renderer->GetSpecification();
        if (spec.Width != pixelSize.Width || spec.Height != pixelSize.Height)
        {
            m_Renderer->Resize(pixelSize.Width, pixelSize.Height);
            m_CameraController->GetCamera().SetAspectRatio(
                static_cast<float>(pixelSize.Width) /
                static_cast<float>(pixelSize.Height)
            );
        }
        m_Renderer->Render(session.GetScene(), m_CameraController->GetCamera());
    }

    void SceneViewportPanel::Draw(const Limen::Window &window, const EditorUIRenderer &renderer,
                                  const UIRect &centerRect)
    {
        if (!m_Renderer ||
            centerRect.Width <= 0.0f ||
            centerRect.Height <= 0.0f
        )
        {
            return;
        }

        const std::uintptr_t handle = m_Renderer->GetFinalColorAttachmentHandle();
        if (handle == 0)
            return;

        renderer.DrawNativeTextureRect(
            window,
            {centerRect.X, centerRect.Y},
            {centerRect.Width, centerRect.Height},
            handle);
    }

    bool SceneViewportPanel::OnEvent(Limen::Event &event)
    {
        // 滚轮只在光标位于视口内时交给相机控制器
        if (event.GetEventType() == Limen::EventType::MouseScrolled)
        {
            if (!m_IsHovered || !m_IsNavigating)
                return false;
            m_CameraController->OnEvent(event);
            return true;
        }

        Limen::EventDispatcher dispatcher(event);
        bool handled = false;

        dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
            [this,& handled](const Limen::MouseButtonPressedEvent &press)
            {
                const auto button = press.GetButton();
                if (button != Limen::MouseButton::Right && button != Limen::MouseButton::Middle)
                    return false;
                const auto [x, y] = Limen::Input::GetMousePos();
                if (!m_CenterRect.Contains(x, y))
                    return false;

                m_IsNavigating = true;
                handled = true;
                return true;
            }
        );
        if (handled)
            return true;
        dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
           [this, &handled](const Limen::MouseButtonReleasedEvent &release)
           {
               const auto button = release.GetButton();
               if ((button != Limen::MouseButton::Right &&button != Limen::MouseButton::Middle) ||
                   !m_IsNavigating)
                   return false;

               m_IsNavigating = false;
               handled = true;
               return true;
           });

        return handled;
    }

    bool SceneViewportPanel::PickAndSelect(
        EditorSession &session,
        const float x,
        const float y,
        const bool toggle)
    {
        glm::vec3 origin{};
        glm::vec3 direction{};
        if (!TryBuildWorldRay(x, y, origin, direction))
            return false;

        Limen::SceneObjectID hit{};
        if (session.GetScene().PickObject(origin, direction, hit))
        {
            if (toggle)
                session.ToggleObjectSelection(hit);
            else
                session.SelectObject(hit);
        }
        else if (!toggle)
        {
            // 点击视口空白处清空对象选择。
            session.ClearSelection();
        }

        return true;
    }

    bool SceneViewportPanel::TryGetPlacementTransform(
        Limen::SceneTransform &outTransform,
        const float distance) const
    {
        return TryGetPlacementTransformAt(
            m_CenterRect.X + m_CenterRect.Width * 0.5f,
            m_CenterRect.Y + m_CenterRect.Height * 0.5f,
            outTransform,
            distance);
    }

    bool SceneViewportPanel::TryGetPlacementTransformAt(
        const float x,
        const float y,
        Limen::SceneTransform &outTransform,
        const float distance) const
    {
        if (distance <= 0.0f)
            return false;

        glm::vec3 origin{};
        glm::vec3 direction{};
        if (!TryBuildWorldRay(x, y, origin, direction))
            return false;

        // 对象沿鼠标射线放在相机前方指定距离。
        Limen::SceneTransform placement;
        placement.Location =
            m_CameraController->GetCamera().GetPosition() +
            direction * distance;
        outTransform = placement;
        return true;
    }

    bool SceneViewportPanel::ContainsPoint(
        const float x,
        const float y) const noexcept
    {
        return m_CameraController &&
               m_CenterRect.Width > 0.0f &&
               m_CenterRect.Height > 0.0f &&
               m_CenterRect.Contains(x, y);
    }

    bool SceneViewportPanel::TryProjectWorldBounds(
        const Limen::AxisAlignedBoundingBox &bounds,
        glm::vec2 &outMin,
        glm::vec2 &outMax) const
    {
        if (!m_CameraController)
            return false;
        return TryProjectWorldBoundsToScreen(
            m_CameraController->GetCamera().GetViewProjectionMatrix(),
            m_CenterRect,
            bounds,
            outMin,
            outMax);
    }

    bool SceneViewportPanel::TryBuildWorldRay(
        const float x,
        const float y,
        glm::vec3 &outOrigin,
        glm::vec3 &outDirection) const
    {
        if (!ContainsPoint(x, y))
            return false;

        const float localX = (x - m_CenterRect.X) / m_CenterRect.Width;
        const float localY = (y - m_CenterRect.Y) / m_CenterRect.Height;
        const float ndcX = localX * 2.0f - 1.0f;
        const float ndcY = 1.0f - localY * 2.0f;

        const glm::mat4 inverseViewProjection = glm::inverse(
            m_CameraController->GetCamera().GetViewProjectionMatrix());
        glm::vec4 nearPoint = inverseViewProjection *
            glm::vec4{ndcX, ndcY, -1.0f, 1.0f};
        glm::vec4 farPoint = inverseViewProjection *
            glm::vec4{ndcX, ndcY, 1.0f, 1.0f};
        if (nearPoint.w == 0.0f || farPoint.w == 0.0f)
            return false;

        nearPoint /= nearPoint.w;
        farPoint /= farPoint.w;

        const glm::vec3 origin{nearPoint};
        const glm::vec3 unnormalizedDirection =
            glm::vec3{farPoint} - origin;
        if (glm::dot(unnormalizedDirection, unnormalizedDirection) <= 0.0f)
            return false;

        outOrigin = origin;
        outDirection = glm::normalize(unnormalizedDirection);
        return true;
    }

    void SceneViewportPanel::CancelNavigation()
    {
        m_IsHovered = false;
        m_IsNavigating = false;
        if (m_CameraController)
            m_CameraController->SetMouseLookEnabled(false);
    }
}
