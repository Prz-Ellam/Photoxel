#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "ImGui.h"
#include "ImGuiWindow.h"
#include "Image.h"
#include "FileDialog.h"
#include "imgui_internals.h"
#include <iostream>
#include "IconsFontAwesome5.h"
#include "ColorGenerator.h"
#include <stb_image_write.h>
#include <stb_image_resize.h>
#include <iomanip>
#include <sstream>

#define WIDTH 1280
#define HEIGHT 720

namespace Photoxel
{
	std::string formatTime(int segundos) {
		int horas = segundos / 3600;
		int minutos = (segundos % 3600) / 60;
		int segundosRestantes = (segundos % 3600) % 60;

		std::ostringstream oss;
		oss << std::setfill('0') << std::setw(2) << horas << ":"
			<< std::setfill('0') << std::setw(2) << minutos << ":"
			<< std::setfill('0') << std::setw(2) << segundosRestantes;

		return oss.str();
	}

	Application::Application()
		: m_Running(true)
	{
		//std::cout << std::filesystem::current_path() << std::endl;
		m_Window = std::make_shared<Window>();
		m_Window->SetWindowCloseCallback([&]() {
			m_Running = false;
		});
		m_Window->SetIcons("logo.png");
		m_Renderer = std::make_shared<Renderer>();
		m_ViewportFramebuffer = std::make_shared<Framebuffer>(1280u, 720u);

		m_GuiLayer = std::make_shared<ImGuiLayer>(m_Window);
		m_GuiWindow = std::make_shared<ImGuiWindow>("Viewport", false);

		const int data = -16777216;
		m_VideoFrame = std::make_shared<Image>(1, 1, &data);
		m_Camera = std::make_shared<Photoxel::Image>(1, 1, &data);

		m_FilterMap = {
			{ "Negative", Filter::Negative },
			{ "Grayscale", Filter::Grayscale },
			{ "Sepia", Filter::Sepia },
			{ "Brightness", Filter::Brightness },
			{ "Contrast", Filter::Contrast },
			{ "Edge Detection", Filter::EdgeDetection },
			{ "Binary", Filter::Binary },
			{ "Gradient", Filter::Gradient },
			{ "Pixelate", Filter::Pixelate }
		};

		if (!m_CascadeClassifier.load("haarcascade_frontalface_alt.xml"))
		{
			std::cout << "Error con los cascade classifier" << std::endl;
		}
	}

	void Application::Run() {
		mySequence.mFrameMin = 0;
		mySequence.mFrameMax = 0;
		mySequence.myItems.push_back(MySequence::MySequenceItem{ 0, 0, 10, true });

		while (m_Running) {
			if (m_Image)
			{
				m_ViewportFramebuffer->Resize(m_Image->GetWidth(), m_Image->GetHeight());
			}
			m_ViewportFramebuffer->Begin();

			if (m_Video) {
				int status = m_Video->Read();
				uint8_t* buffer = m_Video->GetFrame();
				if (status == 2) continue;
				//if (!buffer) continue;
				m_VideoFrame->SetData2(m_Video->GetWidth(), m_Video->GetHeight(), buffer);
			}

			m_Renderer->BeginScene();
			m_ViewportFramebuffer->ClearAttachment();

			switch (m_SectionFocus) {
				case IMAGE:
					if (m_Image) {
						m_Image->Bind();
						m_Renderer->BindImage();
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat("u_Brightness", m_Brightness);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat("u_Contrast", m_Contrast);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat("u_Thresehold", m_Thresehold);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetInt("u_Width", m_Image->GetWidth());
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetInt("u_Height", m_Image->GetHeight());
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetInt("u_Mosaic", m_Mosaic);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetInt("u_MosaicWidth", m_Image->GetWidth());
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetInt("u_MosaicHeight", m_Image->GetHeight());
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat3("u_StartColour", m_StartColour);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat3("u_EndColour", m_EndColour);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat("u_Angle", m_Angle);
						dynamic_cast<Shader*>(m_Renderer->GetShader())->SetFloat("u_Intensity", m_Intensity);
					}
					break;
				case VIDEO:
					m_VideoFrame->Bind();
					m_Renderer->BindVideo();
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat("u_Brightness", m_VideoBrightness);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat("u_Contrast", m_VideoContrast);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat("u_Thresehold", m_VideoThresehold);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetInt("u_Width", m_VideoFrame->GetWidth());
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetInt("u_Height", m_VideoFrame->GetHeight());
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetInt("u_Mosaic", m_VideoMosaic);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetInt("u_MosaicWidth", m_VideoFrame->GetWidth());
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetInt("u_MosaicHeight", m_VideoFrame->GetHeight());
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat3("u_StartColour", m_VideoStartColour);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat3("u_EndColour", m_VideoEndColour);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat("u_Angle", m_VideoAngle);
					dynamic_cast<Shader*>(m_Renderer->GetShaderVideo())->SetFloat("u_Intensity", m_VideoIntensity);
					break;
			}

			m_Renderer->OnRender();

			if (m_HistogramHasUpdate)
			{
				m_HistogramHasUpdate = false;
				std::vector<uint8_t>& data = m_ViewportFramebuffer->GetData();

				int width = m_ViewportFramebuffer->GetWidth();
				int height = m_ViewportFramebuffer->GetHeight();

				int pixelCount = width * height;
				int byteCount = pixelCount * 4;

				red.clear();
				green.clear();
				blue.clear();

				red.reserve(pixelCount);
				green.reserve(pixelCount);
				blue.reserve(pixelCount);

				for (int i = 0; i < byteCount; i += 4) {
					red.emplace_back((float)data[i]);
					green.emplace_back((float)data[i + 1]);
					blue.emplace_back((float)data[i + 2]);
				}
			}

			m_GuiLayer->Begin();

			RenderMenuBar();
			RenderCameraTab();
			RenderVideoTab();
			RenderImageTab();
			RenderThemeWindow();

			m_ViewportFramebuffer->End();
			m_GuiLayer->End();
			m_Window->Update();
		}
	}

