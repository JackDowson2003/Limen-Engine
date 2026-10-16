//
// Created by chenlong on 2026/10/14.
//
#include "Limen.h"
#include "UI/EditorUILayer.h"

namespace
{
    class EditorApp final : public Limen::Application
    {
    public:
        EditorApp()
            : Application(
                false,
                Limen::RendererAPI::API::OPENGL
                )
        {
            PushLayer(new LimenEditor::EditorUILayer());
        }
    };
}

Limen::Application *Limen::CreateApplication()
{
    return new EditorApp();
}
