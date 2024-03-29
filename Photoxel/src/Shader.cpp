#include "Shader.h"
#include <glad/glad.h>
#if 1
#include<iostream>
#endif
#include <fstream>
#include <filesystem>

namespace Photoxel
{
	static GLenum PhotoxelToGLShaderType(ShaderType type)
	{
		switch (type)
		{
			case ShaderType::Vertex:		return GL_VERTEX_SHADER;
			case ShaderType::Pixel:			return GL_FRAGMENT_SHADER;
		}

		return 0;
	}

	Shader::Shader(std::initializer_list<ShaderProperties> properties)
	{
		m_RendererID = glCreateProgram();
		std::vector<GLuint> shaders;
		shaders.reserve(properties.size());
		for (auto& shaderProperty : properties)
		{
			GLuint shader = CreateShader(shaderProperty);
			glAttachShader(m_RendererID, shader);
			shaders.emplace_back(shader);
		}

		glLinkProgram(m_RendererID);
		glValidateProgram(m_RendererID);

		for (auto& shader : shaders)
		{
			glDeleteShader(shader);
		}

		glUseProgram(m_RendererID);
	}

	Shader::~Shader()
	{
		glDeleteProgram(m_RendererID);
	}

	void Shader::Bind() const
	{
		glUseProgram(m_RendererID);
	}

	void Shader::RecreateShader(std::initializer_list<ShaderProperties> properties, std::unordered_set<Filter> filtersApply)
	{
		Kill();
		m_RendererID = glCreateProgram();
		std::vector<GLuint> shaders;
		shaders.reserve(properties.size());
		for (auto& shaderProperty : properties)
		{
			GLuint shader = CreateShader(shaderProperty, filtersApply);
			glAttachShader(m_RendererID, shader);
			shaders.emplace_back(shader);
		}

		glLinkProgram(m_RendererID);
		glValidateProgram(m_RendererID);

		for (auto& shader : shaders)
		{
			glDeleteShader(shader);
		}

		glUseProgram(m_RendererID);
	}

	void Shader::SetInt(const std::string& name, int value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1i(location, value);
	}

	void Shader::SetFloat(const std::string& name, float value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform1f(location, value);
	}

	void Shader::SetMat4(const std::string& name, const glm::mat4& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
	}

	void Shader::SetFloat3(const std::string& name, const glm::vec3& value)
	{
		GLint location = glGetUniformLocation(m_RendererID, name.c_str());
		glUniform3f(location, value.x, value.y, value.z);
	}

	void Shader::Kill()
	{
		glDeleteProgram(m_RendererID);
	}

	uint32_t Shader::CreateShader(ShaderProperties properties)
	{
		GLuint shader = glCreateShader(PhotoxelToGLShaderType(properties.Type));
		std::string codeStr = GetShaderCode(properties.Filepath).c_str();
		const char* code = codeStr.c_str();
		glShaderSource(shader, 1, &code, nullptr);
		glCompileShader(shader);

#if 1
		int sucess;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &sucess);
		if (!sucess) {
			int length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> infoLog(length);
			glGetShaderInfoLog(shader, 512, nullptr, infoLog.data());
			std::cout << "Error de compilacion: " << std::string(infoLog.begin(), infoLog.end()) << '\n';
		}
#endif

		return shader;
	}

	uint32_t Shader::CreateShader(ShaderProperties properties, std::unordered_set<Filter> filters)
	{
		GLuint shader = glCreateShader(PhotoxelToGLShaderType(properties.Type));
		std::string codeStr = GetShaderCode(properties.Filepath).c_str();

		std::string headerCode = "";
		std::string mainCode = "";
		if (properties.Type == ShaderType::Pixel)
		{
			for (auto& filter : filters)
			{
				switch (filter)
				{
					case Filter::Grayscale:
					{
						mainCode += "o_FragColor = grayscale(o_FragColor);\n";
						break;
					}
					case Filter::Negative:
					{
						mainCode += "o_FragColor = negative(o_FragColor);\n";
						break;
					}
					case Filter::Sepia:
					{
						mainCode += "o_FragColor = sepia(o_FragColor);\n";
						break;
					}
					case Filter::Brightness:
					{
						headerCode += "uniform float u_Brightness;\n";
						mainCode += "o_FragColor = brightness(o_FragColor, u_Brightness);\n";
						break;
					}
					case Filter::Contrast:
					{
						headerCode += "uniform float u_Contrast;\n";
						mainCode += "o_FragColor = contrast(o_FragColor, u_Contrast);\n";
						break;
					}
					case Filter::Binary:
					{
						headerCode += "uniform float u_Thresehold;\n";
						mainCode += "o_FragColor = binary(o_FragColor, u_Thresehold);\n";
						break;
					}
					case Filter::EdgeDetection:
					{
						headerCode += "uniform int u_Width;\n";
						headerCode += "uniform int u_Height;\n";
						mainCode += "o_FragColor = edgeDetection(u_Width, u_Height);\n";
						break;
					}
					case Filter::Pixelate:
					{
						headerCode += "uniform int u_Mosaic;\n";
						headerCode += "uniform int u_MosaicWidth;\n";
						headerCode += "uniform int u_MosaicHeight;\n";
						mainCode += "o_FragColor = mosaic(u_Mosaic, u_MosaicWidth, u_MosaicHeight);\n";
						break;
					}
					case Filter::Gradient:
					{
						headerCode += "uniform vec3 u_StartColour;\n";
						headerCode += "uniform vec3 u_EndColour;\n";
						headerCode += "uniform float u_Angle;\n";
						headerCode += "uniform float u_Intensity;\n";
						mainCode += "o_FragColor = gradient(o_FragColor, u_StartColour, u_EndColour, u_Angle, u_Intensity);\n";
						break;
					}
				}
			}

			codeStr.replace(codeStr.find("/*{{HEADER}}*/"), sizeof("/*{{HEADER}}*/") - 1, headerCode);
			codeStr.replace(codeStr.find("/*{{CONTENT}}*/"), sizeof("/*{{CONTENT}}*/") - 1, mainCode);
		}

		const char* code = codeStr.c_str();
		glShaderSource(shader, 1, &code, nullptr);
		glCompileShader(shader);

#if _DEBUG
		int sucess;
		glGetShaderiv(shader, GL_COMPILE_STATUS, &sucess);
		if (!sucess) {
			int length;
			glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
			std::vector<char> infoLog(length);
			glGetShaderInfoLog(shader, 512, nullptr, infoLog.data());
			std::cout << "Error de compilacion: " << std::string(infoLog.begin(), infoLog.end()) << '\n';
		}
#endif

		return shader;
	}

	std::string Shader::GetShaderCode(const std::string& filepath)
	{
		std::ifstream reader(filepath + ".glsl");

		std::string code;
		std::string line;
		while (!reader.eof())
		{
			std::getline(reader, line);
			code += line;
			code += '\n';
		}

		return code;
	}
}