//
// Created by chenlong on 2026/10/14.
//
#pragma once

#include "Limen/Application/Layer.h"

namespace LimenEditor
{
    // 正式 Editor UI 的入口: 本部只验证接线，暂不绘制
    class EditorUILayer final : public Limen::Layer
    {
    public:
        EditorUILayer()
            : Layer("Limen Editor")
        {
        }

        void OnUIRender(Limen::Window & window) override
        {
        }
    };
}
