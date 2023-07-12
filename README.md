# File-System-OS-
This is my implementation of a file system on blocks. Done on linux environment using C.
Author: Manav Lal

If you want to add your own tests in the file test.c or in another file you need to recreate the main.c file
which runs them. There is a simple shell script which generates this file for you. Execute make-tests.sh
and redirect the output into main.c. Then compile and run main.c.
gcc -Wall main.c CuTest.c test.c fileSystem.c device.c -o test
./test

The display program (in display.c) can be compiled and run (read the comments).
gcc -Wall display.c device.c -o display
./display

The display should work even after closing, you should be able to run a test, close check display, run another test (given you do not format the storage) and still have the previous run up.

For simplcity: 
- Volume name maximum size - 63 single byte characters
- File or directory name maximum size - 7 single byte characters
- File or directory size - limited by available blocks
- File pathname size - limited by available blocks
- Number of blocks on the device - between 8 and 1024
- There is no deletion involved
- The write occurs from the last written point

