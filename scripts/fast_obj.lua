print("current fast_obj path: " .. FAST_OBJ_DIR);

os.chdir(_WORKING_DIR)
-- fast_obj
project "fast_obj"
	kind "StaticLib"
	language "C"
	files {
		path.join(FAST_OBJ_DIR, "fast_obj.h"),
		path.join(FAST_OBJ_DIR, "fast_obj.c"),
	}
	
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
	common_filter()