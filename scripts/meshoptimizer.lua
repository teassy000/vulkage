print("current mopt path: " .. MESH_OPT_DIR);

os.chdir(_WORKING_DIR)
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
	common_filter()
