//
// Created by chenlong on 2026/8/27.
//

#pragma once
#include <vector>

#include "glm/vec3.hpp"
#include "glm/vec4.hpp"

#include "Limen/Core/DeltaTime.h"

namespace SandBox
{
    /**
     * @brief 描述新粒子被发射时的初始参数
     *
     * ParticleSpecification 只负责告诉粒子系统：
     * 新粒子从哪里来、怎么移动、如何辩护以及存活错就
     *
     * 它不是运行中的粒子对象
     */
    struct ParticleSpecification
    {

        //粒子产生时的世界空间中的位置
        glm::vec3 Position{0.f};

        //粒子每秒在空间中移动的速度
        glm::vec3 Velocity{0.f};

        // 粒子刚产生时的 RGBA 颜色。
        glm::vec4 ColorBegin{1.0f};

        // 粒子生命结束时的 RGBA 颜色。
        glm::vec4 ColorEnd{0.0f};

        /**
         * @brief 每个速度分量允许产生的随机变量的范围
         *
         * 最终速度将在:
         * Velocity - VelocityVariation
         * to
         * Velocity + VelocityVariation
         * 之间随机产生
         */
        glm::vec3 VelocityVariation{0.f};
        

        //粒子刚产生时的大小
        float SizeBegin = 1.f;

        //粒子结束时的大小
        float SizeEnd = 0.f;

        //粒子的总存活时间
        float LifeTime = 1.f;

    };

    /**
     * @brief 管理、更新和渲染一组粒子系统
     *
     * 第一版使用固定容量的 CPU 对象池。
     * 粒子系统本身不管理 Camera，也不调用
     * Renderer2D::BeginScene() 或 EndScene()
     */
    class ParticleSystem
    {
    public:
        /**
         * @brief 创建固定容量的粒子对象池
         *
         * @param maxParticles
         * 系统最多能够同时管理的粒子数量。
         * 容量在构造时一次性确定，发射时不重复申请内存。
         */
        explicit ParticleSystem(uint32_t maxParticles);

        /**
         * @brief 发射一个新粒子 负责“产生粒子”。
         *
         * 将Specification 中的初始化参数复制到对象池的一个粒子槽
         */
        void Emit(const ParticleSpecification& specification);

        /**
         * @brief 更新依然存活的粒子
         * 负责减少寿命，并根据Velocity 和 DeltaTime更新位置
         */
        void Update(const Limen::DeltaTime& deltaTime);

        /**
         * @brief 把所有存活粒子提交给Renderer2D
         *
         * 必须在Renderer2D::BeginScene()和EndScene()之间调用
         */
        void Render() const;

    private:

        /**
         * @brief 对象池中一个正在运行或等待复用的粒子。
         *
         * ParticleSpecification 是外部发射粒子时传入的初始参数；
         * Particle 则保存粒子运行过程中的实际状态。
         */
        struct Particle
        {
            // 粒子当前的世界空间位置，每帧都会更新。
            glm::vec3 Position{0.0f};

            // 粒子每秒移动的世界空间距离。
            glm::vec3 Velocity{0.0f};

            // 用于计算粒子当前颜色的起始和结束颜色。
            glm::vec4 ColorBegin{1.0f};
            glm::vec4 ColorEnd{0.0f};

            // 默认为正方体
            // 用于计算粒子当前尺寸的起始和结束尺寸。
            float SizeBegin = 1.0f;
            float SizeEnd = 0.0f;

            // 粒子的完整生命周期。
            float LifeTime = 1.0f;

            // 粒子当前还剩多少秒寿命。
            float LifeRemaining = 0.0f;

            // 当前槽位中是否保存着一个正在运行的粒子。
            bool Active = false;
        };

        //粒子对象池
        std::vector<Particle> m_Particles;

        //下一次Emit的时候的对象池的下标，到达末尾的时候回到0，从而进行循环
        //也就是从粒子系统的对象池的第一个对象的index
        //参考循环队列的index
        uint32_t m_NextParticleIndex = 0;
    };

}
