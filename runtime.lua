solution "Eternity"

	location "build"
	targetdir "build"

	platforms { "x64" }

	configurations {"Debug", "Release", "Ship"}
	
	configuration {"Debug or Release"}
		defines("LIVEUPDATE_ENABLE")
		defines("PUTKI_ENABLE_LOG")		
		
	configuration {"Ship"}
		defines("SHIP")
		defines("PUTKI_NO_RT_PATH_PREFIX")
		
	configuration {"Release or Ship"}
		flags {"Optimize"}
	
	configuration {}
	
	flags { "Symbols" }
	libdirs {"/usr/lib"}
	defines {"_CRT_SECURE_NO_WARNINGS"}

	dofile("ext/putki/runtime.lua")
	dofile("ext/korv/libs.lua")

project "eternity"
        kind "WindowedApp"
        language "C++"
        targetname "eternity"

        putki_use_runtime_lib()
        
        putki_typedefs_runtime("src/types", true)

        files { "src/eternity/**.cpp" }
        files { "src/eternity/**.h" }

        excludes { "src/builder/**.*" }
        excludes { "src/putki/**.*" }

        includedirs { "src" }

    configuration {"windows"}
            excludes {"src/**_osx*"}
    
    configuration {"macosx"}
            excludes {"src/**_win32*"}
            files {"src/**.mm"}
            links {"AppKit.framework", "QuartzCore.framework", "OpenGL.framework"}
