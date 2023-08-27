# 6510
6510 written in standard C, it's the core project of a future project (Commodore 64 emulator).


## Goals:

- **Future project**; The main goal of this project is to develop a 6510 processor for a future project.

- **User-friendly**; Compare the values and help people debug their own emulator. (Passes all tests in the test_file directory)

- **Portable**; The emulator has been tested on MacOS and Linux (Fedora 38) and runs without any issues.


## Output:

How the desired output should look like:

`Tests used: Klaus Dormman 6502, AllSuiteA and Timing test`


```
 ________    ________    ______       ________
|\   ____\  |\   ____\  /___   \     |\   __  \
\ \  \      \ \ \ ___/  \___ \  \    \ \  \|\  \
 \ \  \_____ \ \ \_____     \ \  \    \ \  \|\  \
  \ \    _  \ \ \_______\    \ \  \    \ \  \|\  \
   \ \  \|\  \ \/______\ \    \ \  \    \ \  \|\  \
    \ \_______\ ________\ \  __\_\  \___ \ \_______\
     \|_______| \_\________\ \_\________\ \|_______|

** file loaded: test_files/AllSuiteA.bin **
✓ - test passed!

** file loaded: test_files/6502_decimal_test.bin **
✓ - test passed!

** file loaded: test_files/6502_interrupt_test.bin **
✓ - test passed!

** file loaded: test_files/6502_functional_test.bin **
✓ - test passed!

** file loaded: test_files/timingtest-1.bin **
✓ - test passed!

Program executed in 0 seconds
```


## Debugging:

For debugging please comment out the `cpu_debug()` function in order to output the values and mnemonics executed during testing.

- Illegal/Undocumented opcodes haven't been tested, but the legal/documented ones are mostly accurate.

- TomHarte tests haven't been used during testing.


## Resources:

In order to access the resources used, please refer to the resources.txt file and the comments inside the source code.
