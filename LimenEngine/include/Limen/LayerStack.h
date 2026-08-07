#pragma once

#include "Layer.h"

namespace Limen {

	class LIMEN_API LayerStack
	{
	public:
		LayerStack() = default;
		~LayerStack();

		void PushLayer(Layer* layer);
		void PushOverlay(Layer* overlay);
		void PopLayer(Layer* layer);
		void PopOverlay(Layer* overlay);

		std::vector<Layer*>::iterator begin() { return m_Layers.begin(); }
		std::vector<Layer*>::iterator end() { return m_Layers.end(); }
		std::vector<Layer*>::reverse_iterator rbegin() { return m_Layers.rbegin(); }
		std::vector<Layer*>::reverse_iterator rend() { return m_Layers.rend(); }

		[[nodiscard]] std::vector<Layer*>::const_iterator cbegin() const { return m_Layers.cbegin(); }
		[[nodiscard]] std::vector<Layer*>::const_iterator cend()	const { return m_Layers.cend(); }
		[[nodiscard]] std::vector<Layer*>::const_reverse_iterator crbegin() const { return m_Layers.crbegin(); }
		[[nodiscard]] std::vector<Layer*>::const_reverse_iterator crend() const { return m_Layers.crend(); }
	private:
		std::vector<Layer*> m_Layers;
		unsigned int m_LayerInsertIndex = 0;
	};

}