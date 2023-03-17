#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "Framebuffer.h"
#include "ImGui.h"
#include "ImGuiWindow.h"
#include <ImGuizmo.h>
#include <thread>
#include <chrono>
#include "Image.h"
#include <ImCurveEdit.h>
#include <math.h>
#include <vector>
#include <algorithm>
#include "FileDialog.h"
#include <iostream>
#include <glm/gtx/matrix_decompose.hpp>
#include "imgui_internals.h"
#include <filesystem>
#include <string.h>
#include <implot.h>
#include "IconsFontAwesome5.h"
#include <glad/glad.h>

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
		m_ViewportFramebuffer = std::make_shared<Framebuffer>(1280, 720);

		m_GuiLayer = std::make_shared<ImGuiLayer>(m_Window);
		m_GuiWindow = std::make_shared<ImGuiWindow>("Viewport", false);

		m_Image = std::make_shared<Photoxel::Image>("image.jpg");
		const int data = -16777216;
		m_Camera = std::make_shared<Photoxel::Image>(1, 1, &data);
		m_Projection = glm::ortho(m_Window->GetWidth() / -2.0f,
			m_Window->GetWidth() / 2.0f,
			m_Window->GetHeight() / -2.0f,
			m_Window->GetHeight() / 2.0f,
			0.0f, 1000.0f);

		m_WebcamDevices = setupESCAPI();
		if (m_WebcamDevices == 0) {
			printf("ESCAPI initialization failure or no devices found.\n");
			return;
		}

		for (int i = 0; i < m_WebcamDevices; i++) {
			char cameraname[255];
			getCaptureDeviceName(i, cameraname, 255);
			m_WebcamDevicesNames.push_back(cameraname);
		}

		m_Capture.mWidth = 512;
		m_Capture.mHeight = 512;
		m_Capture.mTargetBuf = new int[512 * 512];
		m_Detector = dlib::get_frontal_face_detector();
		m_DetectionData.resize(512 * 512);
	}

	void Application::Run()
	{
		mySequence.mFrameMin = 0;
		mySequence.mFrameMax = 100;
		mySequence.myItems.push_back(MySequence::MySequenceItem{ 0, 0, 60, false });

		while (m_Running)
		{
			m_ViewportFramebuffer->Resize(m_ViewportSize.x, m_ViewportSize.y);
			m_ViewportFramebuffer->Begin();

			m_Renderer->BeginScene(m_Projection, m_View, m_Model, m_Dets);
			m_ViewportFramebuffer->ClearAttachment();
			m_Image->Bind();

			m_Renderer->OnRender();
			pixel = m_ViewportFramebuffer->ReadPixel(mousePosition.x, mousePosition.y);

			m_ViewportFramebuffer->End();

			m_GuiLayer->Begin();
			RenderMenuBar();
			static bool open = true;
			if (open) {
				ImGui::Begin("Histogram", &open);

				uint8_t* data = (uint8_t*)m_Image->GetData();
				static std::vector<float> red, green, blue;

				if (red.size() == 0) {
					for (int i = 0; i < m_Image->GetWidth() * m_Image->GetHeight(); i++) {
						red.push_back((float)data[4 * i]);
						green.push_back((float)data[4 * i + 1]);
						blue.push_back((float)data[4 * i + 2]);
					}
				}
				ImGui::PlotHistogramColour("HistogramRed", red.data(), red.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
				ImGui::PlotHistogramColour("HistogramGreen", green.data(), green.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
				ImGui::PlotHistogramColour("HistogramBlue", blue.data(), blue.size(), 0, NULL, 0.0f, 255.0f, ImVec2(ImGui::GetContentRegionAvail().x, 100.0f), 4, ImVec4(0.0f, 0.0f, 1.0f, 1.0f));
				ImGui::End();
			}

			if (open) {
				ImGui::Begin("Brightness", &open);
				static float brightness = 0.0f;
				ImGui::SliderFloat("Brightness", &brightness, 0.0f, 1.0f);
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
					std::string filepath = FileDialog::OpenFile(*m_Window.get(), "(.jpg)\0*.jpg\0(.png)\0*.png");
					std::cout << filepath << std::endl;
				}

				ImGui::Separator();

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File", "Ctrl+S"))
					;

				if (ImGui::MenuItem(ICON_FA_SAVE"\t Save File As...", "Ctrl+Shift+S")) {
					std::string filepath = FileDialog::SaveFile(*m_Window.get(), "(.jpg)\0*.jpg\0(.png)\0*.png");
				}
				
				ImGui::Separator();

				if (ImGui::MenuItem(ICON_FA_DOOR_OPEN"\tExit", "Alt+F4"))
					Close();

				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Help")) {
				if (ImGui::MenuItem(ICON_FA_BOOK"\tUser Manual")) {
					ShellExecuteA(GetDesktopWindow(), "open", "request.pdf", NULL, NULL, SW_SHOWNORMAL);
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
		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Viewport");

		ImVec2 windowSize = ImGui::GetWindowSize();
		ImVec2 windowOffset = ImGui::GetWindowPos();
		m_ViewportSize = ImGui::GetContentRegionAvail();
		m_Projection = glm::ortho(
			m_ViewportSize.x / -2.0f,
			m_ViewportSize.x / 2.0f,
			m_ViewportSize.y / -2.0f,
			m_ViewportSize.y / 2.0f,
			0.0f, 1000.0f
		);

		mousePosition = ImGui::GetMousePos();
		mousePosition.x -= windowOffset.x;
		mousePosition.y -= windowOffset.y;
		mousePosition.y -= windowSize.y - m_ViewportSize.y; // Eliminar el tamaño tab

		//ImGui::Image(m_Image->GetTextureID(), m_ViewportSize);
		ImGui::Image((void*)m_ViewportFramebuffer->GetColorAttachment(), m_ViewportSize, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		if (false)
		{
			ImGuizmo::SetOrthographic(true);
			ImGuizmo::SetDrawlist();
			ImGuizmo::SetRect(
				windowOffset.x,
				windowOffset.y,
				m_ViewportSize.x,
				m_ViewportSize.y
			);

			ImGuizmo::Manipulate(
				glm::value_ptr(m_View),
				glm::value_ptr(m_Projection),
				ImGuizmo::OPERATION::TRANSLATE,
				ImGuizmo::MODE::LOCAL,
				glm::value_ptr(m_Model)
			);
		}
		ImGui::End();
		ImGui::PopStyleVar();


		ImGui::Begin("Tree");
		if (ImGui::TreeNodeEx("Trees"))
		{
			ImVec2 imageSize = ImVec2(64, 64);
			ImVec2 size = ImGui::GetContentRegionAvail();
			
			//ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
			ImGui::ImageButton((void*)m_Image->GetTextureID(),
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::Text("Image 1");
			ImGui::Separator();
			ImGui::ImageButton((void*)m_Image->GetTextureID(),
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0));
			ImGui::SameLine();
			ImGui::Text("Image 2");
			ImGui::Separator();
			//ImGui::PopStyleColor();
			ImGui::TreePop();
		}
		ImGui::End();


		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Effects");
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


		ImGui::Begin("Stats");
		ImGui::Text("Image name");
		ImGui::Text("Image size");
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);
		ImGui::End();
	}

	void Application::RenderVideoTab()
	{
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin(ICON_FA_VIDEO" Videos");
		ImGui::DockSpace(ImGui::GetID("MyDockSpace"), ImVec2(0.0f, 0.0f));

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.0f, 0.0f, 0.0f));
		ImGui::Begin("Video Effects");
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
		ImVec2 videoViewportSize = ImGui::GetContentRegionAvail();
		float ratio = videoViewportSize.y / (float)m_Image->GetHeight();

		ImGui::SetCursorPosX((videoViewportSize.x / 2) - (m_Image->GetWidth() * ratio / 2));


		ImGui::Image((ImTextureID)m_Image->GetTextureID(),
			ImVec2(m_Image->GetWidth() * ratio, videoViewportSize.y), ImVec2(0, 1), ImVec2(1, 0));
		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("TreeVideo");

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
			/*ImGui::ImageButtonWithText(
				(void*)m_Image->GetTextureID(),
				"Hola Mundo",
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0),
				ImVec2(size.x, 64)
			);*/
			ImGui::Separator();

			//ImGui::PopStyleColor();
			ImGui::TreePop();
		}

		ImGui::End();


		static int selectedEntry = -1;
		static int firstFrame = 0;
		static bool expanded = true;
		static int currentFrame = 0;
		if (start) currentFrame++;

		ImGui::Begin("Timeline");
		ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (70 * 0.5f));
		ImGui::Button(ICON_FA_PAUSE, ImVec2(20, 0));
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			start = true;
		}
		ImGui::SameLine();
		ImGui::Button(ICON_FA_STOP, ImVec2(20, 0));
		

		ImSequencer::Sequencer(&mySequence, &currentFrame, &expanded, &selectedEntry, &firstFrame, ImSequencer::SEQUENCER_EDIT_STARTEND | ImSequencer::SEQUENCER_CHANGE_FRAME);
		// add a UI to edit that particular item
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
		ImGui::Combo("##", &item, m_WebcamDevicesNames.data(), m_WebcamDevicesNames.size());
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PLAY, ImVec2(20, 0))) {
			initCapture(item, &m_Capture);
			doCapture(item);
			m_IsRecording = true;
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_STOP, ImVec2(20, 0))) {
			deinitCapture(item);
			const int data = -16777216;
			m_Camera->SetData(1, 1, &data);
			m_IsRecording = false;
		}
		ImGui::End();

		if (m_IsRecording && isCaptureDone(item)) {
			
			for (int i = 0; i < m_Capture.mWidth * m_Capture.mHeight; i++) {
				m_Capture.mTargetBuf[i] = (m_Capture.mTargetBuf[i] & 0xff00ff00) |
					((m_Capture.mTargetBuf[i] & 0xff) << 16) |
					((m_Capture.mTargetBuf[i] & 0xff0000) >> 16);

				m_Capture.mTargetBuf[i] |= 0xff000000;
				m_DetectionData[i] = (m_Capture.mTargetBuf[i] & 0xff00ff00) +
					((m_Capture.mTargetBuf[i] & 0xff) << 16) +
					((m_Capture.mTargetBuf[i] & 0xff0000) >> 16) / 3;
			}

			dlib::array2d<unsigned char> img(512, 512);
			memcpy(&img[0][0], m_DetectionData.data(), 512 * 512 * sizeof(uint8_t));

			m_Dets = m_Detector(img, 0);

			m_Camera->SetData(512, 512, m_Capture.mTargetBuf);
			doCapture(item);
		}



		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
		ImGui::Begin("Camera Viewport");
		ImVec2 videoViewportSize = ImGui::GetContentRegionAvail();
		//float ratio = videoViewportSize.y / (float)m_Image->GetHeight();

		//ImGui::SetCursorPosX((videoViewportSize.x / 2) - (m_Image->GetWidth() * ratio / 2));
		//ImGui::Image((ImTextureID)m_Camera->GetTextureID(),
		//	ImVec2(m_Image->GetWidth() * ratio, videoViewportSize.y));
		ImGui::Image((void*)m_ViewportFramebuffer->GetColorAttachment(), videoViewportSize);

		ImGui::End();
		ImGui::PopStyleVar();

		ImGui::Begin("Camera Stats");
		std::string persons = ICON_FA_SMILE + std::string(" Numero de personas: ") + std::to_string(m_Dets.size());
		ImGui::Text(persons.c_str());
		ImGui::End();


		ImGui::Begin("TreeCamera");

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
			/*ImGui::ImageButtonWithText(
				(void*)m_Image->GetTextureID(),
				"Hola Mundo",
				imageSize,
				ImVec2(0, 1),
				ImVec2(1, 0),
				ImVec2(size.x, 64)
			);*/
			ImGui::Separator();

			//ImGui::PopStyleColor();
			ImGui::TreePop();
		}

		ImGui::End();
	}

	void Application::Close()
	{
		m_Running = false;
	}
}