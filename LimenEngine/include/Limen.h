//
// Created by chenlong on 2026/8/5.
//

#pragma once

// Core and application interfaces exposed to Limen clients.
#include "Limen/Application/Application.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/DeltaTime.h"
#include "Limen/Core/Log.h"
#include "Limen/Input/Input.h"
#include "Limen/Input/KeyCodes.h"

// Rendering front-end and public RHI resources.
#include "Limen/RHI/VertexBuffer.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/Renderer/OrthoGraphicCamera.h"
#include "Limen/Renderer/OrthoGraphicCameraController.h"
#include "Limen/Renderer/PerspectiveCamera.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/Renderer/Renderer2D.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/Texture.h"
#include "Limen/RHI/VertexArray.h"
#include "Limen/RHI/UniformBuffer.h"
#include "Limen/RHI/Framebuffer.h"

// 客户端入口点应最后包含，确保上述公共类型已经可用。
#include "Limen/EntryPoint.h"
