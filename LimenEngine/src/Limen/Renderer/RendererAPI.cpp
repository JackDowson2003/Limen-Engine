//
// Created by chenlong on 2026/8/12.
//
#include "Renderer/RendererAPI.h"

namespace Limen
{
    // 当前阶段只实现OpenGL。以后Windows客户端可在Renderer初始化前，
    // 通过SetAPI()选择DIRECT11或DIRECT12。
    RendererAPI::API RendererAPI::s_API =
        RendererAPI::API::OPENGL;

}
