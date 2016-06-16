
solution "Yojimbo"
    platforms { "x64" }
    includedirs { "." }
    if not os.is "windows" then
        targetdir "bin/"
    end
    configurations { "Debug", "Release" }
    flags { "ExtraWarnings", "FloatFast" }
    rtti "Off"
    configuration "Debug"
        flags { "Symbols" }
        defines { "DEBUG" }
    configuration "Release"
        optimize "Speed"
        defines { "NDEBUG" }

project "test"
    language "C++"
    kind "ConsoleApp"
    files { "test.cpp" }
    links { "yojimbo", "sodium" }

project "network_info"
    language "C++"
    kind "ConsoleApp"
    files { "network_info.cpp" }
    links { "yojimbo", "sodium" }

project "yojimbo"
    language "C++"
    kind "StaticLib"
    files { "yojimbo.h", "yojimbo.cpp", "yojimbo_*.h", "yojimbo_*.cpp" }
    links { "sodium" }

project "client"
    language "C++"
    kind "ConsoleApp"
    files { "client.cpp", "shared.h" }
    links { "yojimbo", "sodium" }

project "server"
    language "C++"
    kind "ConsoleApp"
    files { "server.cpp", "shared.h" }
    links { "yojimbo", "sodium" }

project "client_server"
    language "C++"
    kind "ConsoleApp"
    files { "client_server.cpp", "shared.h" }
    links { "yojimbo", "sodium" }

if _ACTION == "clean" then
    os.rmdir "obj"
    if not os.is "windows" then
        os.execute "rm -rf bin"
        os.execute "rm -rf obj"
        os.execute "rm -f Makefile"
        os.execute "rm -f *.zip"
        os.execute "rm -f *.make"
        os.execute "rm -f test"
        os.execute "rm -f network_info"
        os.execute "rm -f client"
        os.execute "rm -f server"
        os.execute "rm -f client_server"
        os.execute "find . -name .DS_Store -delete"
    else
        os.rmdir "ipch"
		os.rmdir "bin"
		os.rmdir ".vs"
        os.rmdir "Debug"
        os.rmdir "Release"
        os.execute "del /F /Q *.zip"
        os.execute "del /F /Q *.db"
        os.execute "del /F /Q *.opendb"
        os.execute "del /F /Q *.vcproj"
        os.execute "del /F /Q *.vcxproj"
        os.execute "del /F /Q *.vcxproj.user"
        os.execute "del /F /Q *.sln"
    end
end

if not os.is "windows" then

    newaction
    {
        trigger     = "zip",
        description = "Zip up archive of this project",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            _ACTION = "clean"
            premake.action.call( "clean" )
            os.execute "zip -9r Yojimbo.zip BUILDING *.cpp *.h premake5.lua sodium sodium.lib"
        end
    }

    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 test" == 0 then
                os.execute "./bin/test"
            end
        end
    }

    newaction
    {
        trigger     = "info",
        description = "Build and run network info utility",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 network_info" == 0 then
                os.execute "./bin/network_info"
            end
        end
    }

    newaction
    {
        trigger     = "yojimbo",
        description = "Build yojimbo client/server network protocol library",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "make -j4 yojimbo"
        end
    }

    newaction
    {
        trigger     = "cs",
        description = "Build and run client/server testbed",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 client_server" == 0 then
                os.execute "./bin/client_server"
            end
        end
    }

    newaction
    {
        trigger     = "client",
        description = "Build and run client",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 client" == 0 then
                os.execute "./bin/client"
            end
        end
    }

    newaction
    {
        trigger     = "server",
        description = "Build and run server",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            if os.execute "make -j4 server" == 0 then
                os.execute "./bin/server"
            end
        end
    }

end
