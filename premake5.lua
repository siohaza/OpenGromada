newoption {
	trigger = "sdl3-dir",
	value = "PATH",
	description = "Path to SDL3"
}

newoption {
	trigger = "steam-sdk",
	value = "PATH",
	description = "Path to a Steamworks SDK"
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

local steam_sdk = _OPTIONS["steam-sdk"] or os.getenv("STEAM_SDK_DIR")
if not steam_sdk and os.isfile("../SteamworksSDK/public/steam/steam_api.h") then
	steam_sdk = "../SteamworksSDK"
end
if steam_sdk then
	steam_sdk = path.getabsolute(steam_sdk)
	if not os.isfile(path.join(steam_sdk, "public/steam/steam_api.h")) then
		error("Not a Steamworks SDK: " .. steam_sdk)
	end
	print("Steamworks: enabled (" .. steam_sdk .. "); x86/x64 only")
else
	print("Steamworks: disabled. Store is local only")
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

	if steam_sdk then
		filter "platforms:x86 or x64"
			defines { "ALIEN_HAVE_STEAMWORKS=1" }
			externalincludedirs { path.join(steam_sdk, "public") }
			buildoptions { "/Zc:__cplusplus" }
	end

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

	if steam_sdk then
		filter "platforms:x86"
			postbuildcommands {
				'{COPYFILE} "' .. path.join(steam_sdk, "redistributable_bin/steam_api.dll") .. '" "%{cfg.targetdir}"'
			}

		filter "platforms:x64"
			postbuildcommands {
				'{COPYFILE} "' .. path.join(steam_sdk, "redistributable_bin/win64/steam_api64.dll") .. '" "%{cfg.targetdir}"'
			}
	end

	filter "platforms:ARM64"
		libdirs { path.join(sdl3_dir, "lib/arm64") }
		postbuildcommands {
			'{COPYFILE} "' .. path.join(sdl3_dir, "lib/arm64/SDL3.dll") .. '" "%{cfg.targetdir}"'
		}

	filter {}
