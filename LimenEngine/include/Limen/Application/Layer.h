#pragma once

#include <string>

#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/Event.h"

namespace  Limen {

	class Window;

	class LIMEN_API Layer
	{
	public:
		explicit Layer( std::string  name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate( DeltaTime& ) {}

		/**
		 * @brief 窗口清屏后提交正式 UI, 不属于Sandbox的 ImGUI调试阶段
		 *
		 * Window 由 Application 拥有， Layer 仅在本次回调期间借用
		 */
		virtual void OnUIRender(Window& ) {};
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& ) {}

		[[nodiscard]] const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}
