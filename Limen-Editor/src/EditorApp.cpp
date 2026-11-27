//
// Created by chenlong on 2026/10/14.
//
#include "Limen.h"
#include "Limen/Asset/AssetRegistry.h"
#include "Project/EditorSession.h"
#include "Project/EditorStartupScene.h"
#include "UI/EditorUILayer.h"
#include "UI/EditorNativeMenu.h"

namespace
{
    class EditorApp final : public Limen::Application
    {
    public:

        explicit EditorApp(Limen::Ref<LimenEditor::EditorSession> session = {},
                           const std::filesystem::path &scenePath = {})
            : Application(
                  false,
                  Limen::RendererAPI::API::OPENGL,
                  false,
                  session ? session->GetConfiguration().AssetRootAbsolute : std::filesystem::path{"assets"}
              ),
              m_Session(std::move(session))
        {
            auto *uiLayer = new LimenEditor::EditorUILayer(m_Session);
            PushLayer(uiLayer);
            LimenEditor::InstallEditorNativeMenu(
                static_cast<bool>(m_Session),
                [uiLayer](const LimenEditor::SceneToolbarAction action) { uiLayer->QueueSceneAction(action); },
                [uiLayer] { static_cast<void>(uiLayer->DeleteSelection()); });
            if (!scenePath.empty())
                uiLayer->OpenSceneFromPath(scenePath);
        }

        ~EditorApp() override
        {
            LimenEditor::UninstallEditorNativeMenu();
        }

    private:
        Limen::Ref<LimenEditor::EditorSession> m_Session;
    };
}

Limen::Application *Limen::CreateApplication(int argc, char **argv)
{
    if (argc == 1)
        return new EditorApp();

    ProjectConfiguration pc;
    ProjectConfigurationError error;
    std::filesystem::path scenePath;
    const bool hasSceneArgument = argc == 5 && LString(argv[3]) == "--scene" && argv[4][0] != '\0';
    if ((argc == 3 || hasSceneArgument) && LString(argv[1]) == "--project")
    {
        if (!TryLoadProjectConfiguration(std::filesystem::path(argv[2]), pc, error))
        {
            LM_ERROR(
                "打开项目失败: file='{}', key='{}', reason='{}'",
                error.File.string(), error.Key, error.Message
            );
            return nullptr;
        }
        if (hasSceneArgument)
            scenePath = argv[4];
    }
    else if (argc == 4 && LString(argv[1]) == "--create-project"){
        if (!TryCreateProjectConfiguration(std::filesystem::path(argv[2]),LString(argv[3]),pc,error))
        {
            LM_ERROR("创建项目配置失败: file='{}', key='{}', reason='{}'",
                 error.File.string(), error.Key, error.Message);
            return nullptr;
        }
    }
    else
    {
        LM_ERROR("用法: LimenEditor [--project <file.lmproject> [--scene <file.lscene>] | "
                 "--create-project <absolute-directory> <display-name>]");
        return nullptr;
    }

    auto registry = CreateScope<AssetRegistry>(pc.AssetRootAbsolute);
    AssetRegistryError registryError;
    if (!registry->TryLoad(registryError))
    {
        LM_ERROR("加载资产注册表失败: path='{}', reason='{}'",
                 registryError.Path.string(), registryError.Message);
        return nullptr;
    }

    // 在创建窗口前解析启动场景 ID；实际场景加载等图形 Context 就绪之后执行。
    if (!hasSceneArgument &&
        !LimenEditor::TryResolveEditorStartupScene(pc, *registry, scenePath, error))
    {
        LM_ERROR("解析项目启动场景失败: file='{}', key='{}', reason='{}'",
                 error.File.string(), error.Key, error.Message);
        return nullptr;
    }

    auto session =
        CreateRef<LimenEditor::EditorSession>(std::move(pc), std::move(registry));

    return new EditorApp(std::move(session), scenePath);
}
