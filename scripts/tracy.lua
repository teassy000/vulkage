print("current tracy path: " .. TRACY_DIR);

os.chdir(_WORKING_DIR)

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

-- tracy
project "tracy"
	kind "StaticLib"
	language "C"
	files {
		path.join(TRACY_DIR, "public/TracyClient.cpp"),
	}
	
    tracy_filter()
	enable_msvc_muiltithreaded()
	disable_msvc_crt_warning()
	common_filter()