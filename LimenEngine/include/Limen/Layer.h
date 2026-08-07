#pragma once

#include "Events/Event.h"

namespace  Limen {

	class LIMEN_API Layer
	{
	public:
		Layer( std::string  name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}
		virtual void OnUpdate() {}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		[[nodiscard]] inline  const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}