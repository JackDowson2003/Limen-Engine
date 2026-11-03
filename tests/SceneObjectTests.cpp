#include <cstdint>
#include <iostream>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

#include "Limen/Scene/Scene.h"

namespace Limen
{
    // 与 Scene 中受 LIMEN_SCENE_TEST_HOOKS 保护的 friend 声明配对。
    // 只用于把槽位 Generation 推进到 uint32 上限，覆盖槽位退休边界。
    struct SceneObjectSlotTestAccess
    {
        static void SetSlotGeneration(
            Scene& scene,
            const std::uint32_t index,
            const std::uint32_t generation)
        {
            scene.m_ObjectSlots[index].Generation = generation;
        }
    };
}

namespace
{
    int g_Failures = 0;

    void Check(const char* name, const bool passed)
    {
        if (passed)
            return;

        ++g_Failures;
        std::cerr << "FAIL: " << name << '\n';
    }

    glm::mat4 Translation(const float x, const float y, const float z)
    {
        return glm::translate(glm::mat4(1.0f), glm::vec3(x, y, z));
    }

    void CheckTransform(
        const char* name,
        const glm::mat4& actual,
        const glm::mat4& expected)
    {
        for (int column = 0; column < 4; ++column)
        {
            for (int row = 0; row < 4; ++row)
            {
                if (actual[column][row] == expected[column][row])
                    continue;

                ++g_Failures;
                std::cerr << "FAIL: " << name
                          << ", column=" << column
                          << ", row=" << row
                          << ", expected=" << expected[column][row]
                          << ", actual=" << actual[column][row]
                          << '\n';
                return;
            }
        }
    }

    // 这些指针只充当 CPU 侧资源身份标记。
    // 测试不会解引用它们，也不会把它们提交给 SceneRenderer。
    Limen::Ref<Limen::Mesh> MakeMeshToken()
    {
        return Limen::Ref<Limen::Mesh>(
            reinterpret_cast<Limen::Mesh*>(std::uintptr_t{1}),
            [](Limen::Mesh*) {}
        );
    }

    Limen::Ref<Limen::Material> MakeMaterialToken()
    {
        return Limen::Ref<Limen::Material>(
            reinterpret_cast<Limen::Material*>(std::uintptr_t{2}),
            [](Limen::Material*) {}
        );
    }

    void TestObjectLifecycle()
    {
        Limen::Scene scene;

        const auto first = scene.CreateObject();
        Check("created handle is valid", first.IsValid());
        Check("created object is alive", scene.IsObjectAlive(first));

        Limen::SceneObjectID firstID{};
        Check("created object exposes stable ID",
              scene.TryGetObjectID(first, firstID));
        Check("created ID is nonzero", firstID.isValid());

        Limen::SceneObjectHandle found{};
        Check("stable ID finds current handle",
              scene.TryFindObject(firstID, found));
        Check("ID lookup returns original handle", found == first);

        glm::mat4 transform{};
        Check("new object has identity transform",
              scene.TryGetObjectTransform(first, transform));
        CheckTransform("initial transform", transform, glm::mat4(1.0f));

        const glm::mat4 moved = Translation(2.0f, 3.0f, 4.0f);
        Check("set object transform succeeds",
              scene.SetObjectTransform(first, moved));
        Check("updated transform can be read",
              scene.TryGetObjectTransform(first, transform));
        CheckTransform("updated transform", transform, moved);

        Check("destroy accepts live object", scene.DestroyObject(first));
        Check("destroyed object is not alive", !scene.IsObjectAlive(first));
        Check("destroy rejects stale handle", !scene.DestroyObject(first));

        const Limen::SceneObjectID idSentinel{7, 8};
        auto idOutput = idSentinel;
        Check("stale handle cannot query ID",
              !scene.TryGetObjectID(first, idOutput));
        Check("failed ID query preserves output", idOutput == idSentinel);

        const Limen::SceneObjectHandle handleSentinel{17, 23};
        auto handleOutput = handleSentinel;
        Check("deleted ID cannot find object",
              !scene.TryFindObject(firstID, handleOutput));
        Check("failed ID lookup preserves output", handleOutput == handleSentinel);

        auto untouched = Translation(90.0f, 91.0f, 92.0f);
        Check("stale handle cannot set transform",
              !scene.SetObjectTransform(first, Translation(8.0f, 9.0f, 10.0f)));
        Check("stale handle cannot read transform",
              !scene.TryGetObjectTransform(first, untouched));
        CheckTransform("failed transform query preserves output",
                       untouched,
                       Translation(90.0f, 91.0f, 92.0f));

        const auto reused = scene.CreateObject();
        Check("freed slot is reused", reused.Index == first.Index);
        Check("reused slot receives a new generation",
              reused.Generation != first.Generation);
        Check("reused slot is alive", scene.IsObjectAlive(reused));
        Check("old handle remains stale after slot reuse",
              !scene.IsObjectAlive(first));

        Limen::SceneObjectID reusedID{};
        Check("reused object exposes a new ID",
              scene.TryGetObjectID(reused, reusedID));
        Check("reused ID differs from deleted ID", !(reusedID == firstID));

        found = handleSentinel;
        Check("new ID finds reused handle",
              scene.TryFindObject(reusedID, found));
        Check("new ID lookup returns current handle", found == reused);

        const glm::mat4 reusedTransform = Translation(-1.0f, -2.0f, -3.0f);
        Check("new handle can update reused slot",
              scene.SetObjectTransform(reused, reusedTransform));
        Check("reused transform can be read",
              scene.TryGetObjectTransform(reused, transform));
        CheckTransform("reused slot transform", transform, reusedTransform);

        untouched = Translation(90.0f, 91.0f, 92.0f);
        Check("old handle cannot read reused slot",
              !scene.TryGetObjectTransform(first, untouched));
        CheckTransform("old handle does not affect new object",
                       untouched,
                       Translation(90.0f, 91.0f, 92.0f));

        Check("reused object can be destroyed", scene.DestroyObject(reused));
    }

