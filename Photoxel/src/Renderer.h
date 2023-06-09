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

		void BeginScene();

		Shader* GetShader() const {
			return m_Shader;
		}

		Shader* GetShaderVideo() const {
			return m_VideoShader;
		}

		void BindImage() {
			m_Shader->Bind();
		}

		void BindVideo() {
			m_VideoShader->Bind();
		}
	private:
		Shader *m_Shader = nullptr, *m_VideoShader = nullptr;
		uint32_t m_VertexArray = 0;
	};
}