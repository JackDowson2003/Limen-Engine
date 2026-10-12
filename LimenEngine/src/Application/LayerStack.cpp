#include "Limen/Application/LayerStack.h"

namespace Limen {

	LayerStack::~LayerStack()
	{
		Clear();
	}

	void LayerStack::Clear()
	{
		for (Layer* layer : m_Layers)
		{
			layer->OnDetach();
			delete layer;
		}

		m_Layers.clear();
		m_LayerInsertIndex = 0;
	}

	void LayerStack::PushLayer(Layer* layer)
	{
		// 普通 Layer 位于 Overlay 前；先在末尾增加一个元素，再将 Overlay 向后移动一格。
		const uint32_t oldSize = m_Layers.getSize();
		m_Layers.push_back(layer);
		for (uint32_t index = oldSize; index > m_LayerInsertIndex; --index)
			m_Layers[index] = m_Layers[index - 1];
		m_Layers[m_LayerInsertIndex] = layer;
		m_LayerInsertIndex++;
	}

	void LayerStack::PushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::PopLayer(Layer* layer)
	{
		for (uint32_t index = 0; index < m_LayerInsertIndex; ++index)
		{
			if (m_Layers[index] != layer)
				continue;

			layer->OnDetach();
			for (uint32_t next = index + 1; next < m_Layers.getSize(); ++next)
				m_Layers[next - 1] = m_Layers[next];
			m_Layers.pop_back();
			m_LayerInsertIndex--;
			return;
		}
	}

	void LayerStack::PopOverlay(Layer* overlay)
	{
		for (uint32_t index = m_LayerInsertIndex; index < m_Layers.getSize(); ++index)
		{
			if (m_Layers[index] != overlay)
				continue;

			overlay->OnDetach();
			for (uint32_t next = index + 1; next < m_Layers.getSize(); ++next)
				m_Layers[next - 1] = m_Layers[next];
			m_Layers.pop_back();
			return;
		}
	}

}
