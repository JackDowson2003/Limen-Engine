#pragma once

#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/Event.h"
#include "Limen/Tool/LString.h"

namespace  Limen {

	class Window;

	class LIMEN_API Layer
	{
	public:
		explicit Layer( LString  name = "Layer");
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

		[[nodiscard]] const LString& GetName() const { return m_DebugName; }
	protected:
		LString m_DebugName;
	};

}
