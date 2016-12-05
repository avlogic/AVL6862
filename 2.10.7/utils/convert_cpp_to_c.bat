
echo on
rem convert file extension from .cpp to .c
rem for /R ..\ %%f in (*.cpp) do ren "%%f" "%%~nf.c"
for /r "../" %%x in (*.cpp) do ren "%%x" *.c
