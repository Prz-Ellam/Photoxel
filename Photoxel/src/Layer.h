#pragma once

namespace Photoxel
{
	class Layer
	{
	public:
		virtual void OnAttach() = 0;
		virtual void OnDetach() = 0;
		virtual void OnUIRender() = 0;
		virtual void OnUpdate() = 0;
	};
}