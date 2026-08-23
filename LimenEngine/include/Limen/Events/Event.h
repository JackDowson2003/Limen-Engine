#pragma once

#include <functional>
#include <ostream>
#include <string>

#include "Limen/Core/Core.h"


namespace Limen {

	// 当前事件采用同步分发：窗口回调产生事件后，立即沿 LayerStack 传播。

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
		EventCategoryApplication = BIT(0),
		EventCategoryInput = BIT(1),
		EventCategoryKeyboard = BIT(2),
		EventCategoryMouse = BIT(3),
		EventCategoryMouseButton = BIT(4)
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
		[[nodiscard]] virtual const char* GetName() const = 0;
		[[nodiscard]] virtual EventCategory GetCategoryFlags() const = 0;
		[[nodiscard]] virtual std::string ToString() const { return GetName(); }
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
		
		// F 由编译器从传入的可调用对象推导。
		template<typename T, typename F>
		bool Dispatch(const F& func)
		{
			if (m_Event.GetEventType() == T::GetStaticType())
			{
				// 一旦事件已被处理，后续回调不能把它恢复为未处理状态。
				m_Event.m_Handled |= func(static_cast<T&>(m_Event));
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
