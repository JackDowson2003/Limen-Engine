//
// Created by chenlong on 2026/8/22.
//

#pragma once

#include "PerspectiveCamera.h"
#include "Limen/Core/Core.h"

namespace Limen
{
    class MouseScrolledEvent;
    class DeltaTime;
    class Event;

    class LIMEN_API PerspectiveCameraController
    {
    public:
        /**
         * @brief 创建透视投影的controller
         *
         * @param verticalFOV 垂直视场角
         * @param aspectRatio 宽高比
         * @param nearPlane zNear
         * @param farPlane zFar
         */
        explicit PerspectiveCameraController(
        float verticalFOV,
        float aspectRatio,
        float nearPlane = 0.1f,
        float farPlane = 100.0f
        );


        /**
        * 每帧更新相机。
        *
        * 负责处理键盘移动和鼠标右键旋转。
        *
        * @param deltaTime 当前帧与上一帧之间经过的时间。
        */
        void OnUpdate(const DeltaTime& deltaTime);

        /**
         * 接收引擎事件。
         *
         * 当前处理用于调整飞行速度的滚轮事件。
         *
         * @param event 当前事件。
         */
        void OnEvent(Event& event);

        /**
         * 更新相机使用的视口尺寸。
         *
         * 这里不会调用 glViewport，只负责更新透视投影矩阵的宽高比。
         *
         * @param width  视口宽度。
         * @param height 视口高度。
         */
        void OnResize(float width, float height);

        /**
         * 设置相机的移动速度。
         *
         * 单位为“世界单位/秒”。
         */
        void SetTranslationSpeed(float speed);

        /**
         * 设置鼠标旋转灵敏度。
         *
         * 表示鼠标每移动一个像素，相机旋转多少度。
         */
        void SetMouseSensitivity(float sensitivity);

        /**
         * 设置滚轮调整移动速度时的步长。
         */
        void SetSpeedAdjustmentStep(float step);

        /**
         * 控制是否允许鼠标右键旋转相机。
         */
        void SetMouseLookEnabled(bool enabled);

        [[nodiscard]] float GetTranslationSpeed() const;
        [[nodiscard]] float GetMouseSensitivity() const;

        PerspectiveCamera& GetCamera();
        const PerspectiveCamera& GetCamera() const;

    private:
        /**
         * 处理鼠标滚轮。
         *
         * 第一版中，滚轮用于调整相机移动速度，而不是改变 FOV。
         */
        bool OnMouseScrolled(const MouseScrolledEvent& event);

        /**
         * 根据当前 Pitch 和 Yaw 计算相机朝向。
         */
        [[nodiscard]] glm::vec3 CalculateForwardDirection() const;


        /**
         * @brief 使用 Rodrigues 公式，让向量绕任意轴旋转。
         *
         * @param vector       要旋转的向量。
         * @param axis         旋转轴，可以不是单位向量。
         * @param angleRadians 旋转角度，单位必须是弧度。
         *
         * @return 旋转后的向量。
         */
        [[nodiscard]]
        static glm::vec3 RotateAroundAxis(
            const glm::vec3 &vector,
            const glm::vec3 &axis,
            float angleRadians
        );

        PerspectiveCamera m_Camera;

        // 相机移动速度，单位为世界单位/秒。
        float m_TranslationSpeed = 3.f;

        // 鼠标每移动一个像素对应的旋转角度。
        float m_MouseSensitivity = 0.1f;

        // 每个滚轮刻度调整移动速度的比例。
        float m_SpeedAdjustmentStep = 0.5f;

        // 是否允许鼠标控制相机朝向。
        bool m_MouseLookEnabled = true;

        // 记录上一帧是否处于鼠标观察状态，避免首次计算产生跳变。
        bool m_WasMouseLooking = false;

        // 上一帧鼠标位置，用于计算相对位移。
        glm::vec2 m_LastMousePos = glm::vec2(0.0f);
    };
}
