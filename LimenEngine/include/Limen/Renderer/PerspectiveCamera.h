#pragma once

#include "Camera.h"

namespace Limen
{
    /**
     * @brief 透视投影相机。
     *
     * 用于3D场景，会产生“近大远小”的视觉效果。
     * 相机默认朝向自身局部坐标系的-Z方向。
     */
    class LIMEN_API PerspectiveCamera final : public Camera
    {
    public:
        /**
         * @brief 创建透视相机。
         *
         * @param verticalFOVDegrees
         * 垂直视场角，单位为度。
         * 数值越大，看到的范围越广，但透视变形越明显。
         * 常用范围为30～90度，例如45度。
         *
         * @param aspectRatio
         * 视口宽高比，计算方式为：
         *
         *     framebufferWidth / framebufferHeight
         *
         * 例如1600×900窗口的宽高比是：
         *
         *     1600.0f / 900.0f
         *
         * @param nearPlane
         * 近裁剪面到相机的正距离，必须大于0。
         * 它不是世界坐标中的Z值。
         * 常用值为0.1。
         *
         * @param farPlane
         * 远裁剪面到相机的正距离，必须大于nearPlane。
         * 常用值为100或1000。
         */
        PerspectiveCamera(
            float verticalFOVDegrees,
            float aspectRatio,
            float nearPlane = 0.1f,
            float farPlane = 100.0f
        );

        /**
         * @brief 设置相机在世界空间中的位置。
         *
         * @param position
         * 相机的世界坐标，单位由引擎自行约定。
         * 例如(0, 0, 3)表示相机位于世界原点前方3个单位。
         *
         * 调用后会重新计算View矩阵和ViewProjection矩阵。
         */
        void SetPosition(const glm::vec3& position);

        /**
         * @brief 设置相机在世界空间中的欧拉角。
         *
         * @param rotationDegrees
         * 三个分量的单位都是度：
         *
         *     x = pitch，绕X轴旋转
         *     y = yaw，绕Y轴旋转
         *     z = roll，绕Z轴旋转
         *
         * 调用后会重新计算View矩阵和ViewProjection矩阵。
         */
        void SetRotation(const glm::vec3& rotationDegrees);

        /**
         * @brief 设置相机的垂直视场角。
         *
         * @param verticalFOVDegrees
         * 垂直视场角，单位为度，必须位于(0, 180)范围内。
         *
         * 调用后会重新计算Projection矩阵和ViewProjection矩阵。
         */
        void SetFOV(float verticalFOVDegrees);

        /**
         * @brief 设置相机视口的宽高比。
         *
         * @param aspectRatio
         * 宽度除以高度，必须大于0。
         *
         * 例如：
         *
         *     1600.0f / 900.0f
         *
         * 调用后会重新计算Projection矩阵和ViewProjection矩阵。
         */
        void SetAspectRatio(float aspectRatio);

        /**
         * @brief 获取相机的世界空间位置。
         */
        [[nodiscard]]
        const glm::vec3& GetPosition() const override
        {
            return m_Position;
        }

        /**
         * @brief 获取相机的欧拉角，单位为度。
         */
        [[nodiscard]]
        const glm::vec3& GetRotation() const
        {
            return m_Rotation;
        }

        /**
         * @brief 获取透视投影矩阵。
         *
         * 该矩阵负责透视效果、FOV、宽高比和裁剪范围。
         */
        [[nodiscard]]
        const glm::mat4& GetProjectionMatrix() const override
        {
            return m_ProjectionMatrix;
        }

        /**
         * @brief 获取观察矩阵。
         *
         * View矩阵是相机世界变换的逆矩阵，
         * 用于把世界空间坐标转换到相机空间。
         */
        [[nodiscard]]
        const glm::mat4& GetViewMatrix() const override
        {
            return m_ViewMatrix;
        }

        /**
         * @brief 获取Projection × View矩阵。
         *
         * Shader中通常使用：
         *
         *     gl_Position =
         *         ViewProjection * Model * localPosition;
         */
        [[nodiscard]]
        const glm::mat4& GetViewProjectionMatrix() const override
        {
            return m_ViewProjectionMatrix;
        }

    private:
        /**
         * @brief 根据FOV、宽高比、near和far重新计算投影矩阵。
         */
        void RecalculateProjectionMatrix();

        /**
         * @brief 根据相机位置和旋转重新计算观察矩阵。
         */
        void RecalculateViewMatrix();

        // 将相机空间坐标投影到裁剪空间。
        glm::mat4 m_ProjectionMatrix{1.0f};

        // 将世界空间坐标转换到相机空间。
        glm::mat4 m_ViewMatrix{1.0f};

        // Projection × View，供Renderer提交给Shader。
        glm::mat4 m_ViewProjectionMatrix{1.0f};

        // 相机在世界空间中的位置。
        // 默认放在z=3处，能够看到世界原点附近的物体。
        glm::vec3 m_Position{0.0f, 0.0f, 3.0f};

        // 欧拉角，单位为度：x=pitch、y=yaw、z=roll。
        glm::vec3 m_Rotation{0.0f};

        // 垂直视场角，单位为度。
        float m_VerticalFOV = 45.0f;

        // 视口宽度除以视口高度。
        float m_AspectRatio = 16.0f / 9.0f;

        // 近裁剪面到相机的正距离。
        float m_NearPlane = 0.1f;

        // 远裁剪面到相机的正距离。
        float m_FarPlane = 100.0f;
    };
}