//
// Created by chenlong on 2026/11/21.
//

#pragma once

#include "glm/mat4x4.hpp"
#include "glm/vec2.hpp"
#include "glm/vec3.hpp"

#include "Limen/Core/Core.h"
#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "Limen/Scene/SceneTransform.h"
#include "UI/EditorUILayout.h"

namespace Limen
{
    class PerspectiveCameraController;
    class SceneRenderer;
    class Event;
    class DeltaTime;
    class Window;
}

namespace LimenEditor
{
    class EditorUIRenderer;
    class EditorSession;

    // 场景视口面板：拥有离屏的渲染器与相机，只借用EditorSession 的场景
    class SceneViewportPanel final
    {
    public:
        SceneViewportPanel() = default;
        ~SceneViewportPanel();

        // Don't allow to copy this class
        SceneViewportPanel(const SceneViewportPanel&) = delete;
        SceneViewportPanel& operator=(const SceneViewportPanel&) = delete;

        bool Init();
        void Shutdown();

        // 每帧 hover/相机, 按中央区域像素尺寸渲染会话场景
        void Update(
            const EditorSession& session,
            const Limen::Window& window,
            const Limen::DeltaTime& deltaTime,
            const UIRect& centerRect
        );

        // 把 SceneRenderer 的最终颜色贴到中央区域。
        void Draw(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const UIRect &centerRect
        );

        // 处理视口输入；返回 true 表示事件被视口消费。
        bool OnEvent(Limen::Event &event);

        // 在视口内用鼠标射线拾取对象；toggle 对应 Cmd/Ctrl 增选。
        bool PickAndSelect(
            EditorSession &session,
            float x,
            float y,
            bool toggle
        );

        // 将选中对象放到画面中央，保留当前相机朝向。
        [[nodiscard]] bool FrameSelection(const EditorSession &session);

        // 在视口前方生成对象；失败时不修改 outTransform。
        [[nodiscard]]
        bool TryGetPlacementTransform(
            Limen::SceneTransform &outTransform,
            float distance = 4.0f) const;

        // 按视口内逻辑坐标生成放置矩阵；坐标不在视口内或视口未就绪时失败。
        [[nodiscard]]
        bool TryGetPlacementTransformAt(
            float x,
            float y,
            Limen::SceneTransform &outTransform,
            float distance = 4.0f) const;

        [[nodiscard]]
        bool ContainsPoint(float x, float y) const noexcept;

        // 将世界空间 AABB 投影为视口内屏幕矩形；失败时不修改输出。
        [[nodiscard]]
        bool TryProjectWorldBounds(
            const Limen::AxisAlignedBoundingBox &bounds,
            glm::vec2 &outMin,
            glm::vec2 &outMax) const;

        // 窗口最小化或尺寸归零时取消导航并恢复光标。
        void CancelNavigation();

        // 是否已经就绪
        [[nodiscard]]
        bool IsReady() const noexcept
        {
            return m_Renderer && m_CameraController;
        }

    private:
        [[nodiscard]]
        bool TryBuildWorldRay(
            float x,
            float y,
            glm::vec3 &outOrigin,
            glm::vec3 &outDirection) const;

        Limen::Scope<Limen::SceneRenderer> m_Renderer;
        Limen::Scope<Limen::PerspectiveCameraController> m_CameraController;
        UIRect m_CenterRect{};
        bool m_IsHovered = false;
        bool m_IsNavigating = false;
    };
}
