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

		void BeginScene(const glm::mat4& projection, 
			const glm::mat4& view, 
			const glm::mat4& model,
			std::vector<dlib::rectangle> rectangles);
	private:
		Shader *m_Shader, *m_LineShader;
		uint32_t m_VertexArray = 0, m_LineVertexArray = 0;
		uint32_t m_LineVertexBuffer = 0;
		glm::mat4 m_Projection, m_View, m_Model;
		std::vector<dlib::rectangle> m_Dets;
	};
}