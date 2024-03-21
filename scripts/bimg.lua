os.chdir(_WORKING_DIR)
print("current bimg path: " .. BIMG_DIR);

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
