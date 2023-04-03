workspace "Photoxel"
    architecture "x86_64"
    configurations {
        "Debug",
        "Release",
        "Dist"
    }
    startproject "Photoxel"

outputdir = "%{cfg.buildcfg}-%{cfg.system}-%{cfg.architecture}"

project "Photoxel"
    location "Photoxel"
    kind "ConsoleApp"
    language "C++"
    cppdialect "C++17"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
	objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    debugdir ("%{prj.name}/assets")

    files {
        "%{prj.name}/src/**.h", 
        "%{prj.name}/src/**.cpp",
        "vendor/imgui/backends/imgui_impl_glfw.cpp",
        "vendor/imgui/backends/imgui_impl_opengl3.cpp",
        "vendor/escapi/escapi.cpp"
    }

    defines {
        "GLFW_INCLUDE_NONE"
    }

    includedirs {
        "vendor/glfw/include",
        "vendor/glad/include",
        "vendor/glm",
        "vendor/imgui",
        "vendor/ImGuizmo",
        "vendor/implot",
        "vendor/stb",
        "vendor/IconFontCppHeaders",
        "vendor/escapi",
        "vendor/dlib",
        "vendor/ffmpeg/include"
    }

    libdirs {
        "vendor/ffmpeg/lib",
        "Photoxel/assets"
    }

    links {
		"GLFW",
		"Glad",
		"ImGui",
		"ImGuizmo",
        "ImPlot",
		"opengl32.lib",
        "swscale.lib",
        "swresample.lib",
        "postproc.lib",
        "avutil.lib",
        "avformat.lib",
        "avfilter.lib",
        "avdevice.lib",
        "avcodec.lib",
        "dlib19.24.0_release_64bit_msvc1932.lib"
	}

    filter "configurations:Debug"
		defines { "DEBUG" }
		symbols "On"

	filter "configurations:Release"
		defines { "NDEBUG" }
		optimize "On"

glfw = "vendor/glfw/"
glad = "vendor/glad/"
imgui = "vendor/imgui/"
imguizmo = "vendor/ImGuizmo/"
implot = "vendor/implot/"

project "GLFW"
    kind "StaticLib"
    language "C"
    location "vendor/GLFW"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files
    {
        (glfw .. "include/GLFW/glfw3.h"),
        (glfw .. "include/GLFW/glfw3native.h"),
        (glfw .. "src/glfw_config.h"),
        (glfw .. "src/context.c"),
        (glfw .. "src/init.c"),
        (glfw .. "src/input.c"),
        (glfw .. "src/monitor.c"),
        (glfw .. "src/vulkan.c"),
        (glfw .. "src/window.c"),
        (glfw .. "src/platform.c"),
        (glfw .. "src/null_init.c"),
        (glfw .. "src/null_joystick.c"),
        (glfw .. "src/null_monitor.c"),
        (glfw .. "src/null_window.c")
    }

    filter "system:linux"
        pic "On"

        systemversion "latest"
        staticruntime "On"

        files
        {
            (glfw .. "src/x11_init.c"),
            (glfw .. "src/x11_monitor.c"),
            (glfw .. "src/x11_window.c"),
            (glfw .. "src/xkb_unicode.c"),
            (glfw .. "src/posix_time.c"),
            (glfw .. "src/posix_thread.c"),
            (glfw .. "src/glx_context.c"),
            (glfw .. "src/egl_context.c"),
            (glfw .. "src/osmesa_context.c"),
            (glfw .. "src/linux_joystick.c")
        }

        defines
        {
            "_GLFW_X11"
        }

    filter "system:windows"
        systemversion "latest"
        staticruntime "On"

        files
        {
            (glfw .. "src/win32_init.c"),
            (glfw .. "src/win32_joystick.c"),
            (glfw .. "src/win32_monitor.c"),
            (glfw .. "src/win32_time.c"),
            (glfw .. "src/win32_thread.c"),
            (glfw .. "src/win32_window.c"),
            (glfw .. "src/wgl_context.c"),
            (glfw .. "src/egl_context.c"),
            (glfw .. "src/osmesa_context.c"),
            (glfw .. "src/win32_module.c")
        }

        defines 
        { 
            "_GLFW_WIN32",
            "_CRT_SECURE_NO_WARNINGS"
        }

    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"

    filter "configurations:Release"
        runtime "Release"
        optimize "on"


project "ImGui"
    kind "StaticLib"
    language "C++"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files {
        (imgui .. "imconfig.h"),
        (imgui .. "imgui.h"),
        (imgui .. "imgui.cpp"),
        (imgui .. "imgui_draw.cpp"),
        (imgui .. "imgui_internal.h"),
        (imgui .. "imgui_widgets.cpp"),
        (imgui .. "imgui_tables.cpp"),
        (imgui .. "imstb_rectpack.h"),
        (imgui .. "imstb_textedit.h"),
        (imgui .. "imstb_truetype.h"),
        (imgui .. "imgui_demo.cpp")
    }
    
    filter "system:windows"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
    
    filter "system:linux"
        pic "On"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
    
    filter "configurations:Release"
        runtime "Release"
        optimize "on"


project "ImGuizmo"
    kind "StaticLib"
    language "C++"

    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")

    files {
        (imguizmo .. "GraphEditor.h"),
        (imguizmo .. "GraphEditor.cpp"),
        (imguizmo .. "ImCurveEdit.h"),
        (imguizmo .. "ImCurveEdit.cpp"),
        (imguizmo .. "ImGradient.h"),
        (imguizmo .. "ImGradient.cpp"),
        (imguizmo .. "ImGuizmo.h"),
        (imguizmo .. "ImGuizmo.cpp"),
        (imguizmo .. "ImSequencer.h"),
        (imguizmo .. "ImSequencer.cpp"),
        (imguizmo .. "ImZoomSlider.h")
    }
    
    includedirs {
        "vendor/imgui"
    }

    filter "system:windows"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
    
    filter "system:linux"
        pic "On"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
    
    filter "configurations:Release"
        runtime "Release"
        optimize "on"


project "ImPlot"
    kind "StaticLib"
    language "C++"
    
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files {
        (implot .. "implot.h"),
        (implot .. "implot.cpp"),
        (implot .. "implot_demo.cpp"),
        (implot .. "implot_internal.h"),
        (implot .. "implot_items.cpp")
    }
        
    includedirs {
        "vendor/imgui"
    }
    
    filter "system:windows"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
        
    filter "system:linux"
        pic "On"
        systemversion "latest"
        cppdialect "C++17"
        staticruntime "On"
        
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
        
    filter "configurations:Release"
        runtime "Release"
        optimize "on"


project "Glad"
    kind "StaticLib"
    language "C"
    staticruntime "off"
        
    targetdir ("bin/" .. outputdir .. "/%{prj.name}")
    objdir ("bin-int/" .. outputdir .. "/%{prj.name}")
    
    files {
        (glad .. "include/glad/glad.h"),
        (glad .. "include/KHR/khrplatform.h"),
        (glad .."src/glad.c")
    }
    
    includedirs {
        (glad .. "include")
    }
        
    filter "system:windows"
        systemversion "latest"
    
    filter "configurations:Debug"
        runtime "Debug"
        symbols "on"
    
    filter "configurations:Release"
        runtime "Release"
        optimize "on"