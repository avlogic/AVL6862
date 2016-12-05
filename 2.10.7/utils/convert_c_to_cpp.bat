
echo on
rem convert file extension from .c to .cpp
rem for /R ..\ %%f in (*.c) do ren %%f %%~nf.cpp
for /r "../" %%x in (*.c) do ren "%%x" *.cpp
