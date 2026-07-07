
newoption {
    trigger     = "sodium",
    value       = "impl",
    description = "Which libsodium to link against",
    default     = "builtin",
    allowed     = {
        { "builtin", "Use the bundled minimal libsodium in sodium/ (default; no external dependency)" },
        { "system",  "Link the system-installed libsodium (-lsodium)" },
    }
}

-- Default to the bundled libsodium on every platform. Pass --sodium=system to
-- premake to link the system-installed libsodium instead.
local use_system_sodium = ( _OPTIONS["sodium"] == "system" )

-- Link yojimbo and its dependencies, selecting the libsodium backend.
local function link_dependencies()
    links { "yojimbo", "tlsf", "netcode", "reliable" }
    if use_system_sodium then
        links { "sodium" }
        filter "system:not windows"
            libdirs { "/opt/homebrew/lib" }
        filter {}
    else
        links { "sodium-builtin" }
    end
end

solution "Yojimbo"
    kind "ConsoleApp"
    language "C++"
    configurations { "Debug", "Release" }
    if os.istarget "windows" then
        platforms { "Win32", "x64", "ARM64" }
    end
    includedirs { ".", "include", "sodium", "tlsf", "netcode", "reliable", "serialize" }
    if not os.istarget "windows" then
        targetdir "bin/"
    end
    rtti "Off"
    warnings "Extra"
    floatingpoint "Fast"
    filter "configurations:Debug"
        symbols "On"
        defines { "YOJIMBO_DEBUG", "NETCODE_DEBUG", "RELIABLE_DEBUG", "SERIALIZE_DEBUG" }
    filter "configurations:Release"
        symbols "Off"
        optimize "Speed"
        defines { "YOJIMBO_RELEASE", "NETCODE_RELEASE", "RELIABLE_RELEASE", "SERIALIZE_RELEASE" }

    filter "platforms:Win32"
        architecture "x86"
    filter "platforms:x64"
        architecture "x86_64"
    filter "platforms:ARM64"
        architecture "ARM64"

-- The bundled minimal libsodium. Built on every platform unless --sodium=system
-- is given, in which case it is skipped and the system libsodium is linked.
if not use_system_sodium then
project "sodium-builtin"
    kind "StaticLib"
    language "C"
    warnings "Off"
    files {
        "sodium/**.c",
        "sodium/**.h",
    }
    filter { "action:gmake*" }
        buildoptions { "-Wno-unused-parameter", "-Wno-unused-function", "-Wno-unknown-pragmas", "-Wno-unused-variable", "-Wno-type-limits" }
    filter {}
end

project "netcode"
    kind "StaticLib"
    language "C"
    defines { "NETCODE_ENABLE_TESTS=1" }
    files { "netcode/netcode.c", "netcode/netcode.h" }

project "reliable"
    kind "StaticLib"
    language "C"
    defines { "RELIABLE_ENABLE_TESTS=1" }
    files { "reliable/reliable.c", "reliable/reliable.h" }

project "tlsf"
    kind "StaticLib"
    language "C"
    files { "tlsf/tlsf.c", "tlsf/tlsf.h" }

project "yojimbo"
    kind "StaticLib"
    files { "include/*.h", "source/*.cpp" }

project "client"
    files { "client.cpp", "shared.h" }
    link_dependencies()

project "server"
    files { "server.cpp", "shared.h" }
    link_dependencies()

project "loopback"
    files { "loopback.cpp", "shared.h" }
    link_dependencies()

project "soak"
    files { "soak.cpp", "shared.h" }
    link_dependencies()

project "test"
    files { "test.cpp" }
    defines { "SERIALIZE_ENABLE_TESTS=1" }
    link_dependencies()

newaction
{
    trigger     = "clean",

    description = "Clean all build files and output",

    execute = function ()

        files_to_delete =
        {
            "Makefile",
            "*.make",
            "*.txt",
            "*.zip",
            "*.tar.gz",
            "*.db",
            "*.opendb",
            "*.vcproj",
            "*.vcxproj",
            "*.vcxproj.user",
            "*.vcxproj.filters",
            "*.sln",
            "*.xcodeproj",
            "*.xcworkspace"
        }

        directories_to_delete =
        {
            "obj",
            "ipch",
            "bin",
            ".vs",
            "Debug",
            "Release",
            "release",
            "cov-int",
            "docker/yojimbo",
            "valgrind/yojimbo",
            "docs",
            "xml"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if not os.istarget "windows" then
            os.execute "find . -name .DS_Store -delete"
            for i,v in ipairs( files_to_delete ) do
              os.execute( "rm -f " .. v )
            end
        else
            for i,v in ipairs( files_to_delete ) do
              os.execute( "del /F /Q  " .. v )
            end
        end

    end
}
