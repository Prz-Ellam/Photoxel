#pragma once

#include <inttypes.h>
#include <memory>
#include "Shader.h"
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_io.h>

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
		void Bind() {
			m_Shader->Bind();
		}
	private:
		Shader *m_Shader;
		uint32_t m_VertexArray = 0;
		uint32_t m_LineVertexBuffer = 0;
	};
}