#include <cstdint>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Limen/Scene/Scene.h"

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
        Check("render object is alive", scene.IsObjectAlive(handle));

        Limen::SceneObjectID renderID{};
        Check("render object also has stable identity",
              scene.TryGetObjectID(handle, renderID) && renderID.isValid());

        Limen::SceneRenderObject actual;
        Check("render object can be queried",
              scene.TryGetRenderObject(handle, actual));
        Check("queried mesh is preserved", actual.MeshResource == mesh);
        Check("queried material is preserved",
              actual.MaterialResource == material);
        CheckTransform("queried render transform",
                       actual.Transform,
                       source.Transform);

        const glm::mat4 moved = Translation(4.0f, 5.0f, 6.0f);
        Check("legacy render transform entry updates object",
              scene.SetRenderObjectTransform(handle, moved));

        glm::mat4 actualTransform{};
        Check("unified transform is visible through object query",
              scene.TryGetObjectTransform(handle, actualTransform));
        CheckTransform("render transform after compatibility update",
                       actualTransform,
                       moved);

        const glm::mat4 movedAgain = Translation(7.0f, 8.0f, 9.0f);
        Check("object transform entry updates render data",
              scene.SetObjectTransform(handle, movedAgain));
        Check("updated render object can be queried",
              scene.TryGetRenderObject(handle, actual));
        CheckTransform("render transform after object update",
                       actual.Transform,
                       movedAgain);

        int visited = 0;
        scene.ForEachRenderObject(
            [&](const Limen::SceneRenderObject& renderObject)
            {
                ++visited;
                Check("render visitor sees original mesh",
                      renderObject.MeshResource == mesh);
                Check("render visitor sees original material",
                      renderObject.MaterialResource == material);
                CheckTransform("render visitor sees current transform",
                               renderObject.Transform,
                               movedAgain);
            }
        );
        Check("one renderable object is visited", visited == 1);

        const auto plainObject = scene.CreateObject();
        Check("plain scene object is valid", plainObject.IsValid());

        Limen::SceneRenderObject untouched = source;
        untouched.Transform = Translation(40.0f, 50.0f, 60.0f);
        Check("plain object is not renderable",
              !scene.TryGetRenderObject(plainObject, untouched));
        Check("failed render query preserves mesh",
              untouched.MeshResource == mesh);
        Check("failed render query preserves material",
              untouched.MaterialResource == material);
        CheckTransform("failed render query preserves transform",
                       untouched.Transform,
                       Translation(40.0f, 50.0f, 60.0f));

        visited = 0;
        scene.ForEachRenderObject(
            [&](const Limen::SceneRenderObject&)
            {
                ++visited;
            }
        );
        Check("plain object is skipped by render visitor", visited == 1);

        Check("render object can be destroyed", scene.DestroyObject(handle));

        visited = 0;
        scene.ForEachRenderObject(
            [&](const Limen::SceneRenderObject&)
            {
                ++visited;
            }
        );
        Check("destroyed render object is not visited", visited == 0);

        const auto emptyHandle = scene.AddRenderObject(Limen::SceneRenderObject{});
        Check("empty render object is rejected", !emptyHandle.IsValid());

        Limen::SceneRenderObject missingMaterial;
        missingMaterial.MeshResource = mesh;
        const auto missingMaterialHandle = scene.AddRenderObject(missingMaterial);
        Check("render object without material is rejected",
              !missingMaterialHandle.IsValid());

        Limen::SceneRenderObject missingMesh;
        missingMesh.MaterialResource = material;
        const auto missingMeshHandle = scene.AddRenderObject(missingMesh);
        Check("render object without mesh is rejected",
              !missingMeshHandle.IsValid());

        visited = 0;
        scene.ForEachRenderObject(
            [&](const Limen::SceneRenderObject&)
            {
                ++visited;
            }
        );
        Check("rejected render objects do not enter render visitor",
              visited == 0);

        Check("plain object remains alive after render cleanup",
              scene.IsObjectAlive(plainObject));
        Check("plain object can be destroyed",
              scene.DestroyObject(plainObject));
    }
}

int main()
{
    TestObjectLifecycle();
    TestIndependentObjectsAndInvalidInput();
    TestRenderObjectLifecycle();

    if (g_Failures == 0)
        std::cout << "Scene object identity, generation, and render views: PASS\n";

    return g_Failures == 0 ? 0 : 1;
}
