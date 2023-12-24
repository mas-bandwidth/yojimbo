
yojimbo_version = "1.0"

solution "Yojimbo"
    kind "ConsoleApp"
    language "C++"
    configurations { "Debug", "Release" }
    includedirs { ".", "mbedtls", "sodium", "tlsf", "certs", "netcode", "reliable" }
    if os.istarget "windows" then
        libdirs { "./windows" }
    else
        targetdir "bin/"  
    end
    rtti "Off"
    warnings "Extra"
    floatingpoint "Fast"
    configuration "Debug"
        symbols "On"
        defines { "YOJIMBO_DEBUG", "NETCODE_DEBUG", "RELIABLE_DEBUG" }
    configuration "Release"
        optimize "Speed"
        defines { "YOJIMBO_RELEASE", "NETCODE_RELEASE", "RELIABLE_RELEASE" }

project "netcode"
    kind "StaticLib"
    language "C"
    defines { "NETCODE_ENABLE_TESTS=1" }
    files { "netcode/netcode.c", "netcode/netcode.h" }

project "reliable"
    kind "StaticLib"
    language "C"
    defines { "NETCODE_ENABLE_TESTS=1" }
    files { "reliable/reliable.c", "reliable/reliable.h" }

project "tslf"
    kind "StaticLib"
    language "C"
    files { "tlsf/tlsf.c", "tlsf/tlsf.h" }

project "certs"
    kind "StaticLib"
    language "C"
    files { "certs/certs.c", "certs/certs.h" }

project "yojimbo"
    kind "StaticLib"
    links { "reliable", "netcode", "tlsf", "certs" }
    defines { "NETCODE_ENABLE_TESTS=1", "RELIABLE_ENABLE_TESTS=1" }
    files { "yojimbo.h", "yojimbo.cpp" }
