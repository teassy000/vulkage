
SRC_DIR 		= 	path.join(_WORKING_DIR, "src")
EXT_DIR 		= 	path.join(_WORKING_DIR, "externs")

BX_DIR 			= 	path.join(EXT_DIR, "bx")
BIMG_DIR 		= 	path.join(EXT_DIR, "bimg")
GLFW_DIR 		= 	path.join(EXT_DIR, "glfw")
GLM_DIR 		= 	path.join(EXT_DIR, "glm")
FAST_OBJ_DIR 	= 	path.join(EXT_DIR, "fast_obj")
IMGUI_DIR 		= 	path.join(EXT_DIR, "imgui")
MESH_OPT_DIR 	= 	path.join(EXT_DIR, "meshoptimizer")
METIS_DIR 		= 	path.join(EXT_DIR, "metis")
TRACY_DIR 		= 	path.join(EXT_DIR, "tracy")
VOLK_DIR 		= 	path.join(EXT_DIR, "volk")
TINYSTL_DIR 	= 	path.join(EXT_DIR, "tinystl")
BGFX_COMMON_DIR = 	path.join(EXT_DIR, "bgfx_common")
CGLTF_DIR 		= 	path.join(EXT_DIR, "cgltf")
FFX_SDK_DIR 	= 	path.join(EXT_DIR, "FidelityFX/sdk") -- FidelityFX builded with option: .\BuildFidelityFXSDK.bat -DFFX_API_BACKEND=VK_X64 -DFFX_ALL=ON -DFFX_AUTO_COMPILE_SHADERS=1

VK_SDK_DIR 		= os.getenv("VULKAN_SDK")
if VK_SDK_DIR == nil then
	error("VULKAN_SDK environment variable not set")
end

KTX_SDK_DIR 	= os.getenv("KTX_SDK")
if KTX_SDK_DIR == nil then
	error("KTX_SDK_DIR environment variable not set")
end

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

function using_vulkan()
	filter "system:windows"
		defines {
			"VK_USE_PLATFORM_WIN32_KHR",
		}
end

function custom_build_glsl()
	local abs_filepath = path.join(SRC_DIR, "shaders", "%(Filename).glsl")
	local abs_outputpath = path.join(_WORKING_DIR, "shaders", "%(Filename).spv")
    -- Store include paths in an array
    local include_paths = {
        path.join(SRC_DIR, "shaders"),
        path.join(FFX_SDK_DIR, "include/FidelityFX/gpu"),
        path.join(FFX_SDK_DIR, "include/FidelityFX/gpu/brixelizer")
    }

	local include_args = ""
    for _, include_path in ipairs(include_paths) do
        include_args = include_args .. " -I" .. include_path
    end

	filter "files:**.glsl"
		buildmessage "custom compiling to spv..."
		buildcommands {
			"$(VULKAN_SDK)\\Bin\\glslangValidator " 
            .. abs_filepath 
            .. include_args 
            .. " -V -o " .. abs_outputpath 
            .. " --target-env spirv1.4"
		}
		buildoutputs {
			abs_outputpath
		}
end


workspace "vulkage"
	configurations {"debug", "debug_prof", "release", "release_prof"}
	platforms {"x64"}
	location "build"
	targetdir "bin/%{cfg.buildcfg}"
	startproject "vulkage"


dofile("scripts/bx.lua")
dofile("scripts/bimg.lua")
dofile("scripts/glfw.lua")
dofile("scripts/imgui.lua")
dofile("scripts/meshoptimizer.lua")
dofile("scripts/tracy.lua")
dofile("scripts/fast_obj.lua")
dofile("scripts/bgfx_common.lua")

