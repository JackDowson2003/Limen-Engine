//
// Created by chenlong on 2026/8/27.
//

#include "ParticleSystem.h"

#include <random>

#include "Limen/Core/Core.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer2D.h"

namespace SandBox
{

    namespace
    {
        /**
        * @brief 生成一个位于 [-1, 1] 范围内的随机浮点数。
        *
        * 固定随机种子可以让每次运行产生相同的随机序列，
        * 便于复现和调试粒子效果。
        */
        float RandomSignedUnit()
        {
            //使用固定种子后，每次运行程序都会得到相同的粒子随机序列：
            static std::mt19937 generator{0xC0FFBEu};

            static std::uniform_real_distribution<float> distribution(-1.f, 1.f);

            return distribution(generator);
        }
    }

    /*
     * 构造时一次性创建固定数量的 Particle。
     *
     * 如果 Release 构建关闭了断言，并且调用者错误地传入0，
     * 至少保留一个槽位，避免后续循环对象池时发生除以0。
     */
    ParticleSystem::ParticleSystem(const uint32_t maxParticles)
        : m_Particles(maxParticles > 0 ? maxParticles : 1)
    {
        LM_ASSERT(maxParticles >0, "Particle system should have at least one particle.");
    }

    void ParticleSystem::Emit(const ParticleSpecification &specification)
    {
        /**
         * 取得循环对象池中下一处可以写入的粒子槽位。
         * 这里使用引用，因此下面的修改会直接作用于
         * m_Particles 中保存的真实 Particle。
         */
        Particle &particle = m_Particles[m_NextParticleIndex];

        /*
         * LifeTime 必须大于0，否则粒子产生后会立即死亡，
         * 并且后续计算生命比例时可能发生除以0。
         */
        LM_ASSERT(
            specification.LifeTime > 0.0f,
            "Particle lifetime must be greater than 0s"
        );

        if (specification.LifeTime <= 0.0f)
            return;

        //激活粒子并复制参数
        particle.Active = true;

        // 复制粒子的初始运动状态。
        particle.Position = specification.Position;

        //粒子的速记速的随机扰动值
        const glm::vec3 randomVariation{
            RandomSignedUnit(),
            RandomSignedUnit(),
            RandomSignedUnit()
        };

        particle.Velocity = specification.Velocity + randomVariation* specification.VelocityVariation;

        // 复制颜色变化范围。
        particle.ColorBegin = specification.ColorBegin;
        particle.ColorEnd = specification.ColorEnd;

        // 复制尺寸变化范围。
        particle.SizeBegin = specification.SizeBegin;
        particle.SizeEnd = specification.SizeEnd;

        /*
         * LifeTime 保存完整寿命，后续不会减少；
         * LifeRemaining 保存剩余寿命，每帧都会减少。
         */
        particle.LifeTime = specification.LifeTime;
        particle.LifeRemaining = specification.LifeTime;

        //到达末尾进行循环
        m_NextParticleIndex = (m_NextParticleIndex + 1) % m_Particles.size();
    }

    void ParticleSystem::Update(const Limen::DeltaTime &deltaTime)
    {
        for (auto &particle: m_Particles)
        {
            if (!particle.Active)
                continue;

            particle.LifeRemaining -= deltaTime.GetSeconds();
            if (particle.LifeRemaining <= 0.0f)
            {
                particle.Active = false;
                continue;
            }

            glm::vec3 &velocity = particle.Velocity;
            //静止情况不移动
            if (velocity.x == 0.0f && velocity.y == 0.0f && velocity.z == 0.0f)
                continue;

            glm::vec3 &position = particle.Position;
            position += velocity * deltaTime.GetSeconds();
        }
    }

    //提交出去渲染的
    void ParticleSystem::Render() const
    {
        for (const Particle &particle: m_Particles)
        {
            //1.不存活就接着找
            if (!particle.Active)
                continue;
            //2.不需要找没有时间了的 已经死了
            //3.计算生命进度
            const float lifeProgress =
                    1.f -
                    particle.LifeRemaining /
                    particle.LifeTime;

            //4.计算现在的颜色
            const glm::vec4 currentColor =
                    particle.ColorBegin +
                    (particle.ColorEnd - particle.ColorBegin) * lifeProgress;
            //5.计算尺寸
            const float currentSize =
                    particle.SizeBegin +
                    (particle.SizeEnd - particle.SizeBegin) *
                    lifeProgress;

            //6.提交给Renderer2D 此处不仅任何的Begin
            //只是一味的向GPU提交数据 如果Begin 和End会多次绘制
            //而且可能会在后续的使用中因为多次绘制导师失效
            Limen::Renderer2D::DrawQuad(
                particle.Position,
                glm::vec2(currentSize, currentSize),
                currentColor
            );
        }
    }
}
