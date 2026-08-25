#pragma once

#include <string>

#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/Event.h"

namespace  Limen {

	class LIMEN_API Layer
	{
	public:
		explicit Layer( std::string  name = "Layer");
		virtual ~Layer() = default;

		virtual void OnAttach() {}
		virtual void OnDetach() {}

		virtual void OnUpdate( DeltaTime& dt){}
		virtual void OnImGuiRender() {}
		virtual void OnEvent(Event& event) {}

		[[nodiscard]] inline  const std::string& GetName() const { return m_DebugName; }
	protected:
		std::string m_DebugName;
	};

}
