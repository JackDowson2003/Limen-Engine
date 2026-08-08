#pragma once

#include "Core.h"

#include <cstdint>

namespace Limen {

	// Events in Limen are currently blocking, meaning when an event occurs it
	// immediately gets dispatched and must be dealt with right then an there.
	// For the future, a better strategy might be to buffer events in an event
	// bus and process them during the "event" part of the update stage.

	enum class EventType
	{
		None = 0,
		WindowClose, WindowResize, WindowFocus, WindowLostFocus, WindowMoved,
		AppTick, AppUpdate, AppRender,
		KeyPressed, KeyReleased, KeyTyped,
		MouseButtonPressed, MouseButtonReleased, MouseMoved, MouseScrolled
	};

	enum class   EventCategory : std::uint32_t
	{
		None = 0,
		EventCategoryApplication    = BIT(0), //1
		EventCategoryInput          = BIT(1), //2
		EventCategoryKeyboard       = BIT(2), //4
		EventCategoryMouse          = BIT(3), //8
		EventCategoryMouseButton    = BIT(4) //16
	};

	// 保留 enum class 的强类型约束，同时让事件声明可以简写为
	// EVENT_CLASS_CATEGORY(EventCategoryMouse | EventCategoryInput)。
	using enum EventCategory;

	constexpr EventCategory operator|(EventCategory lhs, EventCategory rhs)
	{
		return static_cast<EventCategory>(
			static_cast<std::uint32_t>(lhs) | static_cast<std::uint32_t>(rhs)
		);
	}

	constexpr EventCategory operator&(EventCategory lhs, EventCategory rhs)
	{
		return static_cast<EventCategory>(
			static_cast<std::uint32_t>(lhs) & static_cast<std::uint32_t>(rhs)
		);
	}

	constexpr EventCategory& operator|=(EventCategory& lhs, EventCategory rhs)
	{
		lhs = lhs | rhs;
		return lhs;
	}

#define EVENT_CLASS_TYPE(type) static EventType GetStaticType() { return EventType::type; }\
								virtual EventType GetEventType() const override { return GetStaticType(); }\
								virtual const char* GetName() const override { return #type; }
	using enum EventCategory;
#define EVENT_CLASS_CATEGORY(category) EventCategory GetCategoryFlags() const override { return (category); }

	class LIMEN_API Event
	{
		friend class EventDispatcher;
	public:
		virtual ~Event() = default;

		[[nodiscard]] virtual EventType GetEventType() const = 0;
		virtual const char* GetName() const = 0;
		[[nodiscard]] virtual EventCategory GetCategoryFlags() const = 0;
		virtual std::string ToString() const { return GetName(); }
		[[nodiscard]] bool IsHandled() const
		{
			return m_Handled;
		}

		[[nodiscard]] bool IsInCategory(const EventCategory category) const
		{
			return (GetCategoryFlags() & category) != None;
		}
	protected:
		bool m_Handled = false;
	};

	class LIMEN_API EventDispatcher
	{
		template<typename T>
		using EventFn = std::function<bool(T&)>;
	public:
		EventDispatcher(Event& event)
			: m_Event(event)
		{
		}
		
		// F will be deduced by the compiler
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				m_Event.m_Handled |= func(static_cast<T&>(m_Event)); //会保留以前的状态 true | false == true
				return true;
			}
			return false;
		}
	private:
		Event& m_Event;
	};

	inline std::ostream& operator<<(std::ostream& os, const Event& e)
	{
		return os << e.ToString();
	}

}
