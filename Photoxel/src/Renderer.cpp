#include "Renderer.h"
#include <iostream>
#include <glad/glad.h>
#include <GLFW/glfw3.h>

namespace Photoxel
{
	Renderer::Renderer()
	{
		if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
		{
			return;
		}

        std::cout << glGetString(GL_VENDOR) << std::endl;
        std::cout << glGetString(GL_RENDERER) << std::endl;
        std::cout << glGetString(GL_VERSION) << std::endl;

        m_Shader = new Shader({
            { "VertexShader", Photoxel::ShaderType::Vertex },
            { "PixelShader", Photoxel::ShaderType::Pixel }
        });

        struct Data {
            glm::vec4 Position;
            glm::vec2 TexCoords;
            int ID;
        };
        Data data[] = {
            { {  300.0f,  300.0f, 0.0f, 1.0f }, { 1.0f, 1.0f }, 3 },
            { {  300.0f, -300.0f, 0.0f, 1.0f }, { 1.0f, 0.0f }, 3 },
            { { -300.0f, -300.0f, 0.0f, 1.0f }, { 0.0f, 0.0f }, 3 },
            { { -300.0f,  300.0f, 0.0f, 1.0f }, { 0.0f, 1.0f }, 3 }
        };
        unsigned int indices[] = {  // note that we start from 0!
            0, 1, 3,  // first Triangle
            1, 2, 3   // second Triangle
        };
        unsigned int VBO, EBO;
        glGenVertexArrays(1, &m_VertexArray);
        glCreateBuffers(1, &VBO);
        glCreateBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(m_VertexArray);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(data), data, GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)offsetof(Data, Position));

        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Data), (void*)offsetof(Data, TexCoords));

        glEnableVertexAttribArray(2);
        glVertexAttribIPointer(2, 1, GL_INT, sizeof(Data), (void*)offsetof(Data, ID));

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        glBindVertexArray(0);
	}

    Renderer::~Renderer()
    {
        delete m_Shader;
    }

    void Renderer::OnRender()
    {
        // draw our first triangle
        m_Shader->Bind();
        m_Shader->SetMat4("u_Projection", m_Projection);
        m_Shader->SetMat4("u_View", m_View);
        m_Shader->SetMat4("u_Model", m_Model);
        m_Shader->SetInt("u_Texture", 0);

        glBindVertexArray(m_VertexArray); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        //glDrawArrays(GL_TRIANGLES, 0, 6);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
    }

    void Renderer::BeginScene(const glm::mat4& projection, const glm::mat4& view, const glm::mat4& model)
    {
        m_Projection = projection;
        m_View = view;
        m_Model = model;

        glClearColor(0.15f, 0.15f, 0.15f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
}