//
// Created by chenlong on 2026/8/22.
//

#pragma once

#include "Limen/Core/Core.h"
#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/Event.h"
#include "Limen/Renderer/OrthoGraphicCamera.h"

namespace Limen
{
    class WindowResizeEvent;
    class MouseScrolledEvent;

    /**
     * @brief 正交相机输入控制器。
     *
     * Controller 负责根据键盘、鼠标滚轮和视口尺寸改变相机状态；
     * OrthoGraphicCamera 只负责保存相机状态并计算矩阵。
     */
    class LIMEN_API OrthoGraphicCameraController
    {
    public:
        /**
         * @brief 创建正交相机输入控制器。
         *
         * @param aspectRatio 当前视口宽高比，即 width / height。
         * @param enableRotation 是否允许用户通过输入旋转相机。
         */
        explicit OrthoGraphicCameraController(float aspectRatio, bool enableRotation = false);

        /**
         * @brief 每帧更新相机的位置和旋转。
         *
         * 该函数会读取当前键盘状态，并使用 deltaTime
         * 保证相机移动速度不受帧率影响。
         *
         * @param deltaTime 当前帧和上一帧之间经过的时间。
         */
        void OnUpdate(const DeltaTime &deltaTime);

        /**
         * @brief 处理传递给控制器的事件。
         *
         * 当前主要处理：
         * - MouseScrolledEvent：调整正交相机缩放。
         * - WindowResizeEvent：更新相机宽高比。
         *
         * @param event Application 传递下来的引擎事件。
         */
        void OnEvent(Event &event);

        /**
         * @brief 通知控制器实际渲染视口的尺寸发生变化。
         *
         * 该函数只更新相机投影比例，不负责修改GPU Viewport。
         *
         * @param width 视口宽度，单位为像素。
         * @param height 视口高度，单位为像素。
         */
        void OnResize(float width, float height);

        /**
         * @brief 设置每个滚轮刻度改变多少 ZoomLevel。
         */
        void SetZoomSpeed(float zoomSpeed);

        /**
        * @brief 设置正交相机的缩放等级。
        *
        * 数值越小，看到的世界范围越小，物体看起来越大；
        * 数值越大，看到的世界范围越大，物体看起来越小。
        *
        * @param zoomLevel 新缩放等级，必须大于 0。
        */
        void SetZoomLevel(float zoomLevel);

        /**
         * @brief 设置相机平移速度。
         *
         * @param translationSpeed 用户持续按住移动键时，相机每秒移动的世界单位数量。
         * 该值不应该小于0。
         */
        void SetTranslationSpeed(float translationSpeed);

        /**
         * @brief 设置相机旋转速度。
         *
         * @param rotationSpeed 用户持续按住旋转键时，相机每秒旋转的角度。
         * 单位为度每秒，不应该小于0。
         */
        void SetRotationSpeed(float rotationSpeed);

        /**
         * @brief 启用或关闭相机旋转控制。
         *
         * @param enabled true 表示允许旋转；false 表示忽略旋转输入。
         */
        void SetRotationEnabled(bool enabled)
        {
            m_RotationEnabled = enabled;
        }

        [[nodiscard]]
        float GetZoomLevel() const
        {
            return m_ZoomLevel;
        }

        [[nodiscard]]
        float GetZoomSpeed() const
        {
            return m_ZoomSpeed;
        }

        [[nodiscard]]
        float GetTranslationSpeed() const
        {
            return m_TranslationSpeed;
        }

        [[nodiscard]]
        float GetRotationSpeed() const
        {
            return m_RotationSpeed;
        }

        [[nodiscard]]
        bool IsRotationEnabled() const
        {
            return m_RotationEnabled;
        }

        /**
         * @brief 获取控制器持有的可修改相机。
         */
        [[nodiscard]]
        OrthoGraphicCamera &GetCamera()
        {
            return m_Camera;
        }

        /**
         * @brief 获取控制器持有的只读相机。
         */
        [[nodiscard]]
        const OrthoGraphicCamera &GetCamera() const
        {
            return m_Camera;
        }

    private:
        /**
         * @brief 根据当前宽高比和缩放等级重新计算投影范围。
         */
        void RecalculateProjection();

        bool OnMouseScrolled(const MouseScrolledEvent &event);

        bool OnViewportResized(const WindowResizeEvent &event);
    private:
        // 当前视口的宽高比。
        float m_AspectRatio = 1.0f;

        // 垂直可见范围的一半；值越小，画面中的物体看起来越大。
        float m_ZoomLevel = 1.0f;

        // 每个滚轮刻度改变的缩放等级。
        float m_ZoomSpeed = 0.25f;

        // 缩放下限，避免产生零或负的投影范围。
        float m_MinZoomLevel = 0.25f;

        // 相机每秒移动的世界单位数。
        float m_TranslationSpeed = 1.0f;

        // 相机每秒旋转的角度。
        float m_RotationSpeed = 180.0f;

        // 是否响应旋转输入。
        bool m_RotationEnabled = false;

        // Controller 独占 Camera 的生命周期，无需堆分配或共享所有权。
        OrthoGraphicCamera m_Camera;
    };
}
