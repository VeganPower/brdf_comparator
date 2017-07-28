@echo off
rem F:\workspace\bgfx\.build\win64_vs2017\bin\shadercRelease -f <filename> -i <include_dir> -o <output_file>
set SHC="F:\workspace\bgfx\.build\win64_vs2017\bin\shadercRelease.exe"

rem %SHC%
%SHC% -f shaders\fs_cubes.hlsl -i "F:\workspace\bgfx\src" -p ps_4_0 --platform windows --type fragment -o shaders\dx11\fs_cubes.bin
%SHC% -f shaders\vs_cubes.hlsl -i "F:\workspace\bgfx\src" -p vs_4_0 --platform windows --type vertex -o shaders\dx11\vs_cubes.bin


set SHC=