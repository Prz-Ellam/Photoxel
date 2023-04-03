#pragma once

#include <string>
#include <vector>
#include <unordered_set>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "Filters.h"

namespace Photoxel {
	enum class ShaderType {
		None,
		Vertex,
		Pixel
	};

	struct ShaderProperties {
		std::string Filepath;
		ShaderType Type;
	};

	class Shader
	{
	public:
		Shader(std::initializer_list<ShaderProperties> properties);
		~Shader();

		virtual void Bind() const;

		virtual void RecreateShader(std::initializer_list<ShaderProperties> properties,
			std::unordered_set<Filter> filtersApply);

		void SetInt(const std::string& name, int value);
		void SetFloat(const std::string& name, float value);
		void SetMat4(const std::string& name, const glm::mat4& value);

		void Kill();
	private:
		uint32_t CreateShader(ShaderProperties properties);
		uint32_t CreateShader(ShaderProperties properties, std::unordered_set<Filter> filters);
		std::string GetShaderCode(const std::string& filepath);
		uint32_t m_RendererID;
		std::string m_VertexFilepath = "", m_FragmentFilepath = "";
	};
}