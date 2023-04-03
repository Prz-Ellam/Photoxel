#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "ImGui.h"
#include "ImGuiWindow.h"
#include <ImGuizmo.h>
#include "Image.h"
#include "FileDialog.h"
#include "imgui_internals.h"
#include <filesystem>
#include "IconsFontAwesome5.h"
#include "ColorGenerator.h"
#include <stb_image_write.h>
#include <stb_image_resize.h>

#define WIDTH 1024
#define HEIGHT 1024

float prueba = 0.0;

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

		//m_WebcamDevicesCount = setupESCAPI();
		m_WebcamDevicesCount = 2;
		if (m_WebcamDevicesCount == 0) {
			printf("ESCAPI initialization failure or no devices found.\n");
			return;
		}

		for (int i = 0; i < m_WebcamDevicesCount; i++) {
			char cameraname[255];
			//getCaptureDeviceName(i, cameraname, 255);
			m_WebcamDevicesNames.push_back(cameraname);
			m_WebcamDevicesNamesRef.push_back(m_WebcamDevicesNames[i].c_str());
		}

		//m_Capture.mWidth = WIDTH;
		//m_Capture.mHeight = HEIGHT;
		//m_Capture.mTargetBuf = new int[WIDTH * HEIGHT];
		m_Detector = dlib::get_frontal_face_detector();
	}

	void Application::Run() {
		mySequence.mFrameMin = 0;
		mySequence.mFrameMax = 0;
		mySequence.myItems.push_back(MySequence::MySequenceItem{ 0, 0, 10, true });

		while (m_Running) {
			//m_ViewportFramebuffer->Resize(m_Image->GetWidth(), m_Image->GetHeight());
			m_ViewportFramebuffer->Begin();

			if (m_Video) {
				if (!m_Video->IsPaused()) {
					uint8_t* buffer = m_Video->Read();
					if (!buffer) continue;
					m_VideoFrame->SetData2(m_Video->GetWidth(), m_Video->GetHeight(), buffer);
					delete[] buffer;
				}
			}

			m_Renderer->BeginScene();
			m_ViewportFramebuffer->ClearAttachment();
			
			switch (m_SectionFocus) {
				case IMAGE:
					if (m_Image) m_Image->Bind();
					break;
				case VIDEO:
					m_VideoFrame->Bind();
					break;
			}

			m_Renderer->Bind();
			dynamic_cast<Photoxel::Shader*>(m_Renderer->GetShader())->SetFloat("u_Brightness", prueba);
			dynamic_cast<Photoxel::Shader*>(m_Renderer->GetShader())->SetFloat("u_Contrast", m_Contrast);
			dynamic_cast<Photoxel::Shader*>(m_Renderer->GetShader())->SetFloat("u_Thresehold", m_Thresehold);

			if (m_Image) {
				dynamic_cast<Photoxel::Shader*>(m_Renderer->GetShader())->SetInt("u_Width", m_Image->GetWidth());
				dynamic_cast<Photoxel::Shader*>(m_Renderer->GetShader())->SetInt("u_Height", m_Image->GetHeight());
			}
			m_Renderer->OnRender();

			m_ViewportFramebuffer->End();

			m_GuiLayer->Begin();
			RenderMenuBar();

			static bool open = true;
			if (open) {
				ImGui::Begin("Histogram", &open);

				if (m_HistogramHasUpdate) {
					std::vector<uint8_t> data = m_ViewportFramebuffer->GetData();

					int width = m_ViewportFramebuffer->GetWidth();
					int height = m_ViewportFramebuffer->GetHeight();

					int pixelCount = width * height;

					red.clear();
					green.clear();
					blue.clear();

					red.reserve(pixelCount);
					green.reserve(pixelCount);
					blue.reserve(pixelCount);

					for (int i = 0; i < width * height; i++) {
						red.emplace_back((float)data[4 * i]);
						green.emplace_back((float)data[4 * i + 1]);
						blue.emplace_back((float)data[4 * i + 2]);
					}

					m_HistogramHasUpdate = false;
				}

				ImGui::PlotHistogramColour("HistogramRed", red.data(), red.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PlotHistogramColour("HistogramGreen", green.data(), green.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
				ImGui::PlotHistogramColour("HistogramBlue", blue.data(), blue.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
				ImGui::End();
			}

			RenderCameraTab();
			RenderVideoTab();
			RenderImageTab();

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
				if (ImGui::MenuItem(ICON_FA_FILE"\t New File", "Ctrl+N"))
					;

				if (ImGui::MenuItem(ICON_FA_FOLDER_OPEN"\tOpen File", "Ctrl+O")) {
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

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File", "Ctrl+S"))
					;

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File As...", "Ctrl+Shift+S")) {
					std::string filepath = FileDialog::SaveFile(*m_Window.get(), "(.jpg)\0*.jpg\0(.png)\0*.png");
					std::vector<uint8_t> data = m_ViewportFramebuffer->GetData();
					
					stbi_write_png(filepath.c_str(), m_Image->GetWidth(),
						m_Image->GetHeight(), 4, data.data(), m_Image->GetWidth() * 4);
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
		
		ImVec2 imageSize = ImVec2(64, 64);
		ImVec2 size = ImGui::GetContentRegionAvail();

		int item;

		if (ImGui::TreeNode("Selection State: Single Selection"))
		{
			for (auto& filter : m_ImageFilters) {
				if (ImGui::Selectable("s", 1))
					;
			}
			ImGui::TreePop();
		}

		if (m_ImageFilters.find(Filter::Brightness) != m_ImageFilters.end()) {
			if (ImGui::SliderFloat("Brillo", &prueba, 0.0f, 2.0f)) {
				m_HistogramHasUpdate = true;
			}
		}
		if (m_ImageFilters.find(Filter::Contrast) != m_ImageFilters.end()) {
			ImGui::SliderFloat("Contraste", &m_Contrast, -1.0f, 1.0f);
		}
		if (m_ImageFilters.find(Filter::Binary) != m_ImageFilters.end()) {
			ImGui::SliderFloat("Umbral", &m_Thresehold, 0.0f, 5.0f);
		}
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Effects");
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = IMAGE;
		}
		ImVec2 effectsSize = ImGui::GetContentRegionAvail();
		if (ImGui::Button("Negative", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Negative);
			red.clear();
			green.clear();
			blue.clear();
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
            }, m_ImageFilters);
		}
		if (ImGui::Button("Grayscale", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Grayscale);
			red.clear();
			green.clear();
			blue.clear();
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Sepia", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Sepia);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Brightness", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Brightness);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Contrast", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Contrast);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Edge Detection", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::EdgeDetection);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Binary", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Binary);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
		}
		if (ImGui::Button("Gradient", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Gradient);
		}
		if (ImGui::Button("Pixelate", ImVec2(effectsSize.x, 30.0f))) {
			m_ImageFilters.insert(Filter::Pixelate);
			m_Renderer->GetShader()->RecreateShader({
				{ "VertexShader", Photoxel::ShaderType::Vertex },
				{ "PixelShader", Photoxel::ShaderType::Pixel }
				}, m_ImageFilters);
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
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
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
		ImGui::Button("Negative", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Grayscale", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Sepia", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Brightness", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Contrast", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Edge Detection", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Binary", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Gaussian Blur", ImVec2(effectsSize.x, 30.0f));
		ImGui::Button("Gradient", ImVec2(effectsSize.x, 30.0f));
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
		if (ImGui::IsWindowFocused(ImGuiFocusedFlags_ChildWindows)) {
			m_SectionFocus = VIDEO;
		}
		if (ImGui::TreeNodeEx("Trees"))
		{
			ImVec2 imageSize = ImVec2(64, 64);

			ImVec2 size = ImGui::GetContentRegionAvail();

			//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::Image((void*)m_Image->GetTextureID(),
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0));
			/*ImGui::ImageButtonWithText(
				(void*)m_Image->GetTextureID(),
				"Hola Mundo",
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0),
				ImVec2(size.x, 64)
			);*/
			ImGui::Separator();
			ImGui::Image((void*)m_Image->GetTextureID(),
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0));
			ImGui::Separator();

			//ImGui::PopStyleColor();
			ImGui::TreePop();
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
			m_Video->Pause();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			
		}
		ImGui::SameLine();
		ImGui::Button(ICON_FA_STOP, ImVec2(20, 0));
		
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
		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Cameras");
		static int item = 0;
		std::vector<const char*> captureDevicesNames = m_Capture.GetCaptureDeviceNames();
		ImGui::Combo("##", &item, captureDevicesNames.data(), captureDevicesNames.size());
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			m_Capture.StartCapture(item);
			//initCapture(item, &m_Capture);
			//doCapture(item);
			m_IsRecording = true;
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, ImVec2(20, 0))) {
			m_Capture.StopCapture();
			//deinitCapture(item);
			const int data = -16777216;
			m_Camera->SetData(1, 1, &data);
			m_Dets.clear();
			m_IsRecording = false;
		}
		ImGui::End();
		/*
		if (m_IsRecording && isCaptureDone(item))
		{
			dlib::array2d<dlib::rgb_pixel> img(WIDTH, HEIGHT);
			uint8_t* data = (uint8_t*)m_Capture.mTargetBuf;
			#pragma omp parallel for
			for (int i = 0; i < m_Capture.mWidth * m_Capture.mHeight; i++) 
			{
				int index = i * 4;
				img[0][i].blue = data[index];
				img[0][i].green = data[index + 1];
				img[0][i].red = data[index + 2];
			}

			uint8_t rescale[256 * 256 * 4];
			stbir_resize_uint8(data, m_Capture.mWidth, m_Capture.mHeight, m_Capture.mWidth * 4,
				rescale, 256, 256, 256 * 4, 4);

			dlib::array2d<dlib::rgb_pixel> img2(256, 256);
			#pragma omp parallel for
			for (int i = 0; i < 256 * 256; i++)
			{
				int index = i * 4;
				img2[0][i].blue = rescale[index];
				img2[0][i].green = rescale[index + 1];
				img2[0][i].red = rescale[index + 2];
			}

			m_Dets = m_Detector(img2);
			int iterator = 0;
			for (const auto& face : m_Dets) {
				dlib::rectangle rect(face.left() * 4, face.top() * 4, face.right() * 4, face.bottom() * 4);
				dlib::draw_rectangle(img, rect, GetBasicColor(iterator), 1 * 4);
				dlib::point labelPos(rect.left() - 10, rect.top());
				// TODO: Need to change draw_string to work from () to []
				dlib::draw_string(img, labelPos, std::to_string(iterator + 1), 
					GetBasicColor(iterator));
				iterator++;
			}

			m_Camera->SetData(WIDTH, HEIGHT, &img[0][0]);
			//doCapture(item);
		}
		*/

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Camera Viewport");

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
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		std::string persons = ICON_FA_SMILE + std::string(" Face count: ") + std::to_string(m_Dets.size());
		ImGui::Text(persons.c_str());
		ImGui::End();
	}

	void Application::Close()
	{
		m_Running = false;
	}
}