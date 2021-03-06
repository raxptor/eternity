solution "Tools"

    configurations {"Release", "Debug"}

    flags { "Symbols" }

    location "build"
    targetdir "build"

    defines {"_CRT_SECURE_NO_WARNINGS"}
    defines("BUILDER_DEFAULT_RUNTIME=x64")
    defines("LIVEUPDATE_ENABLE")
    defines("PUTKI_ENABLE_LOG")

    if os.get() == "windows" then
        flags {"StaticRuntime"}
    end

    configuration {"linux", "gmake"}
        buildoptions {"-fPIC"}
        buildoptions ("-std=c++11")

    configuration "Debug"
        defines {"DEBUG"}
    configuration "Release"
        flags {"Optimize"}

    configuration {}

    ------------------------------------
    -- Putki must always come first   --
    ------------------------------------

    dofile "ext/putki/libs.lua"

    project "eternity-putki-lib"
        language "C++"
        targetname "eternity-putki-lib"
        kind "StaticLib"

        -- putki last here        
        putki_use_builder_lib()
        putki_typedefs_builder("src/types", true)

    project "eternity-databuilder"

        kind "ConsoleApp"
        language "C++"
        targetname "eternity-databuilder"

        files { "src/putki/builder-main.cpp" }
        files { "src/builder/**.cpp" }
        links { "eternity-putki-lib" }
        includedirs { "src" }

        putki_use_builder_lib()

        putki_typedefs_builder("src/types", false)

    project "eternity-datatool"

        kind "ConsoleApp"
        language "C++"
        targetname "eternity-datatool"

        files { "src/putki/tool-main.cpp" }

        putki_use_builder_lib()
        links { "eternity-putki-lib" }

        includedirs { "src" }

    project "eternity-data-dll"

        kind "SharedLib"
        language "C++"
        targetname "eternity-data-dll"

        files { "src/putki/dll-main.cpp" }
        files { "src/builder/**.*" }
		files { "src/fontanell/**.*" }
        links { "eternity-putki-lib"}
        includedirs { "src" }

        putki_typedefs_builder("src/types", false)
        putki_use_builder_lib()
