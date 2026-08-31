newoption {
	trigger = "sdl3-dir",
	value = "PATH",
	description = "Path to an extracted SDL3 Visual C++ development package"
}

if not _ACTION then
	return
end

local sdl3_dir = _OPTIONS["sdl3-dir"] or os.getenv("SDL3_DIR") or "dependencies/SDL3"
sdl3_dir = path.getabsolute(sdl3_dir)

local required_sdl3_files = {
	"include/SDL3/SDL.h",
	"include/SDL3/SDL_version.h",
	"lib/x86/SDL3.lib",
	"lib/x86/SDL3.dll",
	"lib/x64/SDL3.lib",
	"lib/x64/SDL3.dll",
	"lib/arm64/SDL3.lib",
	"lib/arm64/SDL3.dll"
}

for _, file in ipairs(required_sdl3_files) do
	if not os.isfile(path.join(sdl3_dir, file)) then
		error("SDL3 file not found: " .. path.join(sdl3_dir, file))
	end
end

local version_file = assert(io.open(path.join(sdl3_dir, "include/SDL3/SDL_version.h"), "r"))
local version_text = version_file:read("*a")
version_file:close()
local sdl_major = tonumber(version_text:match("#define%s+SDL_MAJOR_VERSION%s+(%d+)"))
local sdl_minor = tonumber(version_text:match("#define%s+SDL_MINOR_VERSION%s+(%d+)"))
if sdl_major ~= 3 or not sdl_minor or sdl_minor < 4 then
	error("Alien Shooter requires SDL 3.4 or newer")
end

workspace "AlienShooter"
	location "build/premake"
	configurations { "Debug", "Release" }
	platforms { "x86", "x64", "ARM64" }
	defaultplatform "x64"
	startproject "AlienShooter"
	system "windows"
	systemversion "latest"
	staticruntime "On"
	multiprocessorcompile "On"

	filter "platforms:x86"
		architecture "x86"

	filter "platforms:x64"
		architecture "x86_64"

	filter "platforms:ARM64"
		architecture "ARM64"

	filter "configurations:Debug"
		defines { "_DEBUG" }
		runtime "Debug"
		symbols "On"
		optimize "Off"

	filter "configurations:Release"
		defines { "NDEBUG" }
		runtime "Release"
		symbols "Off"
		optimize "Speed"

	filter {}

local function configure_project()
	language "C++"
	cppdialect "C++20"
	includedirs { "src" }
	externalincludedirs { path.join(sdl3_dir, "include") }
	defines { "NOMINMAX" }
	objdir "build/premake/obj/%{prj.name}/%{cfg.platform}/%{cfg.buildcfg}"

	filter "files:**.cpp"
		forceincludes { "src/util/compat.h" }

	filter {}
end

project "alien_core"
	kind "StaticLib"
	targetdir "build/premake/lib/%{cfg.platform}/%{cfg.buildcfg}"
	files {
		"src/**.c",
		"src/**.cpp",
		"src/**.h"
	}
	removefiles { "src/main.cpp" }
	configure_project()

project "AlienShooter"
	kind "ConsoleApp"
	targetname "AlienShooter"
	targetdir "build/premake/bin/%{cfg.platform}/%{cfg.buildcfg}"
	debugdir "%{cfg.targetdir}"
	files {
		"src/main.cpp",
		"resources/AlienShooter.rc",
		"resources/AlienShooter.res"
	}
	links { "alien_core", "SDL3" }
	linkoptions { '"' .. path.getabsolute("resources/AlienShooter.res") .. '"' }
	configure_project()

	filter "files:resources/AlienShooter.rc or resources/AlienShooter.res"
		buildaction "None"

	filter "platforms:x86"
		libdirs { path.join(sdl3_dir, "lib/x86") }
		postbuildcommands {
			'{COPYFILE} "' .. path.join(sdl3_dir, "lib/x86/SDL3.dll") .. '" "%{cfg.targetdir}"'
		}

	filter "platforms:x64"
		libdirs { path.join(sdl3_dir, "lib/x64") }
		postbuildcommands {
			'{COPYFILE} "' .. path.join(sdl3_dir, "lib/x64/SDL3.dll") .. '" "%{cfg.targetdir}"'
		}

	filter "platforms:ARM64"
		libdirs { path.join(sdl3_dir, "lib/arm64") }
		postbuildcommands {
			'{COPYFILE} "' .. path.join(sdl3_dir, "lib/arm64/SDL3.dll") .. '" "%{cfg.targetdir}"'
		}

	filter {}
