//
// Created by chenlong on 2026/10/14.
//
#include "Limen.h"
#include "Limen/Asset/AssetRegistry.h"
#include "Project/EditorSession.h"
#include "UI/EditorUILayer.h"

namespace
{
    class EditorApp final : public Limen::Application
    {
    public:

        explicit EditorApp(Limen::Ref<LimenEditor::EditorSession> session = {})
            : Application(
                  false,
                  Limen::RendererAPI::API::OPENGL,
                  false,
                  session ? session->GetConfiguration().AssetRootAbsolute : std::filesystem::path{"assets"}
              ),
              m_Session(std::move(session))
        {
            PushLayer(new LimenEditor::EditorUILayer(m_Session));
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
    if (argc == 3 && LString(argv[1]) == "--project")
    {
        if (!TryLoadProjectConfiguration(std::filesystem::path(argv[2]), pc, error))
        {
            LM_ERROR(
                "打开项目失败: file='{}', key='{}', reason='{}'",
                error.File.string(), error.Key, error.Message
            );
            return nullptr;
        }
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
        LM_ERROR("用法: LimenEditor [--project <file.lmproject> | "
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

    auto session =
        CreateRef<LimenEditor::EditorSession>(std::move(pc), std::move(registry));

    return new EditorApp(std::move(session));
}
