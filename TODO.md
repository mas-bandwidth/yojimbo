# TODO

Work out how to install mbedtls on windows.

Once this is working, windows platform should be working again.

OK. Got cmake building with x64. Headers working fine, linking OK, but CMake by default is going to the wrong c standard libraries. Annoying!

How to make CMake generate a solution with the correct debug and release runtime lib?

The trick is to add this to the CMakeLists.txt

	if(MSVC)
	    set(CMAKE_C_FLAGS_DEBUG_INIT "/D_DEBUG /MTd /Zi /Ob0 /Od /RTC1")
	    set(CMAKE_C_FLAGS_MINSIZEREL_INIT     "/MT /O1 /Ob1 /D NDEBUG")
	    set(CMAKE_C_FLAGS_RELEASE_INIT        "/MT /O2 /Ob2 /D NDEBUG")
	    set(CMAKE_C_FLAGS_RELWITHDEBINFO_INIT "/MT /Zi /O2 /Ob1 /D NDEBUG")
	endif()

