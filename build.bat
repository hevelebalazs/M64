@echo off

call vcvarsall.lnk x64

cd bin
del "./*" /s /q > nul

cl /Od /Zi ../code/m64_windows.cpp user32.lib Gdi32.lib