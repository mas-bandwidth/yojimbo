
solution "reliable"
    kind "ConsoleApp"
    language "C++"
    platforms { "x64" }
    configurations { "Debug", "Release" }
    if not os.is "windows" then
        includedirs { ".", "/usr/local/include" }       -- for clang scan-build only. for some reason it needs this to work =p
        targetdir "bin/"  
    end
    rtti "Off"
    flags { "ExtraWarnings", "StaticRuntime", "FloatFast", "EnableSSE2" }
    configuration "Debug"
        symbols "On"
        links { debug_libs }
    configuration "Release"
        symbols "Off"
        optimize "Speed"
        defines { "NDEBUG" }
        links { release_libs }
        
project "test"
    files { "test.c", "reliable.c" }

project "soak"
    files { "soak.c", "reliable.c" }

project "fuzz"
    files { "fuzz.c", "reliable.c" }

if os.is "windows" then

    -- Windows

    newaction
    {
        trigger     = "solution",
        description = "Create and open the reliable.io solution",
        execute = function ()
            os.execute "premake5 vs2015"
            os.execute "start reliable.sln"
        end
    }

    -- todo: create shortcuts here too for windows for consistency

else

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
        trigger     = "fuzz",
        description = "Build and run fuzz test",
        execute = function ()
            os.execute "test ! -e Makefile && premake5 gmake"
            if os.execute "make -j32 fuzz" == 0 then
                os.execute "./bin/fuzz"
            end
        end
    }

    newaction
    {
        trigger     = "cppcheck",
        description = "Run cppcheck over the source code",
        execute = function ()
            os.execute "cppcheck reliable.c"
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
        trigger     = "docker",
        description = "Build and run reliable.io tests inside docker",
        execute = function ()
            os.execute "rm -rf docker/reliable.io && mkdir -p docker/reliable.io && cp *.h docker/reliable.io && cp *.c docker/reliable.io && cp premake5.lua docker/reliable.io && cd docker && docker build -t \"networkprotocol:reliable.io-server\" . && rm -rf reliable.io && docker run -ti -p 40000:40000/udp networkprotocol:reliable.io-server"
        end
    }

    newaction
    {
        trigger     = "valgrind",
        description = "Run valgrind over tests inside docker",
        execute = function ()
            os.execute "rm -rf valgrind/reliable.io && mkdir -p valgrind/reliable.io && cp *.h valgrind/reliable.io && cp *.c valgrind/reliable.io && cp premake5.lua valgrind/reliable.io && cd valgrind && docker build -t \"networkprotocol:reliable.io-valgrind\" . && rm -rf reliable.io && docker run -ti networkprotocol:reliable.io-valgrind"
        end
    }

    newaction
    {
        trigger     = "loc",
        description = "Count lines of code",
        execute = function ()
            os.execute "wc -l *.h *.c"
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
            "docs",
            "xml",
            "docker/reliable.io",
            "valgrind/reliable.io"
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
