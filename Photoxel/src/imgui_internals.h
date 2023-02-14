#include <imgui.h>
#define IMGUI_DEFINE_MATH_OPERATORS
#include <imgui_internal.h>
// Declaration (.h file)
namespace ImGui {
    // label is used as id
    // <0 frame_padding uses default frame padding settings. 0 for no padding
    IMGUI_API void          PlotHistogramColour(const char* label, const float* values, int values_count, int values_offset = 0, const char* overlay_text = NULL, float scale_min = FLT_MAX, float scale_max = FLT_MAX, ImVec2 graph_size = ImVec2(0, 0), int stride = sizeof(float), ImVec4 colour = ImVec4(0, 0, 0, 0));
    IMGUI_API int           PlotExColour(ImGuiPlotType plot_type, const char* label, float (*values_getter)(void* data, int idx), void* data, int values_count, int values_offset, const char* overlay_text, float scale_min, float scale_max, ImVec2 frame_size, ImVec4 colour = ImVec4(0, 0, 0, 0));
} // namespace ImGui
