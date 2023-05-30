#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "ImGui.h"
#include "ImGuiWindow.h"
#include "Image.h"
#include "FileDialog.h"
#include "imgui_internals.h"
#include <filesystem>
#include "IconsFontAwesome5.h"
#include "ColorGenerator.h"
#include <stb_image_write.h>
#include <stb_image_resize.h>

#define WIDTH 1280
#define HEIGHT 720

namespace Photoxel
{
	Application::Application()
		: m_Running(true)
	{
		std::cout << std::filesystem::current_path() << std::endl;
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
		m_Detector = dlib::get_frontal_face_detector();

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
			if (ImGui::SliderFloat("Brightness", &m_Brightness, 0.0f, 2.0f)) {
				m_HistogramHasUpdate = true;
			}
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
		ImGui::End();
	}

	void Application::RenderVideoTab()
	{
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
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (70 * 0.5f));
		if (ImGui::Button(ICON_FA_PAUSE, ImVec2(20, 0))) {
			if (m_Video)
				m_Video->Pause();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			if (m_Video)
				m_Video->Resume();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, ImVec2(20, 0))) {
			const int data = -16777216;
			m_VideoFrame->SetData(1, 1, &data);
			m_Video = nullptr;
			currentFrame = 0;
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
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			m_Capture2.StartCapture(item);
			//initCapture(item, &m_Capture);
			//doCapture(item);
			m_IsRecording = true;
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, ImVec2(20, 0))) {
			m_Capture2.StopCapture();
			//deinitCapture(item);
			const int data = -16777216;
			m_Camera->SetData(1, 1, &data);
			m_Dets.clear();
			m_IsRecording = false;
		}
		ImGui::End();

		static int a = 0;
		if (m_IsRecording) {
			uint8_t* data = m_Capture2.GetBuffer();
			uint32_t width = 1024;
			uint32_t height = 1024;

			dlib::array2d<dlib::rgb_pixel> img(width, height);
			memcpy(&img[0][0], data, width * height * 3);

			uint32_t scaledWidth = 512;
			uint32_t scaledHeight = 512;
			dlib::array2d<dlib::rgb_pixel> img2(scaledWidth, scaledHeight);
			if (a > 10) {

				std::vector<uint8_t> rescale(width * height * 3);
				stbir_resize_uint8(data, width, height, width * 3,
					rescale.data(), scaledWidth, scaledHeight, scaledWidth * 3, 3);

				memcpy(&img2[0][0], rescale.data(), scaledWidth * scaledHeight * 3);

				m_Dets = m_Detector(img2);
				a = 0;
			}
			else {
				a++;
			}
			int iterator = 0;
			for (const auto& face : m_Dets) {
				dlib::rectangle rect(
					face.left() * (width / static_cast<double>(scaledWidth)),
					face.top() * (height / static_cast<double>(scaledHeight)),
					face.right() * (width / static_cast<double>(scaledWidth)),
					face.bottom() * (height / static_cast<double>(scaledHeight))
				);
				dlib::draw_rectangle(img, rect, GetBasicColor(iterator), 1 * 4);
				dlib::point labelPos(rect.left() - 10, rect.top());
				// TODO: Need to change draw_string to work from () to []
				dlib::draw_string(img, labelPos, std::to_string(iterator + 1), 
					GetBasicColor(iterator));
				iterator++;
			}

			m_Camera->SetData(width, height, &img[0][0]);
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
		//ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		std::string persons = ICON_FA_SMILE + std::string(" Face count: ") + std::to_string(m_Dets.size());
		ImGui::Text(persons.c_str());
		ImGui::End();
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