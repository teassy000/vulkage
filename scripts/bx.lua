
os.chdir(_WORKING_DIR)
print("current bx path: " .. BX_DIR);

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