    void TestIndependentObjectsAndInvalidInput()
    {
        Limen::Scene scene;

        const auto first = scene.CreateObject();
        const auto second = scene.CreateObject();
        Check("two live objects have different handles", !(first == second));

        Limen::SceneObjectID firstID{};
        Limen::SceneObjectID secondID{};
        Check("first independent ID is available",
              scene.TryGetObjectID(first, firstID));
        Check("second independent ID is available",
              scene.TryGetObjectID(second, secondID));
        Check("two live objects have different persistent IDs",
              !(firstID == secondID));

        const glm::mat4 secondTransform = Translation(4.0f, 5.0f, 6.0f);
        Check("second object accepts transform",
              scene.SetObjectTransform(second, secondTransform));
        Check("first object can be deleted independently",
              scene.DestroyObject(first));
        Check("second object survives first deletion",
              scene.IsObjectAlive(second));

        glm::mat4 actual{};
        Check("surviving transform remains accessible",
              scene.TryGetObjectTransform(second, actual));
        CheckTransform("surviving object transform", actual, secondTransform);

        const auto replacement = scene.CreateObject();
        Check("replacement does not invalidate surviving object",
              scene.IsObjectAlive(second));
        Check("surviving object can still be updated",
              scene.SetObjectTransform(second, Translation(7.0f, 8.0f, 9.0f)));

        Check("replacement can be deleted", scene.DestroyObject(replacement));
        Check("survivor can be deleted", scene.DestroyObject(second));

        const Limen::SceneObjectID idSentinel{11, 12};
        const Limen::SceneObjectHandle handleSentinel{21, 22};

        const auto checkRejectedHandle = [&](const Limen::SceneObjectHandle handle)
        {
            Check("invalid handle is not alive", !scene.IsObjectAlive(handle));

            auto idOutput = idSentinel;
            Check("invalid handle cannot query ID",
                  !scene.TryGetObjectID(handle, idOutput));
            Check("invalid ID query preserves output", idOutput == idSentinel);

            Check("invalid handle cannot set transform",
                  !scene.SetObjectTransform(handle, glm::mat4(2.0f)));

            auto output = Translation(50.0f, 60.0f, 70.0f);
            Check("invalid handle cannot read transform",
                  !scene.TryGetObjectTransform(handle, output));
            CheckTransform("invalid transform query preserves output",
                           output,
                           Translation(50.0f, 60.0f, 70.0f));

            Check("invalid handle cannot be destroyed",
                  !scene.DestroyObject(handle));
        };

        checkRejectedHandle(Limen::SceneObjectHandle{});
        checkRejectedHandle(Limen::SceneObjectHandle{0, 0});
        checkRejectedHandle(Limen::SceneObjectHandle{
            Limen::SceneObjectHandle::InvalidIndex, 1
        });
        checkRejectedHandle(Limen::SceneObjectHandle{0, 999});

        auto handleOutput = handleSentinel;
        Check("zero ID cannot find an object",
              !scene.TryFindObject(Limen::SceneObjectID{}, handleOutput));
        Check("failed zero-ID lookup preserves output",
              handleOutput == handleSentinel);
    }

