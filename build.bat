@echo off
gcc -DWINDOWS -c -g src/*.c -Iinclude && ar rcs libpicobt.a *.o && echo Success!
del *.o
