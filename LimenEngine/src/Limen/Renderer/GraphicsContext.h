//
// Created by chenlong on 2026/8/10.
//

#pragma once

namespace Limen
{
    class  GraphicsContext
    {
    public:
        GraphicsContext() = default;
        virtual ~GraphicsContext() = default;
        virtual void Init() = 0;
        virtual void SwapBuffers() = 0;
        virtual void Shutdown() = 0;

    };
}
