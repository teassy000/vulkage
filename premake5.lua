
SRC_DIR 		= 	path.join(_WORKING_DIR, "src")
EXT_DIR 		= 	path.join(_WORKING_DIR, "externs")

BX_DIR 			= 	path.join(EXT_DIR, "bx")
BIMG_DIR 		= 	path.join(EXT_DIR, "bimg")
GLFW_DIR 		= 	path.join(EXT_DIR, "glfw")
GLM_DIR 		= 	path.join(EXT_DIR, "glm")
FAST_OBJ_DIR 	= 	path.join(EXT_DIR, "fast_obj")
IMGUI_DIR 		= 	path.join(EXT_DIR, "imgui")
MESH_OPT_DIR 	= 	path.join(EXT_DIR, "meshoptimizer")
TRACY_DIR 		= 	path.join(EXT_DIR, "tracy")
VOLK_DIR 		= 	path.join(EXT_DIR, "volk")
TINYSTL_DIR 	= 	path.join(EXT_DIR, "tinystl")

VK_SDK_DIR 		= os.getenv("VULKAN_SDK")
if VK_SDK_DIR == nil then
	error("VULKAN_SDK environment variable not set")
end

KTX_SDK_DIR 	= os.getenv("KTX_SDK")
if KTX_SDK_DIR == nil then
	error("KTX_SDK_DIR environment variable not set")
end


workspace "vulkage"
	configurations {"debug", "debug_prof", "release", "release_prof"}
	platforms {"x64"}
	location "build"
	targetdir "bin/%{cfg.buildcfg}"

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

dofile("scripts/bx.lua")
dofile("scripts/bimg.lua")
dofile("scripts/glfw.lua")
dofile("scripts/imgui.lua")
dofile("scripts/meshoptimizer.lua")

-- vulkage
project "vulkage"
	kind "ConsoleApp"
	language "c++"
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