    void TestRenderObjectLifecycle()
    {
        Limen::Scene scene;

        const auto mesh = MakeMeshToken();
        const auto material = MakeMaterialToken();

        Limen::SceneRenderObject source;
        source.MeshResource = mesh;
        source.MaterialResource = material;
        source.Transform = Translation(1.0f, 2.0f, 3.0f);

        const auto handle = scene.AddRenderObject(source);
        Check("render object handle is valid", handle.IsValid());
        Check("render object enters render list",
              scene.GetRenderObjects().getSize() == 1);

        const Limen::SceneRenderObject& stored =
                scene.GetRenderObjects()[handle.Index];
        Check("stored mesh is preserved", stored.MeshResource == mesh);
        Check("stored material is preserved",
              stored.MaterialResource == material);
        CheckTransform("stored render transform",
                       stored.Transform,
                       source.Transform);

        const glm::mat4 moved = Translation(4.0f, 5.0f, 6.0f);
        Check("render transform entry updates render data",
              scene.SetRenderObjectTransform(handle, moved));
        CheckTransform(
            "render transform after update",
            scene.GetRenderObjects()[handle.Index].Transform,
            moved);

        int visited = 0;
        for (const Limen::SceneRenderObject& renderObject :
             scene.GetRenderObjects())
        {
            ++visited;
            Check("render list sees original mesh",
                  renderObject.MeshResource == mesh);
            Check("render list sees original material",
                  renderObject.MaterialResource == material);
            CheckTransform("render list sees current transform",
                           renderObject.Transform,
                           moved);
        }
        Check("one renderable object is visited", visited == 1);

        // 普通场景对象与渲染对象当前是两条独立管理线：
        // 普通对象不进入渲染数组，渲染对象也不占用对象槽位。
        const auto plainObject = scene.CreateObject();
        Check("plain scene object is valid", plainObject.IsValid());
        Check("plain object does not enter render list",
              scene.GetRenderObjects().getSize() == 1);

        Check("plain object can be destroyed",
              scene.DestroyObject(plainObject));
        Check("render list survives plain object removal",
              scene.GetRenderObjects().getSize() == 1);
    }

    void TestSlotRetirementBoundary()
    {
        using Generation = std::uint32_t;
        constexpr Generation maxGeneration =
                std::numeric_limits<Generation>::max();

        Limen::Scene scene;
        const auto first = scene.CreateObject();
        Check("retirement setup object is valid", first.IsValid());

        Limen::SceneObjectID retiredID{};
        Check("retiring object exposes ID",
              scene.TryGetObjectID(first, retiredID) && retiredID.isValid());

        // 白盒：直接把槽位 Generation 推进到 uint32 上限。
        Limen::SceneObjectSlotTestAccess::SetSlotGeneration(
            scene, first.Index, maxGeneration);

        const Limen::SceneObjectHandle maxed{first.Index, maxGeneration};
        Check("max generation handle is valid", maxed.IsValid());
        Check("max generation handle still matches slot",
              scene.IsObjectAlive(maxed));

        Check("destroying max generation slot retires it",
              scene.DestroyObject(maxed));
        Check("retired slot is no longer alive",
              !scene.IsObjectAlive(maxed));
        Check("previous generation handle also fails",
              !scene.IsObjectAlive(first));

        // 退休槽位不进入空闲链：新对象必须分配到新下标。
        const auto replacement = scene.CreateObject();
        Check("replacement uses a different slot",
              replacement.Index != first.Index);
        Check("replacement is alive", scene.IsObjectAlive(replacement));

        // 退休后旧句柄与旧 ID 全部失效，且查询失败不修改 out 参数。
        const Limen::SceneObjectID idSentinel{31, 41};
        auto idOutput = idSentinel;
        Check("retired handle cannot query ID",
              !scene.TryGetObjectID(maxed, idOutput));
        Check("failed ID query preserves output", idOutput == idSentinel);

        const Limen::SceneObjectHandle handleSentinel{51, 61};
        auto handleOutput = handleSentinel;
        Check("retired ID cannot find object",
              !scene.TryFindObject(retiredID, handleOutput));
        Check("failed ID lookup preserves output",
              handleOutput == handleSentinel);

        auto untouched = Translation(70.0f, 80.0f, 90.0f);
        Check("retired handle cannot set transform",
              !scene.SetObjectTransform(
                  maxed, Translation(1.0f, 2.0f, 3.0f)));
        Check("retired handle cannot read transform",
              !scene.TryGetObjectTransform(maxed, untouched));
        CheckTransform("failed transform query preserves output",
                       untouched,
                       Translation(70.0f, 80.0f, 90.0f));

        // 空闲链只回收普通释放槽位，退休槽位永远不会再次分配。
        Check("replacement can be destroyed",
              scene.DestroyObject(replacement));
        const auto recycled = scene.CreateObject();
        Check("freed replacement slot is recycled",
              recycled.Index == replacement.Index);
        Check("recycled object never reuses retired slot",
              recycled.Index != first.Index);
        Check("recycled object is alive", scene.IsObjectAlive(recycled));
        Check("recycled object can be destroyed",
              scene.DestroyObject(recycled));
    }
}

int main()
{
    TestObjectLifecycle();
    TestIndependentObjectsAndInvalidInput();
    TestRenderObjectLifecycle();
    TestSlotRetirementBoundary();

    if (g_Failures == 0)
        std::cout
                << "Scene object identity, generation, retirement, and render views: PASS\n";

    return g_Failures == 0 ? 0 : 1;
}
