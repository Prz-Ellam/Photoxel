#pragma once

#include <imgui.h>
#include <memory>
#include <ImSequencer.h>
#include <vector>
#include <glm/glm.hpp>

namespace Photoxel
{
	static const char* SequencerItemTypeNames[] = { "Video" };


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
		void Run();

		void Close();
	private:
		std::shared_ptr<Photoxel::Window> m_Window;
		std::shared_ptr<Photoxel::Renderer> m_Renderer;
		std::shared_ptr<Photoxel::Framebuffer> m_ViewportFramebuffer;
		std::shared_ptr<Photoxel::ImGuiLayer> m_GuiLayer;
		bool m_Running;
		ImVec2 m_ViewportSize = ImVec2(0, 0);
		std::shared_ptr<Photoxel::ImGuiWindow> m_GuiWindow;

		std::shared_ptr<Photoxel::Image> m_Image;
		MySequence mySequence;

		glm::mat4 m_Projection, m_View = glm::mat4(1.0f), m_Model = glm::mat4(1.0f);
		int pixel;
		ImVec2 mousePosition;
		bool start = false;

		void RenderMenuBar();
		void RenderImageTab();
		void RenderVideoTab();
		void RenderCameraTab();
	};
}