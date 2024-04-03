-- bgfx common

print("current bgfx_common path: " .. BGFX_COMMON_DIR);

os.chdir(_WORKING_DIR)
-- bgfx_common
project "bgfx_common"
	kind "StaticLib"
	language "C++"
	cppdialect "C++17"
    exceptionhandling "Off"
	rtti "Off"
	
    files {
		path.join(BGFX_COMMON_DIR, "entry/*.h"),
		path.join(BGFX_COMMON_DIR, "entry/*.cpp"),
	}

    includedirs {
        path.join(BGFX_COMMON_DIR,  "entry"),
        path.join(BX_DIR,	 	    "include"),
    }
	
    common_filter()
	using_glfw()
	using_bx()
    set_bx_compat()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
