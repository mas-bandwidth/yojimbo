
libyojimbo_version = "0.3.0-Preview10"

if os.is "windows" then
    debug_libs = { "sodium-debug", "mbedtls-debug", "mbedx509-debug", "mbedcrypto-debug" }
    release_libs = { "sodium-release", "mbedtls-release", "mbedx509-release", "mbedcrypto-release" }
else
    debug_libs = { "sodium", "mbedtls", "mbedx509", "mbedcrypto" }
    release_libs = debug_libs
end

solution "Yojimbo"
    kind "ConsoleApp"
    language "C++"
    platforms { "x64" }
    configurations { "Debug", "Release", "Debug_Secure", "Release_Secure" }
    if os.is "windows" then
        includedirs { ".", "./windows" }
        libdirs { "./windows" }
    else
        includedirs { ".", "/usr/local/include" }       -- for clang scan-build only. for some reason it needs this to work =p
        targetdir "bin/"  
    end
    rtti "Off"
    flags { "ExtraWarnings", "StaticRuntime", "FloatFast", "EnableSSE2" }
    configuration "Debug"
        symbols "On"
        defines { "DEBUG", "_DEBUG" }
        links { debug_libs }
    configuration "Release"
        optimize "Speed"
        defines { "NDEBUG" }
        links { release_libs }
    configuration "Debug_Secure"
        symbols "On"
        defines { "YOJIMBO_SECURE_MODE=1", "DEBUG", "_DEBUG" }
        links { debug_libs }
    configuration "Release_Secure"
        optimize "Speed"
        defines { "YOJIMBO_SECURE_MODE=1", "NDEBUG" }
        links { release_libs }
        
project "test"
    files { "tests/test.cpp" }
    links { "yojimbo" }

project "info"
    files { "tests/info.cpp" }
    links { "yojimbo" }

project "yojimbo"
    kind "StaticLib"
    files { "yojimbo.h", "yojimbo.cpp", "yojimbo_*.h", "yojimbo_*.cpp" }