-- vulkage
project "vulkage"
	kind "ConsoleApp"
	language "c++"
	cppdialect "C++17"
	rtti "Off"
	exceptionhandling "Off"

	files {
		path.join(SRC_DIR, "*.h"),
		path.join(SRC_DIR, "*.cpp"),
		path.join(SRC_DIR, "*.inl"),

		-- radiance cascade 
		path.join(SRC_DIR, "radiance_cascade/*.h"),
		path.join(SRC_DIR, "radiance_cascade/*.cpp"),

		-- deffered
		path.join(SRC_DIR, "deferred/*.h"),
		path.join(SRC_DIR, "deferred/*.cpp"),

		-- ffx_intg
		path.join(SRC_DIR, "ffx_intg/*.h"),
		path.join(SRC_DIR, "ffx_intg/*.cpp"),

		-- shaders
		path.join(SRC_DIR, "shaders/*.glsl"),
		path.join(SRC_DIR, "shaders/*.h"),

		-- ffx
		path.join(FFX_SDK_DIR, "include/FidelityFX/host/**.h"),
		path.join(FFX_SDK_DIR, "include/FidelityFX/gpu/**.h"),
		
		-- volk
		path.join(VOLK_DIR, "volk.c"),
		path.join(VOLK_DIR, "volk.h"),

		-- glm
		path.join(GLM_DIR, "glm/**.hpp"),
		path.join(GLM_DIR, "glm/**.inl"),
		path.join(GLM_DIR, "glm/**.h"),

		-- cgltf
		path.join(CGLTF_DIR, "cgltf.h"),
		path.join(CGLTF_DIR, "cgltf_write.h"),

		-- metis
		path.join(METIS_DIR, "include/**.h"),
	}

	includedirs {
		path.join(SRC_DIR),

		path.join(BX_DIR,	 	"include"),
		path.join(BIMG_DIR,	 	"include"),
		path.join(GLFW_DIR,	 	"include"),
		path.join(MESH_OPT_DIR,	"src"),
		path.join(TRACY_DIR,	"public"),
		path.join(BGFX_COMMON_DIR, ""),

		path.join(VOLK_DIR,		""),
		path.join(IMGUI_DIR,	""),
		path.join(FAST_OBJ_DIR,	""),
		path.join(GLM_DIR,		""),
		path.join(METIS_DIR,	"include"),
		path.join(FFX_SDK_DIR,	"include"),

		path.join(VK_SDK_DIR,	"Include"),
		path.join(KTX_SDK_DIR,	"include"),

		path.join(CGLTF_DIR,	""),
	}

	vpaths {
		["src"] = {
			path.join(SRC_DIR, "*.h"),
			path.join(SRC_DIR, "*.inl"),
			path.join(SRC_DIR, "*.cpp"),
		},

		["src/radiance_cascade"] = {
			path.join(SRC_DIR, "radiance_cascade/*.h"),
			path.join(SRC_DIR, "radiance_cascade/*.cpp"),
		},

		["src/deferred"] = {
			path.join(SRC_DIR, "deferred/*.h"),
			path.join(SRC_DIR, "deferred/*.cpp"),
		},

		["src/ffx_intg"] = {
			path.join(SRC_DIR, "ffx_intg/*.h"),
			path.join(SRC_DIR, "ffx_intg/*.cpp"),
		},
		
		["shaders/ffx"] = {
			path.join(SRC_DIR, 	"shaders/ffx/*.h"),
		},

		["shaders/inc"] = {
			path.join(SRC_DIR, 	"shaders/*.h"),
		},

		["shaders"] = {
			path.join(SRC_DIR,	"shaders/*.glsl"),
		}

		



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
		"fast_obj",
		"tracy",
		"bgfx_common",
		path.join(METIS_DIR, "lib", "metis.lib"),
		path.join(METIS_DIR, "lib", "gklib.lib"),
		path.join(VK_SDK_DIR, "Lib", "vulkan-1.lib"),
		path.join(KTX_SDK_DIR, "lib", "ktx.lib"),
	}

	filter "action:vs*"
		debugdir ""

	filter "system:windows"
		defines {
			"WIN32_LEAN_AND_MEAN",
		}

   	filter "configurations:debug"
	   optimize "Debug"
	   symbols "On"
	   links {
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_brixelizer_x64d.lib"),
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_backend_vk_x64d.lib"),
	   }
	
	filter "configurations:debug_prof"
		optimize "Debug"
		symbols "On"
		links {
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_brixelizer_x64d.lib"),
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_backend_vk_x64d.lib"),
	   	}

   	filter "configurations:release"
	   	optimize "Full"
	   	links {
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_brixelizer_x64.lib"),
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_backend_vk_x64.lib"),
	   	}
	   
	filter "configurations:release_prof"
		optimize "Full"
		links {
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_brixelizer_x64.lib"),
			path.join(FFX_SDK_DIR, "bin/ffx_sdk", "ffx_backend_vk_x64.lib"),
	   	}
	
	using_vulkan()
	using_glfw()
	using_bx()

	tracy_filter()

	set_bx_compat()

	common_filter()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
	custom_build_glsl()

-- group projects
project("bx").group 			= "3rd"
project("bimg").group 			= "3rd"
project("glfw").group 			= "3rd"
project("imgui").group 			= "3rd"
project("meshoptimizer").group 	= "3rd"
project("tracy").group 			= "3rd"
project("fast_obj").group 		= "3rd"
project("bgfx_common").group 	= "3rd"
