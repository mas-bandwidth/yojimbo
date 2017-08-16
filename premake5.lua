
yojimbo_version = "1.0"

libs = { "sodium", "mbedtls", "mbedx509", "mbedcrypto" }

solution "Yojimbo"
    kind "ConsoleApp"
    language "C++"
    platforms { "x64" }
    configurations { "Debug", "Release" }
    if os.is "windows" then
        includedirs { ".", "./windows", "netcode.io", "reliable.io" }
        libdirs { "./windows" }
    else
        includedirs { ".", "/usr/local/include", "netcode.io", "reliable.io" }
        targetdir "bin/"  
    end
    rtti "Off"
    links { libs }
    flags { "ExtraWarnings", "FloatFast", "EnableSSE2" }
    configuration "Debug"
        symbols "On"
    configuration "Release"
        optimize "Speed"
        defines { "NDEBUG" }
        
project "test"
    files { "test.cpp" }
    links { "yojimbo" }

project "yojimbo"
    kind "StaticLib"
    defines { "NETCODE_ENABLE_TESTS=1", "RELIABLE_ENABLE_TESTS=1" }
    files { "yojimbo.h", "yojimbo.cpp", "tlsf/tlsf.h", "tlsf/tlsf.c", "netcode.io/netcode.c", "netcode.io/netcode.h", "reliable.io/reliable.c", "reliable.io/reliable.h" }

project "client"
    files { "client.cpp", "shared.h" }
    links { "yojimbo" }

project "server"
    files { "server.cpp", "shared.h" }
    links { "yojimbo" }

project "secure_client"
    files { "secure_client.cpp", "shared.h" }
    links { "yojimbo" }

project "secure_server"
    files { "secure_server.cpp", "shared.h" }
    links { "yojimbo" }

project "client_server"
    files { "client_server.cpp", "shared.h" }
    links { "yojimbo" }

project "loopback"
    files { "loopback.cpp", "shared.h" }
    links { "yojimbo" }

