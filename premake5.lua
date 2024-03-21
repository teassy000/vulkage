local EXT_DIR = "externs"

local BX_DIR = 			path.join(EXT_DIR, "bx")
local BIMG_DIR = 		path.join(EXT_DIR, "bimg")
local GLFW_DIR = 		path.join(EXT_DIR, "glfw")
local GLM_DIR = 		path.join(EXT_DIR, "glm")
local FAST_OBJ_DIR = 	path.join(EXT_DIR, "fast_obj")
local IMGUI_DIR = 		path.join(EXT_DIR, "imgui")
local MESH_OPT_DIR = 	path.join(EXT_DIR, "meshoptimizer")
local TRACY_DIR = 		path.join(EXT_DIR, "tracy")
local VOLK_DIR = 		path.join(EXT_DIR, "volk")
local TINYSTL_DIR = 	path.join(EXT_DIR, "tinystl")

local VK_SDK_DIR = os.getenv("VULKAN_SDK")
if VK_SDK_DIR == nil then
	error("VULKAN_SDK environment variable not set")
end

local KTX_SDK_DIR = os.getenv("KTX_SDK")
if KTX_SDK_DIR == nil then
	error("KTX_SDK_DIR environment variable not set")
end


local SRC_DIR = "src"

workspace "vulkage"
	configurations {"debug", "debug_prof", "release", "release_prof"}
	platforms {"x64"}

function disable_exceptions()
	filter "action:vs*"
		flags { "NoExceptions" }
	filter "action:gmake"
		buildoptions { "-fno-exceptions" }
end

function disable_msvc_crt_warning()
	filter "action:vs*"
		defines "_CRT_SECURE_NO_WARNINGS"
end

function enable_msvc_muiltithreaded()
	filter "action:vs*"
		flags { "MultiProcessorCompile" }
end

function set_bx_compat()
	filter "action:vs*"
		includedirs { path.join(BX_DIR, "include/compat/msvc") }
	buildoptions {
		"/wd4201", -- warning C4201: nonstandard extension used: nameless struct/union
		"/wd4324", -- warning C4324: '': structure was padded due to alignment specifier
		"/Ob2",    -- The Inline Function Expansion
		"/Zc:__cplusplus", -- Enable updated __cplusplus macro
	}
	linkoptions {
		"/ignore:4221", -- LNK4221: This object file does not define any previously undefined public symbols, so it will not be used by any link operation that consumes this library
	}
end

function common_filter()
	filter "configurations:release"
		runtime "Release"
		defines "NDEBUG"
	filter "configurations:release_prof"
		runtime "Release"
		defines "NDEBUG"
	filter "configurations:debug"
		runtime "Debug"
		defines "_DEBUG"
	filter "configurations:debug_prof"
		runtime "Debug"
		defines "_DEBUG"
end

function tracy_filter()
	filter "configurations:release_prof"
		defines {
			"TRACY_ENABLE",
			"TRACY_ON_DEMAND",
		}
	filter "configurations:debug_prof"
		defines {
			"TRACY_ENABLE",
			"TRACY_ON_DEMAND",
		}
	filter {"configurations:debug_prof", "action:vs*"}
		editandcontinue "Off" -- Tracy doesn't work with Edit and Continue, set debug info to "Program Database (/Zi)" instead

end

function using_bx()
	includedirs {
		path.join(BX_DIR, "include"),
	}

	links {
		"bx",
	}

	filter "configurations:release"
		defines "BX_CONFIG_DEBUG=0"
	filter "configurations:release_prof"
		defines "BX_CONFIG_DEBUG=0"
	filter "configurations:debug"
		defines "BX_CONFIG_DEBUG=1"
	filter "configurations:debug_prof"
		defines "BX_CONFIG_DEBUG=1"
end

-- bx
project "bx"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	defines "__STDC_FORMAT_MACROS"
	files
	{
		path.join(BX_DIR, "include/bx/*.h"),
		path.join(BX_DIR, "include/bx/inline/*.inl"),
		path.join(BX_DIR, "src/*.cpp")
	}
	excludes
	{
		path.join(BX_DIR, "src/amalgamated.cpp"),
		path.join(BX_DIR, "src/crtnone.cpp")
	}
	includedirs
	{
		path.join(BX_DIR, "include"),
		path.join(BX_DIR, "3rdparty"),
	}
	
	filter "configurations:release"
		defines "BX_CONFIG_DEBUG=0"
	filter "configurations:release_prof"
		defines "BX_CONFIG_DEBUG=0"
	filter "configurations:debug"
		defines "BX_CONFIG_DEBUG=1"
	filter "configurations:debug_prof"
		defines "BX_CONFIG_DEBUG=1"

	common_filter();
	set_bx_compat()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- bimg
