print("current glfw path: " .. GLFW_DIR);

os.chdir(_WORKING_DIR)

function using_glfw()
	filter "system:windows"
		defines {
			"_GLFW_WIN32",
			"GLFW_EXPOSE_NATIVE_WIN32",
		}
end


project "glfw"
	kind "StaticLib"
	language "c"
	files
	{
		path.join(GLFW_DIR, "include/GLFW/*.h"),
		path.join(GLFW_DIR, "src/context.c"),
		path.join(GLFW_DIR, "src/egl_context.*"),
		path.join(GLFW_DIR, "src/init.c"),
		path.join(GLFW_DIR, "src/input.c"),
		path.join(GLFW_DIR, "src/internal.h"),
		path.join(GLFW_DIR, "src/monitor.c"),
		path.join(GLFW_DIR, "src/null*.*"),
		path.join(GLFW_DIR, "src/osmesa_context.*"),
		path.join(GLFW_DIR, "src/platform.c"),
		path.join(GLFW_DIR, "src/vulkan.c"),
		path.join(GLFW_DIR, "src/window.c"),
	}
	includedirs { 
		path.join(GLFW_DIR, "include"),
		path.join(VK_SDK_DIR, "Include"),
	}
	filter "system:windows"
		files {
			path.join(GLFW_DIR, "src/win32_*.*"),
			path.join(GLFW_DIR, "src/wgl_context.*")
		}

	using_vulkan()
	using_glfw()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
	common_filter()
