#pragma once

#include <inttypes.h>
#include <memory>
#include "Shader.h"
#include <glm/glm.hpp>

namespace Photoxel
{
	class Renderer
	{
	public:
		Renderer();
		~Renderer();
		void OnRender();

		void BeginScene(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model);
	private:
		Shader *m_Shader;
		uint32_t m_VertexArray = 0;
		glm::mat4 m_Projection, m_View, m_Model;
	};
}