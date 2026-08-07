//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Core.h"

namespace  Limen
{

    class LIMEN_API Application //静态链接不需要写__declspec(dllexport)
    {
    public:
        Application();
        virtual ~Application(); //交给sandbox去实现

         void Run();
    };

    //To be defined in CLIENT
    Application* CreateApplication();

}
