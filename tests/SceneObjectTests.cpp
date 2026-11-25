#include <cstdint>
#include <cmath>
#include <iostream>
#include <limits>
#include <variant>

#include <glm/gtc/matrix_transform.hpp>

#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneDocument.h"

namespace Limen
{
    // 与 Scene 中受 LIMEN_SCENE_TEST_HOOKS 保护的 friend 声明配对。
    // 覆盖公开接口无法直接观察的槽位退休和空闲链状态。
    struct SceneObjectSlotTestAccess
    {
        static void SetSlotGeneration(
            Scene& scene,
            const std::uint32_t index,
            const std::uint32_t generation)
        {
            scene.m_ObjectSlots[index].Generation = generation;
        }

        static std::uint32_t GetNextFree(const Scene& scene, const std::uint32_t index)
        {
            return scene.m_ObjectSlots[index].NextFree;
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

    Limen::SceneTransform LocationTransform(
        const float x,
        const float y,
        const float z)
    {
        Limen::SceneTransform transform;
        transform.Location = {x, y, z};
        return transform;
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

    void CheckAuthoringTransform(
        const char* name,
        const Limen::SceneTransform& actual,
        const Limen::SceneTransform& expected)
    {
        const auto sameVec3 = [](const glm::vec3& a, const glm::vec3& b)
        {
            return std::abs(a.x - b.x) <= 1.0e-5f &&
                   std::abs(a.y - b.y) <= 1.0e-5f &&
                   std::abs(a.z - b.z) <= 1.0e-5f;
        };
        const bool sameRotation =
            std::abs(actual.Rotation.x - expected.Rotation.x) <= 1.0e-5f &&
            std::abs(actual.Rotation.y - expected.Rotation.y) <= 1.0e-5f &&
            std::abs(actual.Rotation.z - expected.Rotation.z) <= 1.0e-5f &&
            std::abs(actual.Rotation.w - expected.Rotation.w) <= 1.0e-5f;

        if (sameVec3(actual.Location, expected.Location) &&
            sameRotation &&
            sameVec3(actual.Scale, expected.Scale))
            return;

        ++g_Failures;
        std::cerr << "FAIL: " << name << '\n';
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

        const Limen::SceneTransform movedTransform =
            LocationTransform(2.0f, 3.0f, 4.0f);
        const glm::mat4 moved = movedTransform.ToMatrix();
        Check("set object transform succeeds",
              scene.SetObjectTransform(first, movedTransform));
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
              !scene.SetObjectTransform(
                  first,
                  LocationTransform(8.0f, 9.0f, 10.0f)));
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

        const Limen::SceneTransform reusedAuthoring =
            LocationTransform(-1.0f, -2.0f, -3.0f);
        const glm::mat4 reusedTransform = reusedAuthoring.ToMatrix();
        Check("new handle can update reused slot",
              scene.SetObjectTransform(reused, reusedAuthoring));
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

        const Limen::SceneTransform secondAuthoring =
            LocationTransform(4.0f, 5.0f, 6.0f);
        const glm::mat4 secondTransform = secondAuthoring.ToMatrix();
        Check("second object accepts transform",
              scene.SetObjectTransform(second, secondAuthoring));
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
              scene.SetObjectTransform(
                  second,
                  LocationTransform(7.0f, 8.0f, 9.0f)));

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
                  !scene.SetObjectTransform(handle, Limen::SceneTransform{}));

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
                  maxed,
                  LocationTransform(1.0f, 2.0f, 3.0f)));
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

    void TestForEachObject()
    {
        Limen::Scene scene;

        int emptyVisits = 0;
        scene.ForEachObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &)
            {
                ++emptyVisits;
            }
        );
        Check("empty scene yields no objects", emptyVisits == 0);

        const auto first = scene.CreateObject();
        const auto second = scene.CreateObject();
        const auto third = scene.CreateObject();
        Limen::SceneObjectID deletedID{};
        Check("capture deleted object ID before destroy",
              scene.TryGetObjectID(second, deletedID));
        Check("destroy middle object", scene.DestroyObject(second));

        Limen::LArray<Limen::SceneObjectID> visitedIDs;
        int visited = 0;
        scene.ForEachObject(
            [&](const Limen::SceneObjectHandle &handle,
                const Limen::SceneObjectID &id)
            {
                ++visited;
                visitedIDs.push_back(id);
                Check("visited handle is alive", scene.IsObjectAlive(handle));

                Limen::SceneObjectID viaHandle{};
                Check("visited ID matches handle",
                      scene.TryGetObjectID(handle, viaHandle) &&
                      viaHandle == id);
            }
        );
        Check("two surviving objects are visited", visited == 2);

        Limen::SceneObjectID firstID{};
        Limen::SceneObjectID thirdID{};
        const bool firstKnown = scene.TryGetObjectID(first, firstID);
        const bool thirdKnown = scene.TryGetObjectID(third, thirdID);
        bool sawFirst = false;
        bool sawThird = false;
        for (const Limen::SceneObjectID &id : visitedIDs)
        {
            if (firstKnown && id == firstID)
                sawFirst = true;
            if (thirdKnown && id == thirdID)
                sawThird = true;
        }
        Check("first survivor is visited", sawFirst);
        Check("third survivor is visited", sawThird);

        bool sawDeleted = false;
        for (const Limen::SceneObjectID &id : visitedIDs)
        {
            if (id == deletedID)
                sawDeleted = true;
        }
        Check("deleted object is not visited", !sawDeleted);
    }

    void TestSceneRenderableComponent()
    {
        Limen::Scene scene;
        const auto mesh = MakeMeshToken();
        const auto material = MakeMaterialToken();

        Limen::SceneRenderObject source;
        source.MeshResource = mesh;
        source.MaterialResource = material;
        source.Transform = Translation(1.0f, 2.0f, 0.0f);
        auto objectTransform = LocationTransform(10.0f, 0.0f, 0.0f);
        objectTransform.Rotation = glm::quat{0.0f, 0.0f, 0.0f, 1.0f};
        objectTransform.Scale = {2.0f, 3.0f, 4.0f};

        const auto handle = scene.AddRenderableObject(source, objectTransform);
        Check("renderable object handle is valid", handle.IsValid());
        Check("renderable object is alive", scene.IsObjectAlive(handle));

        Limen::SceneObjectID objectID{};
        Check("renderable object exposes stable ID",
              scene.TryGetObjectID(handle, objectID) && objectID.isValid());

        int objectVisits = 0;
        scene.ForEachObject(
            [&](const Limen::SceneObjectHandle &visitedHandle,
                const Limen::SceneObjectID &visitedID)
            {
                if (!(visitedHandle == handle))
                    return;
                ++objectVisits;
                Check("renderable object ID matches traversal",
                      visitedID == objectID);
            });
        Check("renderable object appears in object traversal",
              objectVisits == 1);

        int snapshotVisits = 0;
        scene.ForEachObjectAuthoring([&](const Limen::SceneObjectAuthoringData &snapshot)
        {
            ++snapshotVisits;
            Check("renderable snapshot preserves object ID", snapshot.ID == objectID);
            CheckAuthoringTransform("renderable snapshot preserves root TRS",
                                    snapshot.Transform, objectTransform);
        });
        Check("renderable creates one authoring snapshot", snapshotVisits == 1);

        int renderVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &visitedHandle,
                const Limen::SceneObjectID &visitedID,
                const Limen::Ref<Limen::Mesh> &meshResource,
                const Limen::Ref<Limen::Material> &materialResource,
                const glm::mat4 &transform)
            {
                ++renderVisits;
                Check("renderable traversal handle matches",
                      visitedHandle == handle);
                Check("renderable traversal ID matches", visitedID == objectID);
                Check("renderable traversal mesh matches", meshResource == mesh);
                Check("renderable traversal material matches",
                      materialResource == material);
                CheckTransform("renderable traversal transform",
                               transform, objectTransform.ToMatrix() * source.Transform);
                // 局部偏移 (1, 2) 先缩放为 (2, 6)，旋转 180° 后再叠加根位移。
                Check("component offset is composed in object space",
                      glm::vec3(transform[3]) == glm::vec3{8.0f, -6.0f, 0.0f});
            });
        Check("one renderable object is visited", renderVisits == 1);

        const Limen::SceneTransform movedAuthoring =
            LocationTransform(4.0f, 5.0f, 6.0f);
        const glm::mat4 moved = movedAuthoring.ToMatrix();
        Check("renderable transform follows object transform",
              scene.SetObjectTransform(handle, movedAuthoring));

        renderVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &transform)
            {
                ++renderVisits;
                CheckTransform("render traversal sees updated transform",
                               transform, moved * source.Transform);
            });
        Check("updated renderable is visited once", renderVisits == 1);

        source.MeshResource.reset();
        source.MaterialResource.reset();
        Check("mesh is shared while renderable is alive",
              mesh.use_count() == 2);
        Check("material is shared while renderable is alive",
              material.use_count() == 2);

        Check("renderable object can be destroyed",
              scene.DestroyObject(handle));
        Check("mesh reference released after destroy", mesh.use_count() == 1);
        Check("material reference released after destroy",
              material.use_count() == 1);

        renderVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &)
            {
                ++renderVisits;
            });
        Check("destroyed renderable is not visited", renderVisits == 0);

        // 复用槽位不得残留上一代渲染组件。
        const auto again = scene.AddRenderableObject(
            Limen::SceneRenderObject{mesh, material}, {});
        Check("replacement renderable is valid", again.IsValid());
        Check("replacement renderable reuses slot",
              again.Index == handle.Index);
        Check("destroy replacement renderable", scene.DestroyObject(again));

        const auto plain = scene.CreateObject();
        Check("plain object reuses renderable slot",
              plain.Index == handle.Index);
        renderVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &)
            {
                ++renderVisits;
            });
        Check("reused slot has no render component", renderVisits == 0);
    }

    void TestGroupedRenderableComponents()
    {
        Limen::Scene scene;
        const auto mesh = MakeMeshToken();
        const auto material = MakeMaterialToken();

        Limen::LArray<Limen::SceneRenderObject> parts;
        parts.push_back(Limen::SceneRenderObject{
            mesh, material, Translation(1.0f, 0.0f, 0.0f)});
        parts.push_back(Limen::SceneRenderObject{
            mesh, material, Translation(0.0f, 2.0f, 0.0f)});

        const Limen::SceneTransform objectTransform = LocationTransform(3.0f, -1.0f, 0.0f);
        auto invalidTransform = objectTransform;
        invalidTransform.Location.x = std::numeric_limits<float>::infinity();
        Limen::LArray<Limen::SceneRenderObject> invalidParts(parts);
        invalidParts[1].MaterialResource.reset();
        Check("single renderable rejects non-finite root TRS",
              !scene.AddRenderableObject(parts[0], invalidTransform).IsValid());
        Check("single renderable rejects missing resources",
              !scene.AddRenderableObject(invalidParts[1], objectTransform).IsValid());
        Check("renderable group rejects empty parts",
              !scene.AddRenderableObjectGroup({}, objectTransform).IsValid());
        Check("renderable group rejects an invalid second part",
              !scene.AddRenderableObjectGroup(invalidParts, objectTransform).IsValid());
        Check("renderable group rejects non-finite root TRS",
              !scene.AddRenderableObjectGroup(parts, invalidTransform).IsValid());

        const auto handle = scene.AddRenderableObjectGroup(parts, objectTransform);
        Check("grouped renderable object is valid", handle.IsValid());
        Limen::SceneTransform storedTransform;
        Check("grouped root TRS is available",
              scene.TryGetObjectTransform(handle, storedTransform));
        CheckAuthoringTransform("grouped root TRS is preserved", storedTransform, objectTransform);

        int objectVisits = 0;
        scene.ForEachObject(
            [&](const Limen::SceneObjectHandle &visitedHandle,
                const Limen::SceneObjectID &)
            {
                ++objectVisits;
                Check("group traversal sees only the created handle", visitedHandle == handle);
            });
        Check("group is one scene object", objectVisits == 1);

        int componentVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &visitedHandle,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &transform)
            {
                if (!(visitedHandle == handle))
                    return;
                if (componentVisits == 0)
                    CheckTransform("first component world transform after creation",
                                   transform,
                                   Translation(4.0f, -1.0f, 0.0f));
                else if (componentVisits == 1)
                    CheckTransform("second component world transform after creation",
                                   transform,
                                   Translation(3.0f, 1.0f, 0.0f));
                ++componentVisits;
            });
        Check("group exposes two render components", componentVisits == 2);

        Check("group object transform moves all components",
              scene.SetObjectTransform(
                  handle,
                  LocationTransform(10.0f, 0.0f, 0.0f)));

        componentVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &visitedHandle,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &transform)
            {
                if (!(visitedHandle == handle))
                    return;
                if (componentVisits == 0)
                    CheckTransform("first component world transform",
                                   transform,
                                   Translation(11.0f, 0.0f, 0.0f));
                else if (componentVisits == 1)
                    CheckTransform("second component world transform",
                                   transform,
                                   Translation(10.0f, 2.0f, 0.0f));
                ++componentVisits;
            });
        Check("group still exposes two components after move",
              componentVisits == 2);

        Check("destroy grouped renderable", scene.DestroyObject(handle));
        componentVisits = 0;
        scene.ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &)
            {
                ++componentVisits;
            });
        Check("destroy clears grouped components", componentVisits == 0);
    }

    void TestObjectLabels()
    {
        Limen::Scene scene;
        const auto first = scene.CreateObject();
        const auto second = scene.CreateObject();

        Limen::LString label("sentinel");
        Check("new object label can be read",
              scene.TryGetObjectLabel(first, label));
        Check("new object starts with an empty label", label.empty());

        Limen::SceneObjectID idBeforeLabel{};
        Check("capture object ID before label edit",
              scene.TryGetObjectID(first, idBeforeLabel));
        Check("set object label succeeds",
              scene.SetObjectLabel(first, "TestCube"));
        Check("set label can be read",
              scene.TryGetObjectLabel(first, label) &&
              label == "TestCube");

        Limen::SceneObjectID idAfterLabel{};
        Check("label edit preserves object ID",
              scene.TryGetObjectID(first, idAfterLabel) &&
              idAfterLabel == idBeforeLabel);

        Check("overwrite object label succeeds",
              scene.SetObjectLabel(first, "场景对象"));
        Check("overwritten label can be read",
              scene.TryGetObjectLabel(first, label) &&
              label == "场景对象");

        Check("empty label clears display name",
              scene.SetObjectLabel(first, ""));
        Check("cleared label can be read",
              scene.TryGetObjectLabel(first, label) &&
              label.empty());

        Check("labels are independent per object",
              scene.SetObjectLabel(second, "SecondObject"));
        Check("second object label can be read",
              scene.TryGetObjectLabel(second, label) &&
              label == "SecondObject");

        Check("destroy labeled object", scene.DestroyObject(first));
        Limen::LString untouched("keep");
        Check("stale handle cannot read label",
              !scene.TryGetObjectLabel(first, untouched));
        Check("failed label query preserves output",
              untouched == "keep");
        Check("stale handle cannot set label",
              !scene.SetObjectLabel(first, "stale"));

        const auto reused = scene.CreateObject();
        Check("label test reuses freed slot",
              reused.Index == first.Index);
        Check("reused slot label can be read",
              scene.TryGetObjectLabel(reused, label));
        Check("reused slot does not retain old label", label.empty());

        Check("surviving object label is unchanged",
              scene.TryGetObjectLabel(second, label) &&
              label == "SecondObject");
    }

    void TestAuthoringTransform()
    {
        Limen::Scene scene;
        const auto handle = scene.CreateObject();

        Limen::SceneTransform actual;
        Check("new object exposes identity authoring TRS",
              scene.TryGetObjectAuthoringTransform(handle, actual));
        CheckAuthoringTransform(
            "new object authoring TRS",
            actual,
            Limen::SceneTransform{});

        Limen::SceneTransform authoring;
        authoring.Location = {1.0f, 2.0f, 3.0f};
        authoring.Rotation = glm::angleAxis(
            glm::radians(45.0f),
            glm::vec3{0.0f, 1.0f, 0.0f});
        authoring.Scale = {2.0f, 3.0f, 4.0f};
        Check("authoring TRS can be set",
              scene.SetObjectAuthoringTransform(handle, authoring));
        Check("authoring TRS can be read",
              scene.TryGetObjectAuthoringTransform(handle, actual));
        CheckAuthoringTransform(
            "authoring TRS matches",
            actual,
            authoring);

        // 新实现把 Authoring TRS 同步到运行期矩阵缓存。
        glm::mat4 matrix{};
        Check("authoring TRS updates matrix cache",
              scene.TryGetObjectTransform(handle, matrix));
        CheckTransform(
            "authoring TRS matrix cache",
            matrix,
            authoring.ToMatrix());

        Limen::SceneTransform untouched = authoring;
        untouched.Location.x = 99.0f;
        Check("NaN authoring TRS is rejected",
              !scene.SetObjectAuthoringTransform(
                  handle,
                  []()
                  {
                      Limen::SceneTransform invalid;
                      invalid.Location.x =
                          std::numeric_limits<float>::quiet_NaN();
                      return invalid;
                  }()));
        Check("rejected authoring TRS preserves previous value",
              scene.TryGetObjectAuthoringTransform(handle, untouched));
        CheckAuthoringTransform(
            "rejected authoring TRS keeps previous value",
            untouched,
            authoring);

        const Limen::SceneObjectHandle stale{999, 999};
        Check("stale handle cannot set authoring TRS",
              !scene.SetObjectAuthoringTransform(stale, Limen::SceneTransform{}));
        Check("stale handle cannot read authoring TRS",
              !scene.TryGetObjectAuthoringTransform(stale, untouched));
        CheckAuthoringTransform(
            "failed authoring query preserves output",
            untouched,
            authoring);

        Check("authoring transform object can be destroyed",
              scene.DestroyObject(handle));
        const auto reused = scene.CreateObject();
        Check("authoring transform slot is reused",
              reused.Index == handle.Index);
        Check("reused object exposes default authoring TRS",
              scene.TryGetObjectAuthoringTransform(reused, actual));
        CheckAuthoringTransform(
            "reused object authoring TRS is reset",
            actual,
            Limen::SceneTransform{});
    }

    void TestSceneObjectComponents()
    {
        Limen::Scene scene;
        const auto handle = scene.CreateObject();

        int visits = 0;
        scene.ForEachObjectComponent(
            handle,
            [&](const Limen::SceneObjectComponent &)
            {
                ++visits;
            });
        Check("new object has no authoring components", visits == 0);

        Limen::SceneObjectComponent model;
        model.Data = Limen::ModelRendererComponent{
            .ModelAssetID = {
                0x0011223344556677ULL,
                0x8899aabbccddeeffULL
            }
        };
        const Limen::SceneComponentID modelID =
            scene.AddObjectComponent(handle, model);
        Check("model component receives generated ID", modelID.isValid());

        Limen::SceneObjectComponent primitive;
        primitive.Data = Limen::PrimitiveRendererComponent{
            .Primitive = Limen::PrimitiveType::Cube
        };
        const Limen::SceneComponentID primitiveID =
            scene.AddObjectComponent(handle, primitive);
        Check("primitive component receives generated ID",
              primitiveID.isValid() && !(primitiveID == modelID));

        Limen::SceneObjectComponent light;
        light.LocalTransform.Location = {1.0f, 2.0f, 3.0f};
        light.Data = Limen::PointLightComponent{
            .Color = {1.0f, 0.5f, 0.25f},
            .Intensity = 4.0f
        };
        const Limen::SceneComponentID lightID =
            scene.AddObjectComponent(handle, light);
        Check("point light component receives generated ID",
              lightID.isValid() &&
              !(lightID == modelID) &&
              !(lightID == primitiveID));

        int modelVisits = 0;
        int primitiveVisits = 0;
        int lightVisits = 0;
        scene.ForEachObjectComponent(
            handle,
            [&](const Limen::SceneObjectComponent &component)
            {
                switch (Limen::GetSceneComponentKind(component))
                {
                    case Limen::SceneComponentKind::ModelRenderer:
                        ++modelVisits;
                        break;
                    case Limen::SceneComponentKind::PrimitiveRenderer:
                        ++primitiveVisits;
                        break;
                    case Limen::SceneComponentKind::PointLight:
                        ++lightVisits;
                        break;
                }
            });
        Check("all three authoring component kinds are stored",
              modelVisits == 1 &&
              primitiveVisits == 1 &&
              lightVisits == 1);

        Limen::SceneObjectComponent duplicate;
        duplicate.ID = modelID;
        duplicate.Data = Limen::ModelRendererComponent{
            .ModelAssetID = {
                0x1111222233334444ULL,
                0x5555666677778888ULL
            }
        };
        Check("duplicate component ID is rejected",
              !scene.AddObjectComponent(handle, duplicate).isValid());

        Limen::SceneObjectComponent invalidModel;
        invalidModel.Data = Limen::ModelRendererComponent{};
        Check("model component requires valid AssetID",
              !scene.AddObjectComponent(handle, invalidModel).isValid());

        Limen::SceneObjectComponent invalidPrimitive;
        invalidPrimitive.Data = Limen::PrimitiveRendererComponent{
            .Primitive =
                static_cast<Limen::PrimitiveType>(255)
        };
        Check("invalid primitive type is rejected",
              !scene.AddObjectComponent(handle, invalidPrimitive).isValid());

        Limen::SceneObjectComponent invalidLight;
        invalidLight.Data = Limen::PointLightComponent{
            .Color = {1.0f, 0.0f, 0.0f},
            .Intensity = -1.0f
        };
        Check("negative light intensity is rejected",
              !scene.AddObjectComponent(handle, invalidLight).isValid());

        invalidLight.Data = Limen::PointLightComponent{
            .Color = {
                std::numeric_limits<float>::quiet_NaN(),
                0.0f,
                0.0f
            },
            .Intensity = 1.0f
        };
        Check("non-finite light color is rejected",
              !scene.AddObjectComponent(handle, invalidLight).isValid());

        Limen::SceneObjectComponent invalidTransform;
        invalidTransform.LocalTransform.Location.x =
            std::numeric_limits<float>::quiet_NaN();
        invalidTransform.Data = Limen::PointLightComponent{};
        Check("non-finite component transform is rejected",
              !scene.AddObjectComponent(handle, invalidTransform).isValid());

        visits = 0;
        scene.ForEachObjectComponent(
            handle,
            [&](const Limen::SceneObjectComponent &)
            {
                ++visits;
            });
        Check("failed component additions preserve stored components",
              visits == 3);

        Check("invalid handle cannot add component",
              !scene.AddObjectComponent(
                  Limen::SceneObjectHandle{},
                  primitive).isValid());
        Check("out-of-range handle cannot add component",
              !scene.AddObjectComponent(
                  Limen::SceneObjectHandle{999, 1},
                  primitive).isValid());

        Check("component container object can be destroyed",
              scene.DestroyObject(handle));
        visits = 0;
        scene.ForEachObjectComponent(
            handle,
            [&](const Limen::SceneObjectComponent &)
            {
                ++visits;
            });
        Check("destroy removes component traversal", visits == 0);
        Check("stale handle cannot add component",
              !scene.AddObjectComponent(handle, primitive).isValid());

        const auto reused = scene.CreateObject();
        Check("component slot is reused",
              reused.Index == handle.Index);
        visits = 0;
        scene.ForEachObjectComponent(
            reused,
            [&](const Limen::SceneObjectComponent &)
            {
                ++visits;
            });
        Check("reused slot has no stale components", visits == 0);
    }

    void TestObjectAuthoringSnapshot()
    {
        Limen::Scene scene;
        const Limen::SceneObjectHandle first = scene.CreateObject();
        const Limen::SceneObjectHandle second = scene.CreateObject();
        const Limen::SceneObjectHandle third = scene.CreateObject();

        Limen::SceneObjectID firstID{};
        Limen::SceneObjectID secondID{};
        Limen::SceneObjectID thirdID{};
        Check("snapshot first ID is available",
              scene.TryGetObjectID(first, firstID));
        Check("snapshot second ID is available",
              scene.TryGetObjectID(second, secondID));
        Check("snapshot third ID is available",
              scene.TryGetObjectID(third, thirdID));

        Check("snapshot first label is set",
              scene.SetObjectLabel(first, "First"));
        Check("snapshot second label is set",
              scene.SetObjectLabel(second, "Second"));
        Check("snapshot third label is set",
              scene.SetObjectLabel(third, "Third"));

        const Limen::SceneTransform firstTransform =
            LocationTransform(1.0f, 2.0f, 3.0f);
        const Limen::SceneTransform thirdTransform =
            LocationTransform(-3.0f, -2.0f, -1.0f);
        Check("snapshot first transform is set",
              scene.SetObjectAuthoringTransform(first, firstTransform));
        Check("snapshot third transform is set",
              scene.SetObjectAuthoringTransform(third, thirdTransform));

        Limen::SceneObjectComponent primitive;
        primitive.ID = {0x1111222233334444ULL, 0x5555666677778888ULL};
        primitive.LocalTransform.Location = {0.25f, 0.5f, 0.75f};
        primitive.Data = Limen::PrimitiveRendererComponent{
            .Primitive = Limen::PrimitiveType::Cube
        };
        Check("snapshot first component is added",
              scene.AddObjectComponent(first, primitive).isValid());

        Limen::SceneObjectComponent light;
        light.ID = {0x9999aaaabbbbccccULL, 0xddddeeeeffff0000ULL};
        light.LocalTransform.Location = {0.0f, 1.0f, 0.0f};
        light.Data = Limen::PointLightComponent{
            .Color = {1.0f, 0.5f, 0.25f},
            .Intensity = 4.0f
        };
        Check("snapshot third component is added",
              scene.AddObjectComponent(third, light).isValid());

        int snapshotVisits = 0;
        bool sawFirst = false;
        bool sawSecond = false;
        bool sawThird = false;
        scene.ForEachObjectAuthoring(
            [&](const Limen::SceneObjectAuthoringData &snapshot)
            {
                ++snapshotVisits;
                if (snapshot.ID == firstID)
                {
                    sawFirst = true;
                    Check("snapshot first label matches",
                          snapshot.Label == "First");
                    CheckAuthoringTransform(
                        "snapshot first transform matches",
                        snapshot.Transform,
                        firstTransform);
                    Check("snapshot first has one component",
                          snapshot.Components.getSize() == 1);
                    if (snapshot.Components.getSize() == 1)
                    {
                        const Limen::SceneObjectComponent &component =
                            snapshot.Components[0];
                        Check("snapshot first component ID matches",
                              component.ID == primitive.ID);
                        Check("snapshot first component kind matches",
                              Limen::GetSceneComponentKind(component) ==
                              Limen::SceneComponentKind::PrimitiveRenderer);
                        CheckAuthoringTransform(
                            "snapshot first component local transform matches",
                            component.LocalTransform,
                            primitive.LocalTransform);
                    }

                    // 快照持有独立副本；修改副本不能影响 Scene。
                    Limen::SceneObjectAuthoringData copy = snapshot;
                    copy.Label = "Mutated";
                    copy.Components.clear();
                }
                else if (snapshot.ID == secondID)
                {
                    sawSecond = true;
                    Check("snapshot second label matches",
                          snapshot.Label == "Second");
                    Check("snapshot second has no components",
                          snapshot.Components.getSize() == 0);
                }
                else if (snapshot.ID == thirdID)
                {
                    sawThird = true;
                    Check("snapshot third label matches",
                          snapshot.Label == "Third");
                    CheckAuthoringTransform(
                        "snapshot third transform matches",
                        snapshot.Transform,
                        thirdTransform);
                    Check("snapshot third has one component",
                          snapshot.Components.getSize() == 1);
                    if (snapshot.Components.getSize() == 1)
                    {
                        const auto *pointLight =
                            std::get_if<Limen::PointLightComponent>(
                                &snapshot.Components[0].Data);
                        Check("snapshot third component kind matches",
                              pointLight != nullptr);
                        if (pointLight)
                        {
                            Check(
                                "snapshot third light data matches",
                                pointLight->Color ==
                                    glm::vec3{1.0f, 0.5f, 0.25f} &&
                                std::abs(pointLight->Intensity - 4.0f) <=
                                    1.0e-5f);
                        }
                    }
                }
            });
        Check("authoring traversal visits every live object",
              snapshotVisits == 3 &&
              sawFirst &&
              sawSecond &&
              sawThird);

        Limen::LString firstLabel;
        Check("mutating snapshot copy preserves Scene label",
              scene.TryGetObjectLabel(first, firstLabel) &&
              firstLabel == "First");
        int storedComponents = 0;
        scene.ForEachObjectComponent(
            first,
            [&](const Limen::SceneObjectComponent &)
            {
                ++storedComponents;
            });
        Check("mutating snapshot copy preserves Scene components",
              storedComponents == 1);

        Check("snapshot middle object can be destroyed",
              scene.DestroyObject(second));

        Limen::LArray<Limen::SceneObjectID> survivors;
        scene.ForEachObjectAuthoring(
            [&](const Limen::SceneObjectAuthoringData &snapshot)
            {
                survivors.push_back(snapshot.ID);
            });
        Check("destroyed object is absent from authoring traversal",
              survivors.getSize() == 2);
        bool survivorFirst = false;
        bool survivorThird = false;
        for (const Limen::SceneObjectID id: survivors)
        {
            survivorFirst |= id == firstID;
            survivorThird |= id == thirdID;
            Check("destroyed ID is not traversed", !(id == secondID));
        }
        Check("authoring traversal continues after dead slot",
              survivorFirst && survivorThird);
    }

    void TestObjectAuthoringRestore()
    {
        Limen::Scene source;
        auto transform = LocationTransform(3.0f, -2.0f, 1.0f);
        transform.Rotation = glm::quat{0.0f, 0.0f, 0.0f, 1.0f};
        transform.Scale = {2.0f, 3.0f, 4.0f};
        const auto original = source.AddRenderableObject({
            .MeshResource = MakeMeshToken(),
            .MaterialResource = MakeMaterialToken()
        }, transform);
        Check("restore source label is set", source.SetObjectLabel(original, "原始模型"));

        const Limen::AssetID modelID{0x1234567812344abcULL, 0x8000123456789abcULL};
        const Limen::SceneObjectComponent model{
            .ID = {1, 2},
            .LocalTransform = LocationTransform(0.0f, 2.0f, 0.0f),
            .Data = Limen::ModelRendererComponent{.ModelAssetID = modelID}
        };
        Check("restore source component is attached",
              source.AddObjectComponent(original, model) == model.ID);

        Limen::SceneObjectAuthoringData saved;
        source.ForEachObjectAuthoring([&](const auto& snapshot) { saved = snapshot; });
        Check("restore source snapshot is valid",
              saved.ID.isValid() && saved.Components.getSize() == 1);
        if (!saved.ID.isValid() || saved.Components.getSize() != 1)
            return;

        Limen::Scene target;
        const auto survivor = target.CreateObject();
        const auto freeTail = target.CreateObject();
        const auto freeHead = target.CreateObject();
        Limen::SceneObjectID survivorID{};
        Check("restore target existing ID is available", target.TryGetObjectID(survivor, survivorID));
        Check("restore target existing label is set", target.SetObjectLabel(survivor, "Survivor"));
        Check("restore target tail slot is freed", target.DestroyObject(freeTail));
        Check("restore target head slot is freed", target.DestroyObject(freeHead));

        auto invalid = saved;
        invalid.ID = {};
        Check("restore rejects zero object ID", !target.Restore(invalid).IsValid());
        invalid.ID = survivorID;
        Check("restore rejects duplicate object ID", !target.Restore(invalid).IsValid());

        invalid = saved;
        invalid.Transform.Location.x = std::numeric_limits<float>::infinity();
        Check("restore rejects non-finite object TRS", !target.Restore(invalid).IsValid());

        invalid = saved;
        invalid.Components[0].ID = {};
        Check("restore rejects zero component ID", !target.Restore(invalid).IsValid());
        invalid.Components[0].ID = model.ID;
        invalid.Components[0].Data = Limen::ModelRendererComponent{};
        Check("restore rejects missing model asset ID", !target.Restore(invalid).IsValid());

        invalid = saved;
        invalid.Components.push_back(model);
        Check("restore rejects duplicate component ID", !target.Restore(invalid).IsValid());

        int liveObjects = 0;
        target.ForEachObject([&](const auto&, const auto&) { ++liveObjects; });
        Limen::LString survivorLabel;
        Check("rejected restores leave the target scene intact",
              liveObjects == 1 && target.TryGetObjectLabel(survivor, survivorLabel) &&
                  survivorLabel == "Survivor");

        const auto restored = target.Restore(saved);
        Check("restore reuses the original free head without consuming a generation",
              target.IsObjectAlive(restored) && restored.Index == freeHead.Index &&
                  restored.Generation == freeHead.Generation + 1);
        if (!target.IsObjectAlive(restored))
            return;

        Check("restored live slot is detached from the free chain",
              Limen::SceneObjectSlotTestAccess::GetNextFree(target, restored.Index) ==
                  Limen::SceneObjectHandle::InvalidIndex);
        Check("old target handle remains invalid", !target.IsObjectAlive(freeHead));
        Limen::SceneObjectHandle located{};
        Check("restored persistent ID resolves to the new handle",
              target.TryFindObject(saved.ID, located) && located == restored);

        // 恢复后修改输入快照，验证目标 Scene 独立持有 Label 与组件。
        saved.Label = "Changed input";
        saved.Components.clear();
        int restoredSnapshots = 0;
        target.ForEachObjectAuthoring([&](const auto& snapshot)
        {
            if (snapshot.ID != saved.ID)
                return;
            ++restoredSnapshots;
            Check("restored snapshot preserves the label", snapshot.Label == "原始模型");
            CheckAuthoringTransform("restored snapshot preserves root TRS", snapshot.Transform, transform);
            Check("restored snapshot preserves the component count", snapshot.Components.getSize() == 1);
            if (snapshot.Components.getSize() != 1)
                return;

            const auto& component = snapshot.Components[0];
            const auto* renderer = std::get_if<Limen::ModelRendererComponent>(&component.Data);
            Check("restored snapshot preserves component and asset IDs",
                  component.ID == model.ID && renderer && renderer->ModelAssetID == modelID);
            CheckAuthoringTransform("restored snapshot preserves component TRS",
                                    component.LocalTransform, model.LocalTransform);
        });
        Check("restored object appears exactly once in snapshots", restoredSnapshots == 1);

        glm::mat4 restoredMatrix{1.0f};
        Check("restored object has a runtime matrix", target.TryGetObjectTransform(restored, restoredMatrix));
        CheckTransform("restored matrix is derived from TRS", restoredMatrix, transform.ToMatrix());

        int runtimeParts = 0;
        target.ForEachRenderableObject([&](const auto&, const auto&, const auto&, const auto&, const auto&)
        {
            ++runtimeParts;
        });
        Check("authoring restore leaves runtime render parts empty", runtimeParts == 0);

        const auto next = target.CreateObject();
        Check("restore preserves the remainder of the free chain",
              target.IsObjectAlive(next) && next.Index == freeTail.Index &&
                  next.Generation == freeTail.Generation + 1);
    }

    void TestSceneDocumentCapture()
    {
        const Limen::AmbientLight ambient{{0.25f, 0.5f, 0.75f}, 0.5f};
        const Limen::DirectionalLight directional{
            {-1.0f, -2.0f, 3.0f}, {1.0f, 0.75f, 0.5f}, 2.0f
        };
        auto transform = LocationTransform(3.0f, -2.0f, 1.0f);
        transform.Rotation = glm::quat{0.0f, 0.0f, 1.0f, 0.0f};
        transform.Scale = {2.0f, 3.0f, 4.0f};
        const Limen::SceneObjectComponent cube{
            .ID = {1, 2},
            .Data = Limen::PrimitiveRendererComponent{}
        };
        const Limen::PointLightComponent lightData{{1.0f, 0.5f, 0.25f}, 4.0f};
        const Limen::SceneObjectComponent light{
            .ID = {3, 4},
            .LocalTransform = LocationTransform(0.0f, 2.0f, -1.0f),
            .Data = lightData
        };

        Limen::SceneObjectID objectID{};
        const auto document = [&]
        {
            Limen::Scene scene;
            scene.SetAmbientLight(ambient);
            scene.SetDirectionalLight(directional);
            const auto removed = scene.CreateObject();
            const auto object = scene.CreateObject();
            Check("document source has a stable object ID", scene.TryGetObjectID(object, objectID));
            Check("document source label is set", scene.SetObjectLabel(object, "主场景立方体"));
            Check("document source root TRS is set", scene.SetObjectAuthoringTransform(object, transform));
            Check("document source has a primitive component", scene.AddObjectComponent(object, cube) == cube.ID);
            Check("document source has a point light component", scene.AddObjectComponent(object, light) == light.ID);
            Check("document fixture frees an earlier slot", scene.DestroyObject(removed));

            char displayName[] = "Main Scene";
            auto snapshot = Limen::CaptureSceneDocument(scene, displayName);

            // 捕获后修改名称缓冲区与原场景，再销毁场景，验证文档持有独立副本。
            displayName[0] = 'X';
            scene.SetAmbientLight({});
            scene.SetDirectionalLight({});
            Check("source label can change after capture", scene.SetObjectLabel(object, "Changed"));
            Check("source TRS can change after capture", scene.SetObjectAuthoringTransform(object, {}));
            Check("source object can be destroyed after capture", scene.DestroyObject(object));
            return snapshot;
        }();

        Check("document records schema version 1", document.SchemaVersion == 1);
        Check("document owns the display name", document.DisplayName == "Main Scene");
        Check("document preserves ambient light values",
              document.Ambient.Color == ambient.Color && document.Ambient.Intensity == ambient.Intensity);
        Check("document preserves directional light values",
              document.DirectionLight.Direction == directional.Direction &&
                  document.DirectionLight.Color == directional.Color &&
                  document.DirectionLight.Intensity == directional.Intensity);
        Check("document captures only the live object", document.Objects.getSize() == 1);
        if (document.Objects.getSize() != 1)
            return;

        const auto &object = document.Objects[0];
        Check("document preserves object ID and UTF-8 label",
              objectID.isValid() && object.ID == objectID && object.Label == "主场景立方体");
        CheckAuthoringTransform("document preserves root TRS", object.Transform, transform);
        Check("document preserves both components", object.Components.getSize() == 2);
        if (object.Components.getSize() != 2)
            return;

        const auto &savedCube = object.Components[0];
        const auto *primitive = std::get_if<Limen::PrimitiveRendererComponent>(&savedCube.Data);
        Check("document preserves primitive component ID and type",
              savedCube.ID == cube.ID && primitive && primitive->Primitive == Limen::PrimitiveType::Cube);
        const auto &savedLight = object.Components[1];
        const auto *pointLight = std::get_if<Limen::PointLightComponent>(&savedLight.Data);
        Check("document preserves point light component ID and values",
              savedLight.ID == light.ID && pointLight && pointLight->Color == lightData.Color &&
                  pointLight->Intensity == lightData.Intensity);
        CheckAuthoringTransform("document preserves point light local TRS",
                                savedLight.LocalTransform, light.LocalTransform);
    }
}

int main()
{
    TestObjectLifecycle();
    TestIndependentObjectsAndInvalidInput();
    TestSlotRetirementBoundary();
    TestForEachObject();
    TestSceneRenderableComponent();
    TestGroupedRenderableComponents();
    TestObjectLabels();
    TestAuthoringTransform();
    TestSceneObjectComponents();
    TestObjectAuthoringSnapshot();
    TestObjectAuthoringRestore();
    TestSceneDocumentCapture();

    if (g_Failures == 0)
        std::cout
                << "Scene object identity, generation, retirement, render views, and document capture: PASS\n";

    return g_Failures == 0 ? 0 : 1;
}
