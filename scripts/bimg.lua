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
		path.join(BIMG_DIR, "src/**"),

		path.join(BIMG_DIR, "3rdparty/**"),
	}

	includedirs
	{
		path.join(BX_DIR, "include"),
		path.join(BIMG_DIR, "include"),
		path.join(BIMG_DIR, "3rdparty"),

		-- astc-encoder
		path.join(BIMG_DIR, "3rdparty/astc-encoder/include"),
		-- edtaa3
		path.join(BIMG_DIR, "3rdparty/edtaa3"),
		-- etc1
		path.join(BIMG_DIR, "3rdparty/etc1"),
		-- etc2
		path.join(BIMG_DIR, "3rdparty/etc2"),
		-- iqa
		path.join(BIMG_DIR, "3rdparty/iqa/include"),
		-- libsquish
		path.join(BIMG_DIR, "3rdparty/libsquish"),
		-- lodepng
		path.join(BIMG_DIR, "3rdparty/lodepng"),
		-- nvtt
		path.join(BIMG_DIR, "3rdparty/nvtt"),
		-- pvrtc
		path.join(BIMG_DIR, "3rdparty/pvrtc"),
		-- stb
		path.join(BIMG_DIR, "3rdparty/stb"),
		-- tinyexr
		path.join(BIMG_DIR, "3rdparty/tinyexr/deps/miniz"),
		path.join(BIMG_DIR, "3rdparty/tinyexr"),
	}

	using_bx()
	set_bx_compat()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
	common_filter()