project "client"
    files { "tests/client.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "server"
    files { "tests/server.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "secure_client"
    files { "tests/secure_client.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "secure_server"
    files { "tests/secure_server.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "client_server"
    files { "tests/client_server.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "soak"
    files { "tests/soak.cpp", "tests/shared.h" }
    links { "yojimbo" }

project "profile"
    files { "tests/profile.cpp", "tests/shared.h" }
    links { "yojimbo" }

if not os.is "windows" then

    -- MacOSX and Linux.
    
    newaction
    {
        trigger     = "test",
        description = "Build and run all unit tests",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 test" == 0 then
                os.execute "./bin/test"
            end
        end
    }

    newaction
    {
        trigger     = "info",
        description = "Build and run network info utility",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 info" == 0 then
                os.execute "./bin/info"
            end
        end
    }

    newaction
    {
        trigger     = "client_server",
        description = "Build and run client/server testbed",     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 client_server" == 0 then
                os.execute "./bin/client_server"
            end
        end
    }

    newoption 
    {
        trigger     = "serverAddress",
        value       = "IP[:port]",
        description = "Specify the server address that the client should connect to",
    }

    newaction
    {
        trigger     = "client",
        description = "Build and run client",
        valid_kinds = premake.action.get("gmake").valid_kinds,
        valid_languages = premake.action.get("gmake").valid_languages,
        valid_tools = premake.action.get("gmake").valid_tools,
     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 client" == 0 then
                if _OPTIONS["serverAddress"] then
                    os.execute( "./bin/client " .. _OPTIONS["serverAddress"] )
                else
                    os.execute "./bin/client"
                end
            end
        end
    }

    newaction
    {
        trigger     = "server",
        description = "Build and run server",     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 server" == 0 then
                os.execute "./bin/server"
            end
        end
    }

    newaction
    {
        trigger     = "secure_server",
        description = "Build and run secure server",     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 secure_server" == 0 then
                os.execute "./bin/secure_server"
            end
        end
    }

	newaction
	{
		trigger     = "docker",
		description = "Build and run a yojimbo server inside a docker container",
		execute = function ()
            os.execute "docker run --rm --privileged alpine hwclock -s" -- workaround for clock getting out of sync on macos. see https://docs.docker.com/docker-for-mac/troubleshoot/#issues
            os.execute "rm -rf docker/libyojimbo && mkdir -p docker/libyojimbo && mkdir -p docker/libyojimbo/tests && cp *.h docker/libyojimbo && cp *.cpp docker/libyojimbo && cp premake5.lua docker/libyojimbo && cp tests/* docker/libyojimbo/tests && cp -R rapidjson docker/libyojimbo && cd docker && docker build -t \"networkprotocol:yojimbo-server\" . && rm -rf libyojimbo && docker run -ti -p 40000:40000/udp networkprotocol:yojimbo-server"
		end
	}

    newaction
    {
        trigger     = "matcher",
        description = "Build and run the matchmaker web service inside a docker container",
        execute = function ()
            os.execute "docker run --rm --privileged alpine hwclock -s" -- workaround for clock getting out of sync on macos. see https://docs.docker.com/docker-for-mac/troubleshoot/#issues
            os.execute "cd docker/matcher && docker build -t networkprotocol:yojimbo-matcher . && docker run -ti -p 8080:8080 networkprotocol:yojimbo-matcher"
        end
    }

    newaction
    {
        trigger     = "secure_client",
        description = "Build and run secure client and connect to a server via the matcher",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 secure_client" == 0 then
                os.execute "./bin/secure_client"
            end
        end
    }

    newaction
    {
        trigger     = "stress",
        description = "Launch 64 secure client instances to stress the matcher and server",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 secure_client" == 0 then
                for i = 0, 63 do
                    os.execute "./bin/secure_client &"
                end
            end
        end
    }

    newaction
    {
        trigger     = "soak",
        description = "Build and run soak test",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 soak" == 0 then
                os.execute "./bin/soak"
            end
        end
    }

    newaction
    {
        trigger     = "profile",
        description = "Build and run profile testbed",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 profile" == 0 then
                os.execute "./bin/profile"
            end
        end
    }

    newaction
    {
        trigger     = "cppcheck",
        description = "Run cppcheck over the source code and write to cppcheck.txt",
        execute = function ()
            os.execute "cppcheck *.h *.cpp --force --std=c++03 --language=c++ --quiet -U min -U max 2>&1 --config-exclude=rapidjson --suppress=incorrectStringBooleanError --suppress=cstyleCast --suppress=unusedFunction --suppress=unusedStructMember --suppress=variableScope --suppress=memsetClassFloat --enable=warning --enable=performance --enable=style --platform=native -j 32 | tee -a cppcheck.txt"
        end
    }

    newaction
    {
        trigger     = "scan-build",
        description = "Run clang scan-build over the project",
        execute = function ()
            os.execute "premake5 clean"
            os.execute "premake5 gmake"
            os.execute "scan-build make all -j32"
        end
    }

    newaction
    {
        trigger     = "coverity",                            -- i can't get this to work on latest MacOS. if you know how please let me know!
        description = "Run coverity over the project",
        execute = function ()
            os.execute "premake5 clean"
            os.execute "premake5 gmake"
            os.execute "cov-build --dir cov-int make -j32 all"
        end
    }

    newaction
    {
        trigger     = "loc",
        description = "Count lines of code",
        execute = function ()
            os.execute "wc -l *.h *.cpp"
        end
    }

    newaction
    {
        trigger     = "release",
        description = "Create a release of this project",
        execute = function ()
            _ACTION = "clean"
            premake.action.call( "clean" )
            files_to_zip = "README.md BUILDING.md CHANGES.md ROADMAP.md *.cpp *.h premake5.lua docker tests rapidjson windows"
            os.execute( "rm -rf *.zip *.tar.gz" );
            os.execute( "rm -rf docker/libyojimbo" );
            os.execute( "zip -9r libyojimbo-" .. libyojimbo_version .. ".zip " .. files_to_zip )
            os.execute( "unzip libyojimbo-" .. libyojimbo_version .. ".zip -d libyojimbo-" .. libyojimbo_version );
            os.execute( "tar -zcvf libyojimbo-" .. libyojimbo_version .. ".tar.gz libyojimbo-" .. libyojimbo_version );
            os.execute( "rm -rf libyojimbo-" .. libyojimbo_version );
            os.execute( "mkdir -p release" );
            os.execute( "mv libyojimbo-" .. libyojimbo_version .. ".zip release" );
            os.execute( "mv libyojimbo-" .. libyojimbo_version .. ".tar.gz release" );
            os.execute( "echo" );
            os.execute( "echo \"*** SUCCESSFULLY CREATED RELEASE - libyojimbo-" .. libyojimbo_version .. " *** \"" );
            os.execute( "echo" );
        end
    }

    newaction
    {
        trigger     = "sublime",
        description = "Create sublime project",
        execute = function ()
            os.execute "cp .sublime yojimbo.sublime-project"
        end
    }

    newaction
    {
        trigger     = "docs",
        description = "Build documentation",
        execute = function ()
            if os.get() == "macosx" then
                os.execute "doxygen doxygen.config && open docs/html/index.html"
            else
                os.execute "doxygen doxygen.config"
            end
        end
    }

else

    -- Windows

    newaction
    {
        trigger     = "solution",
        description = "Open Yojimbo.sln",
        execute = function ()
            os.execute "premake5 vs2015"
            os.execute "start Yojimbo.sln"
        end
    }

	newaction
	{
		trigger     = "docker",
		description = "Build and run a yojimbo server inside a docker container",
		execute = function ()
			os.execute "cd docker && copyFiles.bat && buildServer.bat && runServer.bat"
		end
	}

    newaction
    {
        trigger     = "matcher",
        description = "Build and run the matchmaker web service inside a docker container",
        execute = function ()
            os.execute "cd docker\\matcher && docker build -t networkprotocol:yojimbo-matcher . && docker run -ti -p 8080:8080 networkprotocol:yojimbo-matcher"
        end
    }

    newaction
    {
        trigger     = "stress",
        description = "Launch 64 connect instances to stress the matcher and server",
        execute = function ()
            for i = 0, 63 do
                os.execute "if exist bin\\x64\\Debug\\connect.exe ( start /B bin\\x64\\Debug\\connect ) else ( echo could not find bin\\x64\\Debug\\connect.exe )"
            end
        end
    }

    newaction
    {
        trigger     = "docs",
        description = "Build documentation",
        execute = function ()
            os.execute "doxygen doxygen.config && start docs\\html\\index.html"
        end
    }

end

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
            "docker/libyojimbo",
            "docs",
            "xml"
        }

        for i,v in ipairs( directories_to_delete ) do
          os.rmdir( v )
        end

        if not os.is "windows" then
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
