@echo off
set opts=-nologo -Zi -W4 -std:c++23preview -permissive- -EHsc
set incl=-I../raylib/raylib-6.0_win64_msvc16/include/
set link=-libpath:../raylib/raylib-6.0_win64_msvc16/lib/ -NODEFAULTLIB:MSVCRT -NODEFAULTLIB:LIBCMT
mkdir build
pushd build
@echo on
cl -Fe:game_of_life.exe ../main.cpp gdi32.lib msvcrt.lib raylib.lib winmm.lib user32.lib shell32.lib raylib.lib %incl% %opts% -link %link%
@echo off
popd