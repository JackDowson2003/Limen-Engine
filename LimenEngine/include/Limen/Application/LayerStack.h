#pragma once

#include "Limen/Application/Layer.h"
#include "Limen/Tool/LArray.h"

namespace Limen {

	class LIMEN_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		/**
		 * @brief Detach并销毁当前保存的所有Layer。
		 *
		 * 可以重复调用；析构函数也会调用该函数。
		 */
		void Clear();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		// 移除时只调用 OnDetach，不 delete；调用方需接管并销毁移出的 Layer。
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		auto begin() { return m_Layers.begin(); }
		auto end() { return m_Layers.end(); }
		auto rbegin() { return m_Layers.rbegin(); }
		auto rend() { return m_Layers.rend(); }

		[[nodiscard]] auto begin() const { return m_Layers.cbegin(); }
		[[nodiscard]] auto end() const { return m_Layers.cend(); }
		[[nodiscard]] auto rbegin() const { return m_Layers.crbegin(); }
		[[nodiscard]] auto rend() const { return m_Layers.crend(); }
	private:
		// 仍在栈中的指针由 LayerStack 拥有；[0, m_LayerInsertIndex) 为普通 Layer，其后为 Overlay。
		LArray<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}
