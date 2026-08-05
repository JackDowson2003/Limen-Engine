#include "Limen.h"

class SandBoxApp : public Limen::Application
{
public:
    SandBoxApp()
    {
    }

    ~SandBoxApp()
    {
    }
};

//不实现是无法run的
Limen::Application* Limen::CreateApplication()
{
    return new SandBoxApp();
}