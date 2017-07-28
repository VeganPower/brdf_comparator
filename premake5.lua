lib_dir = "c:/dev/"
workspace_dir = "f:/workspace/"

workspace "BdrfComparator"
   configurations { "Debug", "Release" }


project "BdrfComparator"
   architecture "x64"

   kind "ConsoleApp"
   language "C++"

   --includedirs { lib_dir .. "glm-0.9.8.4" }
   --includedirs { lib_dir .. "glfw-3.2.bin.WIN64/include" }
   --includedirs { "./src/GL" }
   includedirs { workspace_dir .. 'bx/include/compat/msvc' }
   includedirs { workspace_dir .. 'bx/include' }
   includedirs { workspace_dir .. 'bimg/include' }
   includedirs { workspace_dir .. 'bgfx/include' }
   includedirs { workspace_dir .. 'bgfx/3rdparty' }
   includedirs { workspace_dir .. 'bgfx/examples/common' }

   --libdirs { lib_dir .. "glfw-3.2.bin.WIN64/lib-vc2015" }
   libdirs { workspace_dir .. 'bgfx/3rdparty/lib/win64_vs2017' }
   libdirs { workspace_dir .. 'bgfx/.build/win64_vs2017/bin' }
   --links { "glfw3", "opengl32" }

   defines {
      '__STDC_LIMIT_MACROS',
      '__STDC_FORMAT_MACROS',
      '__STDC_CONSTANT_MACROS',
      'NDEBUG',
      'WIN32',
      '_WIN32',
      '_HAS_EXCEPTIONS=0',
      '_HAS_ITERATOR_DEBUGGING=0',
      '_SCL_SECURE=0',
      '_SECURE_SCL=0',
      '_SCL_SECURE_NO_WARNINGS',
      '_CRT_SECURE_NO_WARNINGS',
      '_CRT_SECURE_NO_DEPRECATE',
      '_WIN64',
   }

   vectorextensions "AVX"
   exceptionhandling "Off"
   editandcontinue  "Off"

   flags { "NoEditAndContinue", "NoIncrementalLink", "NoManifest", "NoMinimalRebuild", "C++14" }
   flags { "StaticRuntime" }
   rtti "Off"


   files { "src/**.cpp", "src/**.hpp" } -- C++ style
   files { "src/**.c", "src/**.h" } -- C style


   filter "configurations:Debug"
      optimize "Debug"
      flags { "Symbols" }
      links { "bxDebug" }
      links { "bgfxDebug" }
      links { "bimgDebug" }
      links { "bimg_decodeDebug" }
      links { "example-commonDebug" }

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "Speed"
      flags{ "LinkTimeOptimization" }

      links { "bxRelease" }
      links { "bgfxRelease" }
      links { "bimgRelease" }
      links { "bimg_decodeRelease" }
      links { "example-commonRelease" }
