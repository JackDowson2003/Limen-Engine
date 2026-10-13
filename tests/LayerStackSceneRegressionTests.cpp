#include <cstddef>
#include <cstdint>
#include <iostream>

#include "Limen/Application/LayerStack.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Tool/LStaticArray.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    struct LayerEvents
    {
        Limen::LStaticArray<int, 7> Detached{};
        Limen::LStaticArray<int, 7> Destroyed{};
    };

    class ProbeLayer final : public Limen::Layer
    {
    public:
        ProbeLayer(int id, LayerEvents &events)
            : Limen::Layer("probe"), m_Id(id), m_Events(events)
        {
        }

        ~ProbeLayer() override
        {
            ++m_Events.Destroyed[static_cast<std::size_t>(m_Id)];
        }

        void OnDetach() override
        {
            ++m_Events.Detached[static_cast<std::size_t>(m_Id)];
        }

    private:
        int m_Id;
        LayerEvents &m_Events;
    };

    template<std::size_t N>
    bool MatchesOrder(
        const Limen::LayerStack &stack,
        const Limen::LStaticArray<const Limen::Layer *, N> &expected
    )
    {
        std::size_t index = 0;
        for (const Limen::Layer *layer : stack)
        {
            if (index >= N || layer != expected[index])
                return false;
            ++index;
        }
        if (index != N)
            return false;

        // 正反向遍历都应覆盖相同的 Layer，且顺序相反。
        for (auto it = stack.rbegin(); it != stack.rend(); ++it)
        {
            if (index == 0 || *it != expected[--index])
                return false;
        }
        return index == 0;
    }

    bool Contains(const Limen::LayerStack &stack, const Limen::Layer *wanted)
    {
        for (const Limen::Layer *layer : stack)
        {
            if (layer == wanted)
                return true;
        }
        return false;
    }

    bool TestLayerStackMigration()
    {
        bool passed = true;
        LayerEvents events;
        {
            Limen::LayerStack stack;
            passed &= Check("empty stack iteration", MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 0>{}));

            auto *baseA = new ProbeLayer(1, events);
            auto *baseB = new ProbeLayer(2, events);
            auto *overlayA = new ProbeLayer(3, events);
            auto *overlayB = new ProbeLayer(4, events);

            // 交错插入可检验普通 Layer 始终在 Overlay 前面。
            stack.PushOverlay(overlayA);
            stack.PushLayer(baseA);
            stack.PushOverlay(overlayB);
            stack.PushLayer(baseB);
            passed &= Check("layers precede overlays after interleaved pushes",
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 4>{
                                baseA, baseB, overlayA, overlayB
                            }));

            stack.PopLayer(overlayA);
            stack.PopOverlay(baseA);
            passed &= Check("popping from the wrong group changes nothing",
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 4>{
                                baseA, baseB, overlayA, overlayB
                            }) && events.Detached[1] == 0 && events.Detached[3] == 0);

            stack.PopLayer(baseA);
            const bool baseRemoved = !Contains(stack, baseA);
            passed &= Check("pop layer detaches but does not destroy",
                            baseRemoved && events.Detached[1] == 1 && events.Destroyed[1] == 0 &&
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 3>{
                                baseB, overlayA, overlayB
                            }));
            // Pop 后所有权交还调用者；若实现意外未移除，留给 Clear 销毁以避免双重释放。
            if (baseRemoved && events.Destroyed[1] == 0)
                delete baseA;

            stack.PopOverlay(overlayA);
            const bool overlayRemoved = !Contains(stack, overlayA);
            passed &= Check("pop overlay detaches but does not destroy",
                            overlayRemoved && events.Detached[3] == 1 && events.Destroyed[3] == 0 &&
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 2>{
                                baseB, overlayB
                            }));
            if (overlayRemoved && events.Destroyed[3] == 0)
                delete overlayA;

            stack.Clear();
            passed &= Check("clear detaches and destroys remaining layers",
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 0>{}) &&
                            events.Detached[2] == 1 && events.Detached[4] == 1 &&
                            events.Destroyed[2] == 1 && events.Destroyed[4] == 1);
            stack.Clear();
            passed &= Check("clear is idempotent",
                            events.Detached[2] == 1 && events.Detached[4] == 1 &&
                            events.Destroyed[2] == 1 && events.Destroyed[4] == 1);

            auto *overlayC = new ProbeLayer(5, events);
            auto *baseC = new ProbeLayer(6, events);
            stack.PushOverlay(overlayC);
            stack.PushLayer(baseC);
            passed &= Check("clear resets layer insertion boundary",
                            MatchesOrder(stack, Limen::LStaticArray<const Limen::Layer *, 2>{
                                baseC, overlayC
                            }));
        }

        // 析构函数还会 Clear；每个仍在栈内的 Layer 应恰好 detach、销毁一次。
        for (std::size_t id = 1; id < events.Detached.size(); ++id)
        {
            passed &= Check("each layer detached exactly once", events.Detached[id] == 1);
            passed &= Check("each layer destroyed exactly once", events.Destroyed[id] == 1);
        }
        return passed;
    }

    bool SamePointLight(const Limen::PointLight &lhs, const Limen::PointLight &rhs)
    {
        return lhs.Position.x == rhs.Position.x &&
               lhs.Position.y == rhs.Position.y &&
               lhs.Position.z == rhs.Position.z &&
               lhs.Color.x == rhs.Color.x &&
               lhs.Color.y == rhs.Color.y &&
               lhs.Color.z == rhs.Color.z &&
               lhs.Intensity == rhs.Intensity;
    }

    bool TestScenePointLightMigration()
    {
        bool passed = true;
        Limen::Scene scene;
        const auto &lights = scene.GetPointLights();
        passed &= Check("new scene has no point lights", lights.getSize() == 0);

        Limen::LStaticArray<Limen::ScenePointLightHandle, 64> handles{};
        for (std::size_t i = 0; i < handles.size(); ++i)
        {
            Limen::PointLight light;
            light.Position = {static_cast<float>(i), 2.0f, -3.0f};
            light.Color = {0.25f, 0.5f, 0.75f};
            light.Intensity = static_cast<float>(i + 1);
            handles[i] = scene.AddPointLight(light);
            passed &= Check("added point light has stable index",
                            handles[i].IsValid() && handles[i].Index == i);
        }
        passed &= Check("point light view observes all additions",
                        &lights == &scene.GetPointLights() && lights.getSize() == handles.size());

        // 多次扩容后仍通过下标句柄访问，而不依赖可能已失效的元素指针。
        for (std::size_t i = 0; i < handles.size(); ++i)
        {
            Limen::PointLight actual;
            const bool found = scene.TryGetPointLight(handles[i], actual);
            passed &= Check("old point light handle survives growth",
                            found && actual.Position.x == static_cast<float>(i) &&
                            actual.Intensity == static_cast<float>(i + 1));
        }

        Limen::PointLight changed;
        changed.Position = {9.0f, 8.0f, 7.0f};
        changed.Color = {0.5f, 0.25f, 0.125f};
        changed.Intensity = 123.0f;
        passed &= Check("set point light succeeds", scene.SetPointLight(handles[0], changed));
        Limen::PointLight actual;
        passed &= Check("set point light updates selected element",
                        scene.TryGetPointLight(handles[0], actual) && SamePointLight(actual, changed) &&
                        SamePointLight(lights[0], changed) && lights[1].Intensity == 2.0f);

        actual.Intensity = 999.0f;
        passed &= Check("returned point light is a copy", lights[0].Intensity == 123.0f);

        Limen::PointLight untouched;
        untouched.Intensity = 777.0f;
        const Limen::ScenePointLightHandle invalid;
        passed &= Check("default invalid handle leaves output untouched",
                        !scene.TryGetPointLight(invalid, untouched) && untouched.Intensity == 777.0f);
        const Limen::ScenePointLightHandle outOfRange{lights.getSize()};
        passed &= Check("out-of-range handle leaves output untouched",
                        !scene.TryGetPointLight(outOfRange, untouched) && untouched.Intensity == 777.0f);
        return passed;
    }
}

int main()
{
    const bool layersPassed = TestLayerStackMigration();
    const bool lightsPassed = TestScenePointLightMigration();
    return layersPassed && lightsPassed ? 0 : 1;
}