project "soak"
    files { "soak.cpp", "shared.h" }
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
        trigger     = "client_server",
        description = "Build and run client/server test",     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 client_server" == 0 then
                os.execute "./bin/client_server"
            end
        end
    }

    newaction
    {
        trigger     = "loopback",
        description = "Build and run loopback test",     
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 loopback" == 0 then
                os.execute "./bin/loopback"
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
            os.execute "rm -rf docker/yojimbo && mkdir -p docker/yojimbo && mkdir -p docker/yojimbo/tests && cp *.h docker/yojimbo && cp *.cpp docker/yojimbo && cp premake5.lua docker/yojimbo && cp -R reliable.io docker/yojimbo && cp -R netcode.io docker/yojimbo && cp -R tlsf docker/yojimbo && cd docker && docker build -t \"networkprotocol:yojimbo-server\" . && rm -rf yojimbo && docker run -ti -p 40000:40000/udp networkprotocol:yojimbo-server"
		end
	}

    newaction
    {
        trigger     = "valgrind",
        description = "Run valgrind over tests inside docker",
        execute = function ()
            os.execute "rm -rf valgrind/yojimbo && mkdir -p valgrind/yojimbo && mkdir -p valgrind/yojimbo/tests && cp *.h valgrind/yojimbo && cp *.cpp valgrind/yojimbo && cp premake5.lua valgrind/yojimbo && cp -R reliable.io valgrind/yojimbo && cp -R netcode.io valgrind/yojimbo && cp -R tlsf valgrind/yojimbo && cd valgrind && docker build -t \"networkprotocol:yojimbo-valgrind\" . && rm -rf netcode.io && docker run -ti networkprotocol:yojimbo-valgrind"
        end
    }

    newaction
    {
        trigger     = "matcher",
        description = "Build and run the matchmaker web service inside a docker container",
        execute = function ()
            os.execute "docker run --rm --privileged alpine hwclock -s" -- workaround for clock getting out of sync on macos. see https://docs.docker.com/docker-for-mac/troubleshoot/#issues
            os.execute "cd matcher && docker build -t networkprotocol:yojimbo-matcher . && docker run -ti -p 8080:8080 networkprotocol:yojimbo-matcher"
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
        trigger     = "cppcheck",
        description = "Run cppcheck over the source code and write to cppcheck.txt",
        execute = function ()
            os.execute "cppcheck *.h *.cpp --force --std=c++03 --language=c++ --quiet -U min -U max 2>&1 --config-exclude=tlsf --suppress=incorrectStringBooleanError --suppress=cstyleCast --suppress=unusedFunction --suppress=unusedStructMember --suppress=variableScope --suppress=memsetClassFloat --enable=warning --enable=performance --enable=style --platform=native -j 32 | tee -a cppcheck.txt"
        end
    }

    newaction
    {
        trigger     = "scan-build",
        description = "Run clang scan-build over the project",
        execute = function ()
            os.execute "premake5 clean && premake5 gmake && scan-build make all -j32"
        end
    }

    newaction
    {
        trigger     = "coverity",
        description = "Integrate latest code into coverity_scan so it gets coverity scanned by travis job",
        execute = function ()
            os.execute "git checkout coverity_scan && git merge master && git push && git checkout master"
        end
    }

    newaction
    {
        trigger     = "loc",
        description = "Count lines of code",
        execute = function ()
            os.execute "wc -l *.h *.cpp netcode.io/*.c netcode.io/*.h reliable.io/*.c reliable.io/*.h"
        end
    }

    newaction
    {
        trigger     = "release",
        description = "Create a release of this project",
        execute = function ()
            _ACTION = "clean"
            premake.action.call( "clean" )
            files_to_zip = "README.md BUILDING.md CHANGES.md ROADMAP.md *.cpp *.h premake5.lua docker tests tlsf windows"
            -- todo: need to update this so it works with netcode.io and reliable.io sub-projects
            os.execute( "rm -rf *.zip *.tar.gz" );
            os.execute( "rm -rf docker/yojimbo" );
            os.execute( "zip -9r yojimbo-" .. yojimbo_version .. ".zip " .. files_to_zip )
            os.execute( "unzip yojimbo-" .. yojimbo_version .. ".zip -d yojimbo-" .. yojimbo_version );
            os.execute( "tar -zcvf yojimbo-" .. yojimbo_version .. ".tar.gz yojimbo-" .. yojimbo_version );
            os.execute( "rm -rf yojimbo-" .. yojimbo_version );
            os.execute( "mkdir -p release" );
            os.execute( "mv yojimbo-" .. yojimbo_version .. ".zip release" );
            os.execute( "mv yojimbo-" .. yojimbo_version .. ".tar.gz release" );
            os.execute( "echo" );
            os.execute( "echo \"*** SUCCESSFULLY CREATED RELEASE - yojimbo-" .. yojimbo_version .. " *** \"" );
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

    newaction
    {
        trigger     = "update_submodules",
        description = "Updates to latest code for netcode.io and reliable.io",
        execute = function ()
            os.execute "git pull"
            os.execute "git submodule update --remote --merge"
            os.execute "git add *"
            os.execute "git commit -am \"update submodules\""
            os.execute "git push"
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
            os.execute "cd matcher && docker build -t networkprotocol:yojimbo-matcher . && docker run -ti -p 8080:8080 networkprotocol:yojimbo-matcher"
        end
    }

    newaction
    {
        trigger     = "stress",
        description = "Launch 64 secure client instances to stress the matcher and server",
        execute = function ()
            for i = 0, 63 do
                os.execute "if exist bin\\x64\\Debug\\secure_client.exe ( start /B bin\\x64\\Debug\\secure_client ) else ( echo could not find bin\\x64\\Debug\\secure_client.exe )"
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
            "docker/yojimbo",
            "valgrind/yojimbo",
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
