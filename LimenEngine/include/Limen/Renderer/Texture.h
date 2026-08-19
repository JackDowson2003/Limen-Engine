//
// Created by chenlong on 2026/8/11.
//

#pragma once

#include "Core.h"

namespace Limen
{
    class LIMEN_API Texture
    {
    public:
        virtual ~Texture() = default;

        virtual uint32_t GetWidth() const noexcept = 0;

        virtual uint32_t GetHeight() const noexcept = 0;

        virtual void Bind(uint32_t slot = 0) const = 0;

    };

    class LIMEN_API Texture2D : public Texture
    {
    public:
        static Ref<Texture2D> Create(const char* path);
    };
}
