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

		Shader* GetShaderVideo() const {
			return m_VideoShader;
		}

		Shader* GetShaderCamera() const {
			return m_CameraShader;
		}

		void BindImageShader() {
			m_Shader->Bind();
		}

		void BindVideoShader() {
			m_VideoShader->Bind();
		}

		void BindCameraShader() {
			m_CameraShader->Bind();
		}
	private:
		Shader *m_Shader, *m_VideoShader, *m_CameraShader;
		uint32_t m_VertexArray = 0;
		uint32_t m_LineVertexBuffer = 0;
	};
}