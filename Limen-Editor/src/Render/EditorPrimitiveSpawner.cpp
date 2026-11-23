#include "Render/EditorPrimitiveSpawner.h"

#include <string_view>
#include <utility>

#include "Project/EditorSession.h"
#include "Render/EditorObjectNaming.h"
#include "Render/EditorSceneContent.h"

namespace LimenEditor
{
    namespace
    {
        void SetError(std::string *outError, const std::string_view message)
        {
            if (outError)
                *outError = std::string(message);
        }
    }

    bool EditorPrimitiveSpawner::SpawnCube(
        EditorSession &session,
        const EditorSceneContent &sceneContent,
        const Limen::SceneTransform &transform,
        Limen::LArray<Limen::SceneObjectID> &outIDs,
        std::string *outError) const
    {
        outIDs.clear();
        if (outError)
            outError->clear();

        const Limen::Ref<Limen::Mesh> &mesh =
            sceneContent.GetCubeMesh();
        const Limen::Ref<Limen::Material> &material =
            sceneContent.GetCubeMaterial();
        if (!mesh || !material)
        {
            SetError(outError, "Editor primitive resources are unavailable.");
            return false;
        }

        const std::string label =
            MakeUniqueObjectLabel(session, "Cube");
        const Limen::SceneObjectID id =
            session.CreateRenderableObject(
                mesh,
                material,
                transform,
                label);
        if (!id.isValid())
        {
            SetError(outError, "Failed to create primitive object.");
            return false;
        }

        Limen::SceneObjectComponent component;
        component.Data = Limen::PrimitiveRendererComponent{
            .Primitive = Limen::PrimitiveType::Cube
        };
        if (!session.AddObjectComponent(
                id,
                std::move(component)).isValid())
        {
            static_cast<void>(session.DeleteObject(id));
            SetError(outError, "Failed to attach primitive component.");
            return false;
        }

        outIDs.push_back(id);
        return true;
    }

    bool EditorPrimitiveSpawner::SpawnPointLight(
        EditorSession &session,
        const Limen::SceneTransform &transform,
        Limen::LArray<Limen::SceneObjectID> &outIDs,
        std::string *outError) const
    {
        outIDs.clear();
        if (outError)
            outError->clear();

        const std::string label =
            MakeUniqueObjectLabel(session, "PointLight");
        const Limen::SceneObjectID id = session.CreateObject();
        if (!id.isValid())
        {
            SetError(outError, "Failed to create point light object.");
            return false;
        }
        if (!session.SetObjectLabel(id, label) ||
            !session.SetSelectedObjectTransform(transform))
        {
            static_cast<void>(session.DeleteObject(id));
            SetError(outError, "Failed to initialize point light object.");
            return false;
        }

        Limen::SceneObjectComponent component;
        component.Data = Limen::PointLightComponent{};
        if (!session.AddObjectComponent(
                id,
                std::move(component)).isValid())
        {
            static_cast<void>(session.DeleteObject(id));
            SetError(outError, "Failed to attach point light component.");
            return false;
        }

        outIDs.push_back(id);
        return true;
    }
}