project "bimg"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
	exceptionhandling "Off"
	rtti "Off"
	files
	{
		path.join(BIMG_DIR, "include/bimg/*.h"),
		path.join(BIMG_DIR, "src/image.cpp"),
		path.join(BIMG_DIR, "src/image_gnf.cpp"),
		path.join(BIMG_DIR, "src/*.h"),
		path.join(BIMG_DIR, "3rdparty/astc-codec/src/decoder/*.cc"),
		
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.cpp"),
		path.join(BIMG_DIR, "3rdparty/astc-encoder/source/**.h"),
	}
	includedirs
	{
		path.join(BX_DIR, "include"),
		path.join(BIMG_DIR, "include"),
		path.join(BIMG_DIR, "3rdparty/astc-codec"),
		path.join(BIMG_DIR, "3rdparty/astc-codec/include"),

		path.join(BIMG_DIR, "3rdparty/astc-encoder/include"),
	}

	using_bx()
	set_bx_compat()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- glfw	
project "glfw"
	kind "StaticLib"
	language "C"
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
		defines {
			"_GLFW_WIN32",
			"GLFW_EXPOSE_NATIVE_WIN32",
		}
		files {
			path.join(GLFW_DIR, "src/win32_*.*"),
			path.join(GLFW_DIR, "src/wgl_context.*")
		}

	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- imgui
project "imgui"
	kind "StaticLib"
	language "C++"
	files {
		path.join(IMGUI_DIR, "*.h"),
		path.join(IMGUI_DIR, "*.cpp"),
	}
	
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- meshoptimizer
project "meshoptimizer"
	kind "StaticLib"
	language "C++"
	files {
		path.join(MESH_OPT_DIR, "src/*.cpp"),
		path.join(MESH_OPT_DIR, "src/*.h"),
	}

	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- vulkage
project "vulkage"
	kind "ConsoleApp"
	language "c++"
	targetdir "bin/%{cfg.buildcfg}"
	rtti "Off"
	exceptionhandling "Off"

	files {
		path.join(SRC_DIR, "*.h"),
		path.join(SRC_DIR, "*.cpp"),
		path.join(SRC_DIR, "*.inl"),
		path.join(SRC_DIR, "shaders/*.glsl"),
		path.join(SRC_DIR, "shaders/*.h"),

		-- fast_obj
		path.join(FAST_OBJ_DIR, "fast_obj.h"),
		path.join(FAST_OBJ_DIR, "fast_obj.c"),

		-- volk
		path.join(VOLK_DIR, "volk.c"),
		path.join(VOLK_DIR, "volk.h"),

		-- tinystl		
		path.join(TINYSTL_DIR, "include/TINYSTL/*.h"),

		-- glm
		path.join(GLM_DIR, "glm/**.hpp"),
		path.join(GLM_DIR, "glm/**.inl"),
		path.join(GLM_DIR, "glm/**.h"),

		-- tracy
		path.join(TRACY_DIR, "public/TracyClient.cpp"),
	}

	includedirs {
		path.join(EXT_DIR, "glfw/include"),
		path.join(EXT_DIR, "volk"),
		path.join(EXT_DIR, "meshoptimizer/src"),
		path.join(EXT_DIR, "imgui"),
		path.join(EXT_DIR, "fast_obj"),
		path.join(EXT_DIR, "tinystl/include"),
		path.join(EXT_DIR, "glm"),
		path.join(EXT_DIR, "tracy/public"),
		path.join(VK_SDK_DIR, "Include"),
		path.join(KTX_SDK_DIR, "include"),
	}

	vpaths { 
		["src"] 	= {"src/*.h", "src/*.inl", "src/*.cpp"},
		["3rd"] 	= {"externs/**.h", "externs/**.cpp", "externs/**.c", "externs/**.inl"},
		["shaders"] = {"src/shaders/*.glsl", "src/shaders/*.h"}
	}

	defines {
		"GLM_FORCE_XYZW_ONLY",
		"NOMINMAX",
	}

	links {
		"bx",
		"bimg",
		"glfw",
		"imgui",
		"meshoptimizer",
		path.join(VK_SDK_DIR, "Lib", "vulkan-1.lib"),
		path.join(KTX_SDK_DIR, "lib", "ktx.lib"),
	}

	filter "system:windows"
		defines {
			"_GLFW_WIN32",
			"GLFW_EXPOSE_NATIVE_WIN32",
			"WIN32_LEAN_AND_MEAN",
			"VK_USE_PLATFORM_WIN32_KHR",
		}

   	filter "configurations:debug"
	   optimize "Debug"
	   symbols "On"
	
	filter "configurations:debug_prof"
		optimize "Debug"
		symbols "On"

   	filter "configurations:release"
	   optimize "Full"
	   
	filter "configurations:release_prof"
		optimize "Full"
	
	tracy_filter()
	using_bx()
	common_filter()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()

-- group projects
project("bx").group 			= "3rd"
project("bimg").group 			= "3rd"
project("glfw").group 			= "3rd"
project("imgui").group 			= "3rd"
project("meshoptimizer").group 	= "3rd"
