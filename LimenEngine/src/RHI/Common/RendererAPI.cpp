//
// Created by chenlong on 2026/8/12.
//
#include "Limen/RHI/RendererAPI.h"

namespace Limen
{
    // 当前阶段默认使用OpenGL。Application会在创建Window/Context前
    // 调用SetAPI()；以后macOS可选Metal，Windows可选Direct3D 11/12。
    RendererAPI::API RendererAPI::s_API =
        RendererAPI::API::OPENGL;

}
