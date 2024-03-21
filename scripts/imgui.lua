print("current img path: " .. IMGUI_DIR);

os.chdir(_WORKING_DIR)
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
	common_filter()
