#pragma once

#include <imgui.h>
#include <memory>
#include <ImSequencer.h>
#include <vector>
#include <string>
#include <glm/glm.hpp>
#include <dlib/image_processing/frontal_face_detector.h>
#include <dlib/image_processing.h>
#include <dlib/image_io.h>
#include "Video.h"
#include "Filters.h"
#include "Capture.h"
#include <thread>
#include <mutex>

namespace Photoxel {
	static const char* SequencerItemTypeNames[] = { "Video" };

	enum Section {
		IMAGE,
		VIDEO,
		CAMERA
	};

	struct MySequence : public ImSequencer::SequenceInterface
	{
		// interface with sequencer
		virtual int GetFrameMin() const
		{
			return mFrameMin;
		}

		virtual int GetFrameMax() const
		{
			return mFrameMax;
		}

		virtual int GetItemCount() const
		{
			return (int)myItems.size();
		}

		virtual int GetItemTypeCount() const
		{
			return sizeof(SequencerItemTypeNames) / sizeof(char*);
		}

		virtual const char* GetItemTypeName(int typeIndex) const
		{
			return SequencerItemTypeNames[typeIndex];
		}

		virtual const char* GetItemLabel(int index) const
		{
			static char tmps[512];
			snprintf(tmps, 512, "[%02d] %s", index, SequencerItemTypeNames[myItems[index].mType]);
			return tmps;
		}

		virtual void Get(int index, int** start, int** end, int* type, unsigned int* color)
		{
			MySequenceItem& item = myItems[index];
			if (color)
				*color = 0xFFFFFFFF; // same color for everyone, return color based on type
			if (start)
				*start = &item.mFrameStart;
			if (end)
				*end = &item.mFrameEnd;
			if (type)
				*type = item.mType;
		}

		virtual void Add(int type)
		{
			myItems.push_back(MySequenceItem{ type, 0, 10, false });
		}

		virtual void Del(int index)
		{
			myItems.erase(myItems.begin() + index);
		}

		virtual void Duplicate(int index)
		{
			myItems.push_back(myItems[index]);
		}

		virtual size_t GetCustomHeight(int index)
		{
			return myItems[index].mExpanded ? 100 : 0;
		}

		// my datas
		MySequence() : mFrameMin(0), mFrameMax(0) {}
		int mFrameMin, mFrameMax;
		struct MySequenceItem
		{
			int mType;
			int mFrameStart, mFrameEnd;
			bool mExpanded;
		};
		std::vector<MySequenceItem> myItems;

		virtual void DoubleClick(int index) {
			if (myItems[index].mExpanded)
			{
				myItems[index].mExpanded = false;
				return;
			}
			for (auto& item : myItems)
				item.mExpanded = false;
			myItems[index].mExpanded = !myItems[index].mExpanded;
		}

		virtual void CustomDraw(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& legendRect, const ImRect& clippingRect, const ImRect& legendClippingRect)
		{
		}

		virtual void CustomDrawCompact(int index, ImDrawList* draw_list, const ImRect& rc, const ImRect& clippingRect)
		{
		}
	};

	class Window;
	class Renderer;
	class Framebuffer;
	class ImGuiLayer;
	class ImGuiWindow;
	class Image;

	class Application
	{
	public:
		Application();
		~Application() = default;
		void Run();

		void Close();
	private:
		std::shared_ptr<Photoxel::Window> m_Window;
		std::shared_ptr<Photoxel::Renderer> m_Renderer;
		std::shared_ptr<Photoxel::Framebuffer> m_ViewportFramebuffer;
		std::shared_ptr<Photoxel::ImGuiLayer> m_GuiLayer;
		bool m_Running;
		std::shared_ptr<Photoxel::ImGuiWindow> m_GuiWindow;
		Capture m_Capture;

		std::shared_ptr<Photoxel::Image> m_Image, m_VideoFrame, m_Camera;
		std::shared_ptr<Video> m_Video = nullptr;
		MySequence mySequence;
		
		int m_WebcamDevicesCount;
		std::vector<std::string> m_WebcamDevicesNames;
		std::vector<const char*> m_WebcamDevicesNamesRef;
		bool m_IsRecording = false;
		dlib::frontal_face_detector m_Detector;
		std::vector<dlib::rectangle> m_Dets;

		Section m_SectionFocus = IMAGE;
		std::unordered_set<Filter> m_ImageFilters;

		float m_Contrast = 0.0f;
		float m_Thresehold = 0.0f;

		float m_ImageScale = 1.0f;

		std::vector<float> red, green, blue;
		bool m_HistogramHasUpdate = true;

		void RenderMenuBar();
		void RenderImageTab();
		void RenderVideoTab();
		void RenderCameraTab();
	};
}