#include <chrono>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <variant>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Project/ProjectConfiguration.h"
#include "Limen/Scene/SceneComponent.h"
#include "Project/EditorSession.h"
#include "Render/EditorPrimitiveSpawner.h"
#include "Render/EditorSceneContent.h"

namespace
{
    namespace fs = std::filesystem;

    bool Check(const char *name, const bool passed)
    {
        if (!passed)
            std::cerr << "FAIL: " << name << '\n';
        return passed;
    }

    bool Near(const float actual, const float expected)
    {
        return std::abs(actual - expected) <= 1.0e-5f;
    }

    // 测试只删除自己创建的临时目录，避免影响工作区中的项目资产。
    struct TemporaryAssetRoot
    {
        fs::path Root;

        TemporaryAssetRoot()
        {
            const auto stamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            for (int attempt = 0; attempt < 100; ++attempt)
            {
                const fs::path candidate =
                    fs::temp_directory_path() /
                    ("limen-editor-primitive-spawner-test-" +
                     std::to_string(stamp) + "-" +
                     std::to_string(attempt));
                std::error_code error;
                if (fs::create_directory(candidate, error))
                {
                    Root = candidate;
                    return;
                }
                if (error)
                {
                    throw fs::filesystem_error(
                        "Cannot create test asset root",
                        candidate,
                        error);
                }
            }
            throw std::runtime_error("Cannot reserve a unique test asset root");
        }

        ~TemporaryAssetRoot()
        {
            if (Root.empty())
                return;

            std::error_code ignored;
            fs::remove_all(Root, ignored);
        }

        TemporaryAssetRoot(const TemporaryAssetRoot &) = delete;
        TemporaryAssetRoot &operator=(const TemporaryAssetRoot &) = delete;
    };

    LimenEditor::EditorSession MakeSession(const fs::path &assetRoot)
    {
        Limen::ProjectConfiguration configuration;
        configuration.AssetRootAbsolute = assetRoot;

        auto registry =
            Limen::CreateScope<Limen::AssetRegistry>(assetRoot);
        Limen::AssetRegistryError error;
        if (!registry->TryLoad(error))
        {
            throw std::runtime_error(
                "Cannot load test asset registry: " +
                std::string(error.Message.c_str()));
        }
        return LimenEditor::EditorSession(
            std::move(configuration),
            std::move(registry));
    }