	void Application::RenderMenuBar()
	{
		if (ImGui::BeginMenuBar())
		{
			if (ImGui::BeginMenu("File"))
			{
				if (ImGui::MenuItem(ICON_FA_FILE"\t New File")) {
					switch (m_SectionFocus) {
						case IMAGE: {
							std::string filepath = FileDialog::OpenFile(*m_Window.get(), "Image Files (*.png, *.jpg)|*.png;*.jpg|");
							if (filepath == "") break;
							m_Image = std::make_shared<Image>(filepath.c_str());
							m_HistogramHasUpdate = true;
							break;
						}
						case VIDEO: {
							std::string filepath = FileDialog::OpenFile(*m_Window.get(), "Video Files (*.mp4)|*.mp4|");
							if (filepath == "") break;
							m_Video = std::make_shared<Video>(filepath);
							mySequence.mFrameMax = m_Video->GetDuration();
							break;
						}
					}
				}

				if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN"\tOpen File")) {
					switch (m_SectionFocus) {
						case IMAGE: {
							std::string filepath = FileDialog::OpenFile(*m_Window.get(), "Image Files (*.png, *.jpg)|*.png;*.jpg|");
							if (filepath == "") break;
							m_Image = std::make_shared<Image>(filepath.c_str());
							m_HistogramHasUpdate = true;
							break;
						}
						case VIDEO: {
							std::string filepath = FileDialog::OpenFile(*m_Window.get(), "Video Files (*.mp4)|*.mp4|");
							if (filepath == "") break;
							m_Video = std::make_shared<Video>(filepath);
							mySequence.mFrameMax = m_Video->GetDuration();
							break;
						}
					}
					
				}

				ImGui::Separator();

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File")) {
					if (m_SectionFocus == IMAGE && m_Image) {
						std::string filepath = FileDialog::SaveFile(*m_Window.get(), "(.jpg)\0*.jpg\0(.png)\0*.png");
						std::vector<uint8_t> data = m_ViewportFramebuffer->GetData();
						stbi_write_png(filepath.c_str(), m_Image->GetWidth(),
							m_Image->GetHeight(), 4, data.data(), m_Image->GetWidth() * 4);
					}
				}

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File As...")) {
					if (m_SectionFocus == IMAGE && m_Image) {
						std::string filepath = FileDialog::SaveFile(*m_Window.get(), "(.jpg)\0*.jpg\0(.png)\0*.png");
						std::vector<uint8_t> data = m_ViewportFramebuffer->GetData();
						stbi_write_png(filepath.c_str(), m_Image->GetWidth(),
							m_Image->GetHeight(), 4, data.data(), m_Image->GetWidth() * 4);
					}
				}
				
				ImGui::Separator();

				if (ImGui::MenuItem(ICON_FA_DOOR_OPEN"\tExit", "Alt+F4"))
					Close();

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window")) {
				if (ImGui::MenuItem(ICON_FA_BRUSH"\tChange theme")) {
					m_IsThemeOpen = true;
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem(ICON_FA_BOOK"\tUser Manual")) {
					ShellExecuteA(GetDesktopWindow(), "open", "Photoxel.pdf", NULL, NULL, SW_SHOWNORMAL);
				}
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}
	}

	void Application::RenderImageTab()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(ICON_FA_IMAGE" Images");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}
		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Histogram");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}
		ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		ImGui::PlotHistogramColour("HistogramRed", red.data(), red.size(), 0, NULL, 0.0f, 255.0f, ImVec2(viewportSize.x, viewportSize.y / 3.2), 4, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
		ImGui::PlotHistogramColour("HistogramGreen", green.data(), green.size(), 0, NULL, 0.0f, 255.0f, ImVec2(viewportSize.x, viewportSize.y / 3.2), 4, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
		ImGui::PlotHistogramColour("HistogramBlue", blue.data(), blue.size(), 0, NULL, 0.0f, 255.0f, ImVec2(viewportSize.x, viewportSize.y / 3.2), 4, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_HorizontalScrollbar);
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}

		if (m_Image)
		{
			const ImVec2 windowSize = ImGui::GetWindowSize();
			const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
			const float navbarHeight = windowSize.y - viewportSize.y;

			float widthScale = viewportSize.x / m_Image->GetWidth();
			float heightScale = viewportSize.y / m_Image->GetHeight();
			float minScale = glm::min(widthScale, heightScale);
			glm::vec2 scaleImageSize = glm::vec2(m_Image->GetWidth(), m_Image->GetHeight()) * minScale;
			scaleImageSize *= m_ImageScale;

			ImGui::SetCursorPosX(viewportSize.x / 2 - scaleImageSize.x / 2);
			ImGui::SetCursorPosY((viewportSize.y / 2 - scaleImageSize.y / 2) + navbarHeight);

			ImGui::Image(
				(ImTextureID)m_ViewportFramebuffer->GetColorAttachment(),
				ImVec2(scaleImageSize.x, scaleImageSize.y)
			);
		}

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Tree");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}

		if (ImGui::TreeNode("Eliminar filtros"))
		{
			for (auto& [name, filter] : m_FilterMap) {
				if (m_ImageFilters.find(filter) != m_ImageFilters.end()) {
					if (ImGui::Button(name.c_str())) {
						m_ImageFilters.erase(filter);
						UpdateImageInfo();
					}
				}
			}
			ImGui::TreePop();
		}

		if (m_ImageFilters.find(Filter::Brightness) != m_ImageFilters.end()) {
			
			float buttonWidth = ImGui::GetFrameHeight();
			float contentWidth = ImGui::CalcTextSize("Brightness").x;

			float cursorPosX = ImGui::GetCursorPosX();

			ImGui::AlignTextToFramePadding();
			ImGui::Text("Brightness");
			ImGui::SameLine(cursorPosX + contentWidth + ImGui::GetStyle().ItemSpacing.x);

			ImGui::SetCursorPosX(ImGui::GetWindowWidth() - buttonWidth - 10);
			auto& colors = ImGui::GetStyle().Colors;

			ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8627f, 0.2078f, 0.2706f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.9333f, 0.2353f, 0.2902f, 1.0f));
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7529f, 0.0196f, 0.0824f, 1.0f));
			if (ImGui::Button(ICON_FA_TIMES)) {
				m_ImageFilters.erase(Filter::Brightness);
				UpdateImageInfo();
			}
			ImGui::PopStyleColor(3);


			if (ImGui::SliderFloat("Brightness", &m_Brightness, 0.0f, 2.0f)) {
				m_HistogramHasUpdate = true;
			}

			ImGui::Separator();
		}
		if (m_ImageFilters.find(Filter::Contrast) != m_ImageFilters.end()) {
			if (ImGui::SliderFloat("Contrast", &m_Contrast, -1.0f, 1.0f)) {
				m_HistogramHasUpdate = true;
			}
		}
		if (m_ImageFilters.find(Filter::Binary) != m_ImageFilters.end()) {
			if (ImGui::SliderFloat("Thresehold", &m_Thresehold, 0.0f, 5.0f)) {
				m_HistogramHasUpdate = true;
			}
		}
		if (m_ImageFilters.find(Filter::Pixelate) != m_ImageFilters.end()) {
			if (ImGui::SliderInt("Mosaic", &m_Mosaic, 1, 100)) {
				m_HistogramHasUpdate = true;
			}
		}
		if (m_ImageFilters.find(Filter::Gradient) != m_ImageFilters.end()) {
			if (ImGui::ColorEdit3("Start Colour", glm::value_ptr(m_StartColour))) 
				m_HistogramHasUpdate = true;
			if (ImGui::ColorEdit3("End Colour", glm::value_ptr(m_EndColour))) 
				m_HistogramHasUpdate = true;
			if (ImGui::SliderFloat("Angle", &m_Angle, 0.0f, 360.0f)) 
				m_HistogramHasUpdate = true;
			if (ImGui::SliderFloat("Intensity", &m_Intensity, 0.0f, 1.0f)) 
				m_HistogramHasUpdate = true;
		}
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Effects");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}
		ImVec2 effectsSize = ImGui::GetContentRegionAvail();
		for (auto& [name, filter] : m_FilterMap)
		{
			if (ImGui::Button(name.c_str(), ImVec2(effectsSize.x, 30.0f))) {
				m_ImageFilters.insert(filter);
				UpdateImageInfo();
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::Begin("Stats");
		ImVec2 size2 = ImGui::GetContentRegionAvail();
		ImGui::PushTextWrapPos(ImGui::GetCursorPos().x + size2.x);
		if (m_Image) ImGui::Text("Image name: %s", m_Image->GetFilename());
		ImGui::PopTextWrapPos();
		if (m_Image) ImGui::Text("Image size: (%d x %d)", m_Image->GetWidth(), m_Image->GetHeight());
		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::SliderFloat("Zoom", &m_ImageScale, 0.0f, 5.0f, "%.2f", ImGuiSliderFlags_AlwaysClamp);
		
		if (ImGui::Button("Eliminar imagen")) {
			if (m_Image) {
				const int data = -16777216;
				m_Image->SetData(1, 1, &data);
				m_Image = nullptr;
				m_HistogramHasUpdate = true;
			}
		}
		
		ImGui::End();
	}

	void Application::RenderVideoTab()
	{
		if (m_Video && m_SectionFocus != VIDEO)
			m_Video->Pause();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(ICON_FA_VIDEO" Videos");

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}

		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Video Effects");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}
		ImVec2 effectsSize = ImGui::GetContentRegionAvail();
		for (auto& [name, filter] : m_FilterMap)
		{
			if (ImGui::Button(name.c_str(), ImVec2(effectsSize.x, 30.0f))) {
				m_VideoFilters.insert(filter);
				m_Renderer->GetShaderVideo()->RecreateShader({
					{ "VertexShader", Photoxel::ShaderType::Vertex },
					{ "PixelShader", Photoxel::ShaderType::Pixel }
					}, m_VideoFilters);
			}
		}
		ImGui::End();
		ImGui::PopStyleColor();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Video Viewport");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}

		const ImVec2 windowSize = ImGui::GetWindowSize();
		const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		const float navbarHeight = windowSize.y - viewportSize.y;

		float widthScale = viewportSize.x / m_VideoFrame->GetWidth();
		float heightScale = viewportSize.y / m_VideoFrame->GetHeight();
		float minScale = glm::min(widthScale, heightScale);
		glm::vec2 scaleImageSize = glm::vec2(m_VideoFrame->GetWidth(), m_VideoFrame->GetHeight()) * minScale;
		//scaleImageSize *= m_ImageScale;

		ImGui::SetCursorPosX(viewportSize.x / 2 - scaleImageSize.x / 2);
		ImGui::SetCursorPosY((viewportSize.y / 2 - scaleImageSize.y / 2) + navbarHeight);

		ImGui::Image(
			(ImTextureID)m_ViewportFramebuffer->GetColorAttachment(),
			ImVec2(scaleImageSize.x, scaleImageSize.y)
		);

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("TreeVideo");
		//ImGui::Text("Application (%.1f FPS)", ImGui::GetIO().Framerate);
		//if (m_Video)
		//ImGui::Text("%i", m_Video->GetCurrentSecond());

		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}

		if (ImGui::TreeNode("Eliminar filtros"))
		{
			for (auto& [name, filter] : m_FilterMap) {
				if (m_VideoFilters.find(filter) != m_VideoFilters.end()) {
					if (ImGui::Button(name.c_str())) {
						m_VideoFilters.erase(filter);
						m_Renderer->GetShaderVideo()->RecreateShader({
						{ "VertexShader", Photoxel::ShaderType::Vertex },
						{ "PixelShader", Photoxel::ShaderType::Pixel }
						}, m_VideoFilters);
					}
				}
			}
			ImGui::TreePop();
		}

		if (m_VideoFilters.find(Filter::Brightness) != m_VideoFilters.end()) {
			ImGui::SliderFloat("Brightness", &m_VideoBrightness, 0.0f, 2.0f);
		}
		if (m_VideoFilters.find(Filter::Contrast) != m_VideoFilters.end()) {
			ImGui::SliderFloat("Contrast", &m_VideoContrast, -1.0f, 1.0f);
		}
		if (m_VideoFilters.find(Filter::Binary) != m_VideoFilters.end()) {
			ImGui::SliderFloat("Thresehold", &m_VideoThresehold, 0.0f, 5.0f);
		}
		if (m_VideoFilters.find(Filter::Pixelate) != m_VideoFilters.end()) {
			ImGui::SliderInt("Mosaic", &m_VideoMosaic, 1, 100);
		}
		if (m_VideoFilters.find(Filter::Gradient) != m_VideoFilters.end()) {
			ImGui::ColorEdit3("Start Colour", glm::value_ptr(m_VideoStartColour));
			ImGui::ColorEdit3("End Colour", glm::value_ptr(m_VideoEndColour));
			ImGui::SliderFloat("Angle", &m_VideoAngle, 0.0f, 360.0f);
			ImGui::SliderFloat("Intensity", &m_VideoIntensity, 0.0f, 1.0f);
		}
		
		ImGui::End();

		static int selectedEntry = -1;
		static int firstFrame = 0;
		static bool expanded = true;
		static int currentFrame = 0;
		
		ImGui::Begin("Timeline");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}
		ImGui::Text(formatTime(currentFrame).c_str());
		ImGui::SameLine();
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (70 * 0.5f));
		if (ImGui::Button(ICON_FA_PAUSE, ImVec2(20, 0))) {
			if (m_Video)
				m_Video->Pause();
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Pause the video");
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			if (m_Video)
				m_Video->Resume();
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Play the video");
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, ImVec2(20, 0))) {
			const int data = -16777216;
			m_VideoFrame->SetData(1, 1, &data);
			m_Video = nullptr;
			currentFrame = 0;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Stop the video");
		}
		
		if (m_Video)
			currentFrame = m_Video->GetCurrentSecond();

		ImSequencer::Sequencer(&mySequence, &currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
		
		if (m_Video) {
			if (m_Video->GetCurrentSecond() != currentFrame) {
				m_Video->Seek(currentFrame);
			}
		}
		
		if (selectedEntry != -1)
		{
			const MySequence::MySequenceItem& item = mySequence.myItems[selectedEntry];
		}
		ImGui::End();
	}

	void Application::RenderCameraTab()
	{
		if (m_IsRecording && m_SectionFocus != CAMERA)
		{
			m_Capture2.StopCapture();
			const int data = -16777216;
			m_Camera->SetData(1, 1, &data);
			m_Faces.clear();
			m_IsRecording = false;
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(ICON_FA_CAMERA" Camera");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = CAMERA;
		}
		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Cameras");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = CAMERA;
		}
		static int item = 0;
		auto& names = m_Capture2.GetCaptureDeviceNames();
		ImGui::Combo("##", &item, names.data(), names.size());
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(22, 0))) {
			m_Capture2.StartCapture(item);
			m_IsRecording = true;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Turn on the selected camera");
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_POWER_OFF, ImVec2(22, 0))) {
			m_Capture2.StopCapture();
			const int data = -16777216;
			m_Camera->SetData(1, 1, &data);
			m_Faces.clear();
			m_IsRecording = false;
		}
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
		{
			ImGui::SetTooltip("Turn off the camera");
		}
		ImGui::End();

		static int a = 0;
		if (m_IsRecording) {
			uint8_t* data = m_Capture2.GetBuffer();
			uint32_t width = 1024;
			uint32_t height = 1024;

			cv::Mat frame(width, height, CV_8UC3, data);

			int newWidth = width >> 1;
			int newHeight = height >> 1;

			// Crear un objeto Mat para almacenar la imagen reescalada
			cv::Mat rescaleFrame;

			// Reescalar la imagen utilizando la función resize
			cv::resize(frame, rescaleFrame, cv::Size(640, 480));


			cv::Mat frameGray;
			cvtColor(rescaleFrame, frameGray, cv::COLOR_BGR2GRAY);
			equalizeHist(frameGray, frameGray);

			static int a = 0;
			if (a > 8) {
				m_CascadeClassifier.detectMultiScale(frameGray, m_Faces, 1.1, 10);
				a = 0;
			}
			else {
				a++;
			}

			double scaleWidth = static_cast<double>(frame.cols) / rescaleFrame.cols;
			double scaleHeight = static_cast<double>(frame.rows) / rescaleFrame.rows;

			int i = 0;
			for (const auto& face : m_Faces)
			{
				/*cv::Rect scaledRect(face.x * scaleWidth, face.y * scaleHeight,
					face.width * scaleWidth, face.height * scaleHeight);*/

				rectangle(rescaleFrame, face, GetBasicColor(i++), 2);
				cv::Mat faceROI = frameGray(face);
			}

			m_Camera->SetData(640, 480, rescaleFrame.data);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Camera Viewport");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = CAMERA;
		}

		const ImVec2 windowSize = ImGui::GetWindowSize();
		const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
		const float navbarHeight = windowSize.y - viewportSize.y;

		float widthScale = viewportSize.x / m_Camera->GetWidth();
		float heightScale = viewportSize.y / m_Camera->GetHeight();
		float minScale = glm::min(widthScale, heightScale);
		glm::vec2 scaleCameraSize = glm::vec2(m_Camera->GetWidth(), m_Camera->GetHeight()) * minScale;
		//scaleImageSize *= m_ImageScale;

		ImGui::SetCursorPosX(viewportSize.x / 2 - scaleCameraSize.x / 2);
		ImGui::SetCursorPosY((viewportSize.y / 2 - scaleCameraSize.y / 2) + navbarHeight);

		ImGui::Image(
			(ImTextureID)m_Camera->GetTextureID(),
			ImVec2(scaleCameraSize.x, scaleCameraSize.y)
		);

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Camera Stats");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = CAMERA;
		}
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		std::string persons = ICON_FA_SMILE + std::string(" Face count: ") + std::to_string(m_Faces.size());
		ImGui::Text(persons.c_str());
		ImGui::End();
	}
	
	void Application::RenderThemeWindow()
	{
		if (m_IsThemeOpen)
		{
			ImGui::Begin(ICON_FA_BRUSH" Themes", &m_IsThemeOpen);

			if (ImGui::Button("Dark"))
			{
					// Dark style by dougbinks from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.6000000238418579f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 0.0f;
					style.WindowBorderSize = 1.0f;
					style.WindowMinSize = ImVec2(32.0f, 32.0f);
					style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Left;
					style.ChildRounding = 0.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 0.0f;
					style.PopupBorderSize = 1.0f;
					style.FramePadding = ImVec2(4.0f, 3.0f);
					style.FrameRounding = 0.0f;
					style.FrameBorderSize = 0.0f;
					style.ItemSpacing = ImVec2(8.0f, 4.0f);
					style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
					style.CellPadding = ImVec2(4.0f, 2.0f);
					style.IndentSpacing = 21.0f;
					style.ColumnsMinSpacing = 6.0f;
					style.ScrollbarSize = 14.0f;
					style.ScrollbarRounding = 9.0f;
					style.GrabMinSize = 10.0f;
					style.GrabRounding = 0.0f;
					style.TabRounding = 4.0f;
					style.TabBorderSize = 0.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239215686917305f, 0.5176470875740051f, 0.8784313797950745f, 1.0f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
				
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);
					
					auto& colors = ImGui::GetStyle().Colors;
					colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

					// Headers
					colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
					colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
					colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

					// Buttons
					colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
					colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
					//colors[ImGuiCol_ButtonHovered] = ImVec4{ 160.0f / 255.0f,  32.0f / 255.0f, 240.0f / 255.0f, 1.0f };
					colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

					// Frame BG
					colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
					colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
					colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

					// Tabs
					colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
					colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
					colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
					colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
					colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

					// Title
					colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
					colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
					colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };


			}

			if (ImGui::Button("Light"))
			{
					// Light style by dougbinks from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.6000000238418579f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 0.0f;
					style.WindowBorderSize = 1.0f;
					style.WindowMinSize = ImVec2(32.0f, 32.0f);
					style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Left;
					style.ChildRounding = 0.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 0.0f;
					style.PopupBorderSize = 1.0f;
					style.FramePadding = ImVec2(4.0f, 3.0f);
					style.FrameRounding = 0.0f;
					style.FrameBorderSize = 0.0f;
					style.ItemSpacing = ImVec2(8.0f, 4.0f);
					style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
					style.CellPadding = ImVec2(4.0f, 2.0f);
					style.IndentSpacing = 21.0f;
					style.ColumnsMinSpacing = 6.0f;
					style.ScrollbarSize = 14.0f;
					style.ScrollbarRounding = 9.0f;
					style.GrabMinSize = 10.0f;
					style.GrabRounding = 0.0f;
					style.TabRounding = 4.0f;
					style.TabBorderSize = 0.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.9800000190734863f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.0f, 0.0f, 0.0f, 0.300000011920929f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.95686274766922f, 0.95686274766922f, 0.95686274766922f, 1.0f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.8196078538894653f, 0.8196078538894653f, 0.8196078538894653f, 1.0f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(1.0f, 1.0f, 1.0f, 0.5099999904632568f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.8588235378265381f, 0.8588235378265381f, 0.8588235378265381f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.9764705896377563f, 0.9764705896377563f, 0.9764705896377563f, 0.5299999713897705f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.686274528503418f, 0.686274528503418f, 0.686274528503418f, 0.800000011920929f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4862745106220245f, 0.4862745106220245f, 0.4862745106220245f, 0.800000011920929f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.4862745106220245f, 0.4862745106220245f, 0.4862745106220245f, 1.0f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.7799999713897705f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.4588235318660736f, 0.5372549295425415f, 0.800000011920929f, 0.6000000238418579f);
					style.Colors[ImGuiCol_Button] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3100000023841858f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 0.6200000047683716f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1372549086809158f, 0.4392156898975372f, 0.800000011920929f, 0.7799999713897705f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1372549086809158f, 0.4392156898975372f, 0.800000011920929f, 1.0f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 0.1700000017881393f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.7607843279838562f, 0.7960784435272217f, 0.8352941274642944f, 0.9309999942779541f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.5921568870544434f, 0.7254902124404907f, 0.8823529481887817f, 1.0f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.9176470637321472f, 0.9254902005195618f, 0.9333333373069763f, 0.9861999750137329f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.7411764860153198f, 0.8196078538894653f, 0.9137254953384399f, 1.0f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.4470588266849518f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.7764706015586853f, 0.8666666746139526f, 0.9764705896377563f, 1.0f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.5686274766921997f, 0.5686274766921997f, 0.6392157077789307f, 1.0f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.6784313917160034f, 0.6784313917160034f, 0.7372549176216125f, 1.0f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.2980392277240753f, 0.2980392277240753f, 0.2980392277240753f, 0.09000000357627869f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.699999988079071f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);

			}

			if (ImGui::Button("Deep Dark"))
			{
					// Deep Dark style by janekb04 from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.6000000238418579f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 7.0f;
					style.WindowBorderSize = 1.0f;
					style.WindowMinSize = ImVec2(32.0f, 32.0f);
					style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Left;
					style.ChildRounding = 4.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 4.0f;
					style.PopupBorderSize = 1.0f;
					style.FramePadding = ImVec2(5.0f, 2.0f);
					style.FrameRounding = 3.0f;
					style.FrameBorderSize = 1.0f;
					style.ItemSpacing = ImVec2(6.0f, 6.0f);
					style.ItemInnerSpacing = ImVec2(6.0f, 6.0f);
					style.CellPadding = ImVec2(6.0f, 6.0f);
					style.IndentSpacing = 25.0f;
					style.ColumnsMinSpacing = 6.0f;
					style.ScrollbarSize = 15.0f;
					style.ScrollbarRounding = 9.0f;
					style.GrabMinSize = 10.0f;
					style.GrabRounding = 3.0f;
					style.TabRounding = 4.0f;
					style.TabBorderSize = 1.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.1882352977991104f, 0.9200000166893005f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.1882352977991104f, 0.2899999916553497f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.239999994635582f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(0.0470588244497776f, 0.0470588244497776f, 0.0470588244497776f, 0.5400000214576721f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.1882352977991104f, 0.5400000214576721f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2274509817361832f, 1.0f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 1.0f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.0470588244497776f, 0.0470588244497776f, 0.5400000214576721f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3372549116611481f, 0.3372549116611481f, 0.3372549116611481f, 0.5400000214576721f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f, 0.5400000214576721f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5568627715110779f, 0.5568627715110779f, 0.5568627715110779f, 0.5400000214576721f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.3294117748737335f, 0.6666666865348816f, 0.8588235378265381f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3372549116611481f, 0.3372549116611481f, 0.3372549116611481f, 0.5400000214576721f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5568627715110779f, 0.5568627715110779f, 0.5568627715110779f, 0.5400000214576721f);
					style.Colors[ImGuiCol_Button] = ImVec4(0.0470588244497776f, 0.0470588244497776f, 0.0470588244497776f, 0.5400000214576721f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.1882352977991104f, 0.5400000214576721f);
					style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2274509817361832f, 1.0f);
					style.Colors[ImGuiCol_Header] = ImVec4(0.0f, 0.0f, 0.0f, 0.5199999809265137f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.0f, 0.0f, 0.0f, 0.3600000143051147f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2274509817361832f, 0.3300000131130219f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.2899999916553497f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.2899999916553497f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.4000000059604645f, 0.4392156898975372f, 0.4666666686534882f, 1.0f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.2899999916553497f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.2899999916553497f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4000000059604645f, 0.4392156898975372f, 0.4666666686534882f, 1.0f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.0f, 0.0f, 0.0f, 0.5199999809265137f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 0.3600000143051147f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.0f, 0.0f, 0.5199999809265137f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5199999809265137f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0f, 0.0f, 0.0f, 0.5199999809265137f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.2899999916553497f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2000000029802322f, 0.2196078449487686f, 0.2274509817361832f, 1.0f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.3294117748737335f, 0.6666666865348816f, 0.8588235378265381f, 1.0f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(1.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.0f, 0.0f, 0.699999988079071f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.0f, 0.0f, 0.0f, 0.70f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.70f);
			}

			if (ImGui::Button("Soft Dark"))
			{
					// Photoshop style by Derydoca from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.6000000238418579f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 4.0f;
					style.WindowBorderSize = 1.0f;
					style.WindowMinSize = ImVec2(32.0f, 32.0f);
					style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Left;
					style.ChildRounding = 4.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 2.0f;
					style.PopupBorderSize = 1.0f;
					style.FramePadding = ImVec2(4.0f, 3.0f);
					style.FrameRounding = 2.0f;
					style.FrameBorderSize = 1.0f;
					style.ItemSpacing = ImVec2(8.0f, 4.0f);
					style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
					style.CellPadding = ImVec2(4.0f, 2.0f);
					style.IndentSpacing = 21.0f;
					style.ColumnsMinSpacing = 6.0f;
					style.ScrollbarSize = 13.0f;
					style.ScrollbarRounding = 12.0f;
					style.GrabMinSize = 7.0f;
					style.GrabRounding = 0.0f;
					style.TabRounding = 0.0f;
					style.TabBorderSize = 1.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 1.0f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2000000029802322f, 0.2000000029802322f, 0.2000000029802322f, 1.0f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2784313857555389f, 0.2784313857555389f, 0.2784313857555389f, 1.0f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.1450980454683304f, 0.1450980454683304f, 0.1450980454683304f, 1.0f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.2745098173618317f, 0.2745098173618317f, 0.2745098173618317f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2980392277240753f, 0.2980392277240753f, 0.2980392277240753f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_Button] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
					style.Colors[ImGuiCol_ButtonActive] = ImVec4(1.0f, 1.0f, 1.0f, 0.3910000026226044f);
					style.Colors[ImGuiCol_Header] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.2627451121807098f, 0.2627451121807098f, 0.2627451121807098f, 1.0f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.3882353007793427f, 0.3882353007793427f, 0.3882353007793427f, 1.0f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(1.0f, 1.0f, 1.0f, 0.25f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(1.0f, 1.0f, 1.0f, 0.6700000166893005f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(0.3490196168422699f, 0.3490196168422699f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.09411764889955521f, 0.09411764889955521f, 0.09411764889955521f, 1.0f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1921568661928177f, 0.1921568661928177f, 0.1921568661928177f, 1.0f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.4666666686534882f, 0.4666666686534882f, 0.4666666686534882f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.5843137502670288f, 0.5843137502670288f, 0.5843137502670288f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.1560000032186508f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 0.3882353007793427f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.5860000252723694f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);

			}

			if (ImGui::Button("Confy"))
			{
					// Comfy style by Giuseppe from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.1000000014901161f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 10.0f;
					style.WindowBorderSize = 0.1f;
					style.WindowMinSize = ImVec2(30.0f, 30.0f);
					style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Right;
					style.ChildRounding = 5.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 10.0f;
					style.PopupBorderSize = 0.0f;
					style.FramePadding = ImVec2(4.0f, 3.0f);
					style.FrameRounding = 5.0f;
					style.FrameBorderSize = 0.0f;
					style.ItemSpacing = ImVec2(5.0f, 4.0f);
					style.ItemInnerSpacing = ImVec2(5.0f, 5.0f);
					style.CellPadding = ImVec2(4.0f, 2.0f);
					style.IndentSpacing = 5.0f;
					style.ColumnsMinSpacing = 5.0f;
					style.ScrollbarSize = 15.0f;
					style.ScrollbarRounding = 9.0f;
					style.GrabMinSize = 15.0f;
					style.GrabRounding = 5.0f;
					style.TabRounding = 5.0f;
					style.TabBorderSize = 0.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(1.0f, 1.0f, 1.0f, 0.3605149984359741f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.54935622215271f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.3803921639919281f, 0.4235294163227081f, 0.572549045085907f, 0.5490196347236633f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.09803921729326248f, 0.09803921729326248f, 0.09803921729326248f, 1.0f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.2588235437870026f, 0.2588235437870026f, 0.2588235437870026f, 0.0f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 0.0f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1568627506494522f, 0.1568627506494522f, 0.1568627506494522f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.2352941185235977f, 0.2352941185235977f, 0.2352941185235977f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_Button] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_Header] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.8156862854957581f, 0.772549033164978f, 0.9647058844566345f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0f, 0.4509803950786591f, 1.0f, 0.0f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 0.0f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.294117659330368f, 0.294117659330368f, 0.294117659330368f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.5490196347236633f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.4235294163227081f, 0.3803921639919281f, 0.572549045085907f, 0.2918455004692078f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.03433477878570557f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.7372549176216125f, 0.6941176652908325f, 0.886274516582489f, 0.5490196347236633f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.6196078658103943f, 0.5764706134796143f, 0.7686274647712708f, 0.5490196347236633f);

			}

			if (ImGui::Button("Cyberpunk"))
			{
				// Future Dark style by rewrking from ImThemes
				ImGuiStyle& style = ImGui::GetStyle();

				style.Alpha = 1.0f;
				style.DisabledAlpha = 1.0f;
				style.WindowPadding = ImVec2(12.0f, 12.0f);
				style.WindowRounding = 0.0f;
				style.WindowBorderSize = 1.0f;
				style.WindowMinSize = ImVec2(20.0f, 20.0f);
				style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
				style.WindowMenuButtonPosition = ImGuiDir_None;
				style.ChildRounding = 0.0f;
				style.ChildBorderSize = 1.0f;
				style.PopupRounding = 0.0f;
				style.PopupBorderSize = 1.0f;
				style.FramePadding = ImVec2(4.0f, 3.0f);
				style.FrameRounding = 0.0f;
				style.FrameBorderSize = 0.0f;
				style.ItemSpacing = ImVec2(12.0f, 6.0f);
				style.ItemInnerSpacing = ImVec2(6.0f, 3.0f);
				style.CellPadding = ImVec2(12.0f, 6.0f);
				style.IndentSpacing = 20.0f;
				style.ColumnsMinSpacing = 6.0f;
				style.ScrollbarSize = 12.0f;
				style.ScrollbarRounding = 0.0f;
				style.GrabMinSize = 12.0f;
				style.GrabRounding = 0.0f;
				style.TabRounding = 0.0f;
				style.TabBorderSize = 0.0f;
				style.TabMinWidthForCloseButton = 0.0f;
				style.ColorButtonPosition = ImGuiDir_Right;
				style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
				style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

				style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
				style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
				style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
				style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_CheckMark] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
				style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_Header] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
				style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_Separator] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
				style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
				style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
				style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
				style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_Tab] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
				style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
				style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
				style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f);
				style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
				style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
				style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
				style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
				style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
				style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
				style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
				style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
				style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);

			}
			
			if (ImGui::Button("Irreal"))
			{
					// Unreal style by dev0-1 from ImThemes
					ImGuiStyle& style = ImGui::GetStyle();

					style.Alpha = 1.0f;
					style.DisabledAlpha = 0.6000000238418579f;
					style.WindowPadding = ImVec2(8.0f, 8.0f);
					style.WindowRounding = 0.0f;
					style.WindowBorderSize = 1.0f;
					style.WindowMinSize = ImVec2(32.0f, 32.0f);
					style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
					style.WindowMenuButtonPosition = ImGuiDir_Left;
					style.ChildRounding = 0.0f;
					style.ChildBorderSize = 1.0f;
					style.PopupRounding = 0.0f;
					style.PopupBorderSize = 1.0f;
					style.FramePadding = ImVec2(4.0f, 3.0f);
					style.FrameRounding = 0.0f;
					style.FrameBorderSize = 0.0f;
					style.ItemSpacing = ImVec2(8.0f, 4.0f);
					style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
					style.CellPadding = ImVec2(4.0f, 2.0f);
					style.IndentSpacing = 21.0f;
					style.ColumnsMinSpacing = 6.0f;
					style.ScrollbarSize = 14.0f;
					style.ScrollbarRounding = 9.0f;
					style.GrabMinSize = 10.0f;
					style.GrabRounding = 0.0f;
					style.TabRounding = 4.0f;
					style.TabBorderSize = 0.0f;
					style.TabMinWidthForCloseButton = 0.0f;
					style.ColorButtonPosition = ImGuiDir_Right;
					style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
					style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

					style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
					style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
					style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
					style.Colors[ImGuiCol_ChildBg] = ImVec4(1.0f, 1.0f, 1.0f, 0.0f);
					style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
					style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
					style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_FrameBg] = ImVec4(0.2000000029802322f, 0.2078431397676468f, 0.2196078449487686f, 0.5400000214576721f);
					style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f, 0.4000000059604645f);
					style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.1764705926179886f, 0.1764705926179886f, 0.1764705926179886f, 0.6700000166893005f);
					style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
					style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.2862745225429535f, 0.2862745225429535f, 0.2862745225429535f, 1.0f);
					style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
					style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
					style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
					style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
					style.Colors[ImGuiCol_CheckMark] = ImVec4(0.9372549057006836f, 0.9372549057006836f, 0.9372549057006836f, 1.0f);
					style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
					style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.8588235378265381f, 0.8588235378265381f, 0.8588235378265381f, 1.0f);
					style.Colors[ImGuiCol_Button] = ImVec4(0.4392156898975372f, 0.4392156898975372f, 0.4392156898975372f, 0.4000000059604645f);
					style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.4588235318660736f, 0.4666666686534882f, 0.47843137383461f, 1.0f);
					style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.4196078479290009f, 0.4196078479290009f, 0.4196078479290009f, 1.0f);
					style.Colors[ImGuiCol_Header] = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.3100000023841858f);
					style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.6980392336845398f, 0.6980392336845398f, 0.6980392336845398f, 0.800000011920929f);
					style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.47843137383461f, 0.4980392158031464f, 0.5176470875740051f, 1.0f);
					style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
					style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.7176470756530762f, 0.7176470756530762f, 0.7176470756530762f, 0.7799999713897705f);
					style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
					style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.9098039269447327f, 0.9098039269447327f, 0.9098039269447327f, 0.25f);
					style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.8078431487083435f, 0.8078431487083435f, 0.8078431487083435f, 0.6700000166893005f);
					style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.4588235318660736f, 0.4588235318660736f, 0.4588235318660736f, 0.949999988079071f);
					style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
					style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
					style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
					style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
					style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
					style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
					style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.729411780834198f, 0.6000000238418579f, 0.1490196138620377f, 1.0f);
					style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
					style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
					style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
					style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
					style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
					style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
					style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.8666666746139526f, 0.8666666746139526f, 0.8666666746139526f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
					style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.6000000238418579f, 0.6000000238418579f, 0.6000000238418579f, 1.0f);
					style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
					style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
					style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);
					style.Colors[ImGuiCol_DockingPreview] = ImVec4(0.26f, 0.59f, 0.98f, 0.70f);
					style.Colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.20f, 0.20f, 0.20f, 0.70f);

			}

			if (ImGui::Button("Dracula"))
			{
				ImGuiStyle& style = ImGui::GetStyle();
				style.Alpha = 1.0f;
				style.DisabledAlpha = 1.0f;
				style.WindowPadding = ImVec2(8.0f, 8.0f);
				style.WindowRounding = 9.0f;
				style.WindowBorderSize = 1.0f;
				style.WindowMinSize = ImVec2(32.0f, 32.0f);
				style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
				style.WindowMenuButtonPosition = ImGuiDir_Left;
				style.ChildRounding = 0.0f;
				style.ChildBorderSize = 1.0f;
				style.PopupRounding = 0.0f;
				style.PopupBorderSize = 1.0f;
				style.FramePadding = ImVec2(4.0f, 3.0f);
				style.FrameRounding = 0.0f;
				style.FrameBorderSize = 0.0f;
				style.ItemSpacing = ImVec2(8.0f, 4.0f);
				style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
				style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
				style.IndentSpacing = 21.0f;
				style.ColumnsMinSpacing = 6.0f;
				style.ScrollbarSize = 16.0f;
				style.ScrollbarRounding = 9.0f;
				style.GrabMinSize = 10.0f;
				style.GrabRounding = 0.0f;
				style.TabRounding = 4.0f;
				style.TabBorderSize = 0.0f;
				style.TabMinWidthForCloseButton = 0.0f;
				style.ColorButtonPosition = ImGuiDir_Right;
				style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
				style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

				style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
				style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
				style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
				style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
				style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.2862745225429535f, 0.47843137383461f, 0.5400000214576721f);
				style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
				style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
				style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
				style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1568627506494522f, 0.2862745225429535f, 0.47843137383461f, 1.0f);
				style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
				style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
				style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
				style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239215686917305f, 0.5176470875740051f, 0.8784313797950745f, 1.0f);
				style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Button] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
				style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3100000023841858f);
				style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
				style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
				style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
				style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
				style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
				style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
				style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
				style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
				style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
				style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
				style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
				style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
				style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
				style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
				style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
				style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
				style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
				style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
				style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
				style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
				style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
				style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
				style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);

				auto& colors = ImGui::GetStyle().Colors;
					colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
					colors[ImGuiCol_MenuBarBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

					// Border
					colors[ImGuiCol_Border] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
					colors[ImGuiCol_BorderShadow] = ImVec4{ 0.0f, 0.0f, 0.0f, 0.24f };

					// Text
					colors[ImGuiCol_Text] = ImVec4{ 1.0f, 1.0f, 1.0f, 1.0f };
					colors[ImGuiCol_TextDisabled] = ImVec4{ 0.5f, 0.5f, 0.5f, 1.0f };

					// Headers
					colors[ImGuiCol_Header] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
					colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
					colors[ImGuiCol_HeaderActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

					// Buttons
					colors[ImGuiCol_Button] = ImVec4{ 0.13f, 0.13f, 0.17, 1.0f };
					colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
					colors[ImGuiCol_ButtonActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_CheckMark] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };

					// Popups
					colors[ImGuiCol_PopupBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 0.92f };

					// Slider
					colors[ImGuiCol_SliderGrab] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.54f };
					colors[ImGuiCol_SliderGrabActive] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.54f };

					// Frame BG
					colors[ImGuiCol_FrameBg] = ImVec4{ 0.13f, 0.13, 0.17, 1.0f };
					colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
					colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

					// Tabs
					colors[ImGuiCol_Tab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_TabHovered] = ImVec4{ 0.24, 0.24f, 0.32f, 1.0f };
					colors[ImGuiCol_TabActive] = ImVec4{ 0.2f, 0.22f, 0.27f, 1.0f };
					colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

					// Title
					colors[ImGuiCol_TitleBg] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };

					// Scrollbar
					colors[ImGuiCol_ScrollbarBg] = ImVec4{ 0.1f, 0.1f, 0.13f, 1.0f };
					colors[ImGuiCol_ScrollbarGrab] = ImVec4{ 0.16f, 0.16f, 0.21f, 1.0f };
					colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4{ 0.19f, 0.2f, 0.25f, 1.0f };
					colors[ImGuiCol_ScrollbarGrabActive] = ImVec4{ 0.24f, 0.24f, 0.32f, 1.0f };

					// Seperator
					colors[ImGuiCol_Separator] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };
					colors[ImGuiCol_SeparatorHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 1.0f };
					colors[ImGuiCol_SeparatorActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 1.0f };

					// Resize Grip
					colors[ImGuiCol_ResizeGrip] = ImVec4{ 0.44f, 0.37f, 0.61f, 0.29f };
					colors[ImGuiCol_ResizeGripHovered] = ImVec4{ 0.74f, 0.58f, 0.98f, 0.29f };
					colors[ImGuiCol_ResizeGripActive] = ImVec4{ 0.84f, 0.58f, 1.0f, 0.29f };

					// Docking
					colors[ImGuiCol_DockingPreview] = ImVec4{ 0.44f, 0.37f, 0.61f, 1.0f };

					style.TabRounding = 4;
					style.ScrollbarRounding = 9;
					style.WindowRounding = 7;
					style.GrabRounding = 3;
					style.FrameRounding = 3;
					style.PopupRounding = 4;
					style.ChildRounding = 4;
			}

			if (ImGui::Button("Green"))
			{
				ImGuiStyle& style = ImGui::GetStyle();
				style.Alpha = 1.0f;
				style.DisabledAlpha = 1.0f;
				style.WindowPadding = ImVec2(8.0f, 8.0f);
				style.WindowRounding = 9.0f;
				style.WindowBorderSize = 1.0f;
				style.WindowMinSize = ImVec2(32.0f, 32.0f);
				style.WindowTitleAlign = ImVec2(0.0f, 0.5f);
				style.WindowMenuButtonPosition = ImGuiDir_Left;
				style.ChildRounding = 0.0f;
				style.ChildBorderSize = 1.0f;
				style.PopupRounding = 0.0f;
				style.PopupBorderSize = 1.0f;
				style.FramePadding = ImVec2(4.0f, 3.0f);
				style.FrameRounding = 0.0f;
				style.FrameBorderSize = 0.0f;
				style.ItemSpacing = ImVec2(8.0f, 4.0f);
				style.ItemInnerSpacing = ImVec2(4.0f, 4.0f);
				style.TouchExtraPadding = ImVec2(0.0f, 0.0f);
				style.IndentSpacing = 21.0f;
				style.ColumnsMinSpacing = 6.0f;
				style.ScrollbarSize = 16.0f;
				style.ScrollbarRounding = 9.0f;
				style.GrabMinSize = 10.0f;
				style.GrabRounding = 0.0f;
				style.TabRounding = 4.0f;
				style.TabBorderSize = 0.0f;
				style.TabMinWidthForCloseButton = 0.0f;
				style.ColorButtonPosition = ImGuiDir_Right;
				style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
				style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

				style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
				style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.4980392158031464f, 0.4980392158031464f, 0.4980392158031464f, 1.0f);
				style.Colors[ImGuiCol_WindowBg] = ImVec4(0.05882352963089943f, 0.05882352963089943f, 0.05882352963089943f, 0.9399999976158142f);
				style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.0784313753247261f, 0.0784313753247261f, 0.9399999976158142f);
				style.Colors[ImGuiCol_Border] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
				style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1568627506494522f, 0.2862745225429535f, 0.47843137383461f, 0.5400000214576721f);
				style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
				style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
				style.Colors[ImGuiCol_TitleBg] = ImVec4(0.03921568766236305f, 0.03921568766236305f, 0.03921568766236305f, 1.0f);
				style.Colors[ImGuiCol_TitleBgActive] = ImVec4(0.1568627506494522f, 0.2862745225429535f, 0.47843137383461f, 1.0f);
				style.Colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.0f, 0.0f, 0.0f, 0.5099999904632568f);
				style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.1372549086809158f, 0.1372549086809158f, 0.1372549086809158f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.01960784383118153f, 0.01960784383118153f, 0.01960784383118153f, 0.5299999713897705f);
				style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3098039329051971f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.407843142747879f, 0.407843142747879f, 0.407843142747879f, 1.0f);
				style.Colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.5098039507865906f, 0.5098039507865906f, 0.5098039507865906f, 1.0f);
				style.Colors[ImGuiCol_CheckMark] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.239215686917305f, 0.5176470875740051f, 0.8784313797950745f, 1.0f);
				style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Button] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.4000000059604645f);
				style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.05882352963089943f, 0.529411792755127f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Header] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3100000023841858f);
				style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
				style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_Separator] = ImVec4(0.4274509847164154f, 0.4274509847164154f, 0.4980392158031464f, 0.5f);
				style.Colors[ImGuiCol_SeparatorHovered] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 0.7799999713897705f);
				style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.09803921729326248f, 0.4000000059604645f, 0.7490196228027344f, 1.0f);
				style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.2000000029802322f);
				style.Colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.6700000166893005f);
				style.Colors[ImGuiCol_ResizeGripActive] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.949999988079071f);
				style.Colors[ImGuiCol_Tab] = ImVec4(0.1764705926179886f, 0.3490196168422699f, 0.5764706134796143f, 0.8619999885559082f);
				style.Colors[ImGuiCol_TabHovered] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.800000011920929f);
				style.Colors[ImGuiCol_TabActive] = ImVec4(0.196078434586525f, 0.407843142747879f, 0.6784313917160034f, 1.0f);
				style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.06666667014360428f, 0.1019607856869698f, 0.1450980454683304f, 0.9724000096321106f);
				style.Colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.1333333402872086f, 0.2588235437870026f, 0.4235294163227081f, 1.0f);
				style.Colors[ImGuiCol_PlotLines] = ImVec4(0.6078431606292725f, 0.6078431606292725f, 0.6078431606292725f, 1.0f);
				style.Colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.0f, 0.4274509847164154f, 0.3490196168422699f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogram] = ImVec4(0.8980392217636108f, 0.6980392336845398f, 0.0f, 1.0f);
				style.Colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.0f, 0.6000000238418579f, 0.0f, 1.0f);
				style.Colors[ImGuiCol_TableHeaderBg] = ImVec4(0.1882352977991104f, 0.1882352977991104f, 0.2000000029802322f, 1.0f);
				style.Colors[ImGuiCol_TableBorderStrong] = ImVec4(0.3098039329051971f, 0.3098039329051971f, 0.3490196168422699f, 1.0f);
				style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.2274509817361832f, 0.2274509817361832f, 0.2470588237047195f, 1.0f);
				style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
				style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.0f, 1.0f, 1.0f, 0.05999999865889549f);
				style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 0.3499999940395355f);
				style.Colors[ImGuiCol_DragDropTarget] = ImVec4(1.0f, 1.0f, 0.0f, 0.8999999761581421f);
				style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.2588235437870026f, 0.5882353186607361f, 0.9764705896377563f, 1.0f);
				style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.0f, 1.0f, 1.0f, 0.699999988079071f);
				style.Colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.2000000029802322f);
				style.Colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.800000011920929f, 0.800000011920929f, 0.800000011920929f, 0.3499999940395355f);

				ImVec4* colors = ImGui::GetStyle().Colors;
				colors[ImGuiCol_Text] = ImVec4(1.00f, 1.00f, 1.00f, 1.00f);
				colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.00f);
				colors[ImGuiCol_WindowBg] = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
				colors[ImGuiCol_ChildBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_PopupBg] = ImVec4(0.08f, 0.08f, 0.08f, 0.94f);
				colors[ImGuiCol_Border] = ImVec4(0.43f, 0.43f, 0.50f, 0.50f);
				colors[ImGuiCol_BorderShadow] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_FrameBg] = ImVec4(0.44f, 0.44f, 0.44f, 0.60f);
				colors[ImGuiCol_FrameBgHovered] = ImVec4(0.57f, 0.57f, 0.57f, 0.70f);
				colors[ImGuiCol_FrameBgActive] = ImVec4(0.76f, 0.76f, 0.76f, 0.80f);
				colors[ImGuiCol_TitleBg] = ImVec4(0.04f, 0.04f, 0.04f, 1.00f);
				colors[ImGuiCol_TitleBgActive] = ImVec4(0.16f, 0.16f, 0.16f, 1.00f);
				colors[ImGuiCol_TitleBgCollapsed] = ImVec4(0.00f, 0.00f, 0.00f, 0.60f);
				colors[ImGuiCol_MenuBarBg] = ImVec4(0.14f, 0.14f, 0.14f, 1.00f);
				colors[ImGuiCol_ScrollbarBg] = ImVec4(0.02f, 0.02f, 0.02f, 0.53f);
				colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.31f, 0.31f, 0.31f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.41f, 0.41f, 0.41f, 1.00f);
				colors[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.51f, 0.51f, 0.51f, 1.00f);
				colors[ImGuiCol_CheckMark] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
				colors[ImGuiCol_SliderGrab] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
				colors[ImGuiCol_SliderGrabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_Button] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
				colors[ImGuiCol_ButtonHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
				colors[ImGuiCol_ButtonActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_Header] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
				colors[ImGuiCol_HeaderHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
				colors[ImGuiCol_HeaderActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_Separator] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
				colors[ImGuiCol_SeparatorHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
				colors[ImGuiCol_SeparatorActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_ResizeGrip] = ImVec4(0.13f, 0.75f, 0.55f, 0.40f);
				colors[ImGuiCol_ResizeGripHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.60f);
				colors[ImGuiCol_ResizeGripActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_Tab] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
				colors[ImGuiCol_TabHovered] = ImVec4(0.13f, 0.75f, 0.75f, 0.80f);
				colors[ImGuiCol_TabActive] = ImVec4(0.13f, 0.75f, 1.00f, 0.80f);
				colors[ImGuiCol_TabUnfocused] = ImVec4(0.18f, 0.18f, 0.18f, 1.00f);
				colors[ImGuiCol_TabUnfocusedActive] = ImVec4(0.36f, 0.36f, 0.36f, 0.54f);
				colors[ImGuiCol_DockingPreview] = ImVec4(0.13f, 0.75f, 0.55f, 0.80f);
				colors[ImGuiCol_DockingEmptyBg] = ImVec4(0.13f, 0.13f, 0.13f, 0.80f);
				colors[ImGuiCol_PlotLines] = ImVec4(0.61f, 0.61f, 0.61f, 1.00f);
				colors[ImGuiCol_PlotLinesHovered] = ImVec4(1.00f, 0.43f, 0.35f, 1.00f);
				colors[ImGuiCol_PlotHistogram] = ImVec4(0.90f, 0.70f, 0.00f, 1.00f);
				colors[ImGuiCol_PlotHistogramHovered] = ImVec4(1.00f, 0.60f, 0.00f, 1.00f);
				colors[ImGuiCol_TableHeaderBg] = ImVec4(0.19f, 0.19f, 0.20f, 1.00f);
				colors[ImGuiCol_TableBorderStrong] = ImVec4(0.31f, 0.31f, 0.35f, 1.00f);
				colors[ImGuiCol_TableBorderLight] = ImVec4(0.23f, 0.23f, 0.25f, 1.00f);
				colors[ImGuiCol_TableRowBg] = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
				colors[ImGuiCol_TableRowBgAlt] = ImVec4(1.00f, 1.00f, 1.00f, 0.07f);
				colors[ImGuiCol_TextSelectedBg] = ImVec4(0.26f, 0.59f, 0.98f, 0.35f);
				colors[ImGuiCol_DragDropTarget] = ImVec4(1.00f, 1.00f, 0.00f, 0.90f);
				colors[ImGuiCol_NavHighlight] = ImVec4(0.26f, 0.59f, 0.98f, 1.00f);
				colors[ImGuiCol_NavWindowingHighlight] = ImVec4(1.00f, 1.00f, 1.00f, 0.70f);
				colors[ImGuiCol_NavWindowingDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.20f);
				colors[ImGuiCol_ModalWindowDimBg] = ImVec4(0.80f, 0.80f, 0.80f, 0.35f);
			}
			
			ImGui::End();
		}
	}

	void Application::Close()
	{
		m_Running = false;
	}

	void Application::UpdateImageInfo()
	{
		m_Renderer->GetShader()->RecreateShader({
			{ "VertexShader", Photoxel::ShaderType::Vertex },
			{ "PixelShader", Photoxel::ShaderType::Pixel }
		}, m_ImageFilters);
		m_HistogramHasUpdate = true;
	}
}