//
// Created by chenlong on 2026/8/22.
//

#include "SandBox2D.h"

SandBox2D::SandBox2D()
{
    m_Layer2D = Limen::CreateRef<SandBox::ExampleLayer>();
}

void SandBox2D::OnAttach()
{
    m_Layer2D->OnAttach();
}

void SandBox2D::OnDetach()
{
    m_Layer2D->OnDetach();
}

void SandBox2D::OnUpdate(Limen::DeltaTime &dt)
{
    m_Layer2D->OnUpdate(dt);
}

void SandBox2D::OnImGuiRender()
{
    m_Layer2D->OnImGuiRender();
}

void SandBox2D::OnEvent(Limen::Event &event)
{
    m_Layer2D->OnEvent(event);
}