    std::uint32_t CountObjects(const LimenEditor::EditorSession &session)
    {
        std::uint32_t count = 0;
        session.GetScene().ForEachObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &)
            {
                ++count;
            });
        return count;
    }

    std::uint32_t CountRenderableVisits(
        const LimenEditor::EditorSession &session,
        const Limen::SceneObjectID id)
    {
        std::uint32_t count = 0;
        session.GetScene().ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &,
                const Limen::SceneObjectID &visitedID,
                const Limen::Ref<Limen::Mesh> &,
                const Limen::Ref<Limen::Material> &,
                const glm::mat4 &)
            {
                if (visitedID == id)
                    ++count;
            });
        return count;
    }

    bool TestPointLightSpawn(const fs::path &assetRoot)
    {
        LimenEditor::EditorSession session = MakeSession(assetRoot);
        LimenEditor::EditorPrimitiveSpawner spawner;

        Limen::SceneTransform transform;
        transform.Location = {3.0f, 4.0f, 5.0f};

        Limen::LArray<Limen::SceneObjectID> created;
        created.push_back(Limen::SceneObjectID{
            0xDEADBEEFDEADBEEFULL,
            0xDEADBEEFDEADBEEFULL
        });
        std::string error = "unchanged";
        bool passed = Check(
            "point light spawn succeeds",
            spawner.SpawnPointLight(
                session,
                transform,
                created,
                &error));
        passed &= Check("point light spawn clears stale out IDs",
                        created.getSize() == 1);
        passed &= Check("point light spawn clears stale error",
                        error.empty());
        passed &= Check("point light spawn selects its object",
                        created.getSize() == 1 &&
                        session.GetSelectedObjectID() == created[0]);
        passed &= Check("point light spawn marks session dirty",
                        session.IsDirty());
        passed &= Check("point light spawn creates one object",
                        CountObjects(session) == 1);

        Limen::LString label;
        passed &= Check("point light receives default label",
                        created.getSize() == 1 &&
                        session.TryGetObjectLabel(created[0], label) &&
                        label == "PointLight");

        Limen::SceneTransform storedTransform;
        passed &= Check("point light stores placement transform",
                        created.getSize() == 1 &&
                        session.TryGetObjectTransform(
                            created[0],
                            storedTransform) &&
                        Near(storedTransform.Location.x, 3.0f) &&
                        Near(storedTransform.Location.y, 4.0f) &&
                        Near(storedTransform.Location.z, 5.0f));

        Limen::SceneObjectHandle handle{};
        std::uint32_t componentCount = 0;
        bool hasExpectedLight = false;
        if (created.getSize() == 1)
        {
            passed &= Check(
                "spawned point light can be found",
                session.GetScene().TryFindObject(created[0], handle));
            if (handle.IsValid())
            {
                session.GetScene().ForEachObjectComponent(
                    handle,
                    [&](const Limen::SceneObjectComponent &component)
                    {
                        ++componentCount;
                        const auto *light =
                            std::get_if<Limen::PointLightComponent>(
                                &component.Data);
                        if (!light)
                            return;

                        hasExpectedLight =
                            Near(light->Color.x, 1.0f) &&
                            Near(light->Color.y, 1.0f) &&
                            Near(light->Color.z, 1.0f) &&
                            Near(light->Intensity, 1.0f);
                    });
            }
        }
        passed &= Check("point light attaches one authoring component",
                        componentCount == 1);
        passed &= Check("point light attaches expected component data",
                        hasExpectedLight);
        passed &= Check("point light object has no render components",
                        created.getSize() == 1 &&
                        CountRenderableVisits(session, created[0]) == 0);

        Limen::LArray<Limen::SceneObjectID> secondCreated;
        Limen::SceneTransform secondTransform;
        secondTransform.Location = {-1.0f, 0.0f, 2.0f};
        passed &= Check(
            "second point light spawns",
            spawner.SpawnPointLight(
                session,
                secondTransform,
                secondCreated));
        Limen::LString secondLabel;
        passed &= Check("second point light receives unique label",
                        secondCreated.getSize() == 1 &&
                        session.TryGetObjectLabel(
                            secondCreated[0],
                            secondLabel) &&
                        secondLabel == "PointLight_2");
        passed &= Check("point light spawn does not reuse labels",
                        created.getSize() == 1 &&
                        secondCreated.getSize() == 1 &&
                        !(created[0] == secondCreated[0]));
        return passed;
    }

    bool TestFailedSpawnsLeaveNoObject(const fs::path &assetRoot)
    {
        LimenEditor::EditorSession session = MakeSession(assetRoot);
        LimenEditor::EditorPrimitiveSpawner spawner;

        const Limen::SceneObjectID staleID{
            0xDEADBEEFDEADBEEFULL,
            0xDEADBEEFDEADBEEFULL
        };
        Limen::LArray<Limen::SceneObjectID> created;
        created.push_back(staleID);
        std::string error = "unchanged";

        // 未初始化的 SceneContent 没有 Cube 运行期资源，必须在创建对象前失败。
        LimenEditor::EditorSceneContent emptyContent;
        Limen::SceneTransform transform;
        bool passed = Check(
            "cube spawn fails without runtime resources",
            !spawner.SpawnCube(
                session,
                emptyContent,
                transform,
                created,
                &error));
        passed &= Check("failed cube resource spawn clears out IDs",
                        created.getSize() == 0);
        passed &= Check("failed cube resource spawn reports error",
                        !error.empty());
        passed &= Check("failed cube resource spawn creates no object",
                        CountObjects(session) == 0);

        // 非有限 Transform 会在对象已创建后被 Scene 拒绝；Spawner 必须回滚。
        Limen::SceneTransform invalidTransform;
        invalidTransform.Location.x =
            std::numeric_limits<float>::infinity();
        created.push_back(staleID);
        error = "unchanged";
        passed &= Check(
            "point light spawn rejects non-finite transform",
            !spawner.SpawnPointLight(
                session,
                invalidTransform,
                created,
                &error));
        passed &= Check("failed point light spawn clears out IDs",
                        created.getSize() == 0);
        passed &= Check("failed point light spawn reports error",
                        !error.empty());
        passed &= Check("failed point light spawn rolls back object",
                        CountObjects(session) == 0);
        passed &= Check("failed point light spawn clears selection",
                        session.GetSelectedObjectCount() == 0);
        return passed;
    }
}

int main()
{
    try
    {
        TemporaryAssetRoot assetRoot;
        bool passed = true;
        passed &= TestPointLightSpawn(assetRoot.Root);
        passed &= TestFailedSpawnsLeaveNoObject(assetRoot.Root);
        if (passed)
            std::cout << "Editor primitive spawner: PASS\n";
        return passed ? 0 : 1;
    }
    catch (const std::exception &error)
    {
        std::cerr << "Editor primitive spawner test exception: "
                  << error.what() << '\n';
        return 1;
    }
}
