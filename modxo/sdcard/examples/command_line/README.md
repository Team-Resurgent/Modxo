# command_line example
This is a comprehensive example demonstrating many capabilities of the no-OS-FatFS-SD-SDIO-SPI-RPi-Pico library.
It also can be used to launch some test programs.

It provides a command line interface; something like busybox or DOS.

## Operation
* Connect a terminal. [PuTTY](https://www.putty.org/) or `tio` work OK. For example:
  * `tio -m ODELBS /dev/ttyACM0`
* Press Enter to start the CLI. You should see a prompt like:
```
    > 
```    
* The `help` command describes the available commands:
```    
> help
setrtc <DD> <MM> <YY> <hh> <mm> <ss>:
 Set Real Time Clock
 Parameters: new date (DD MM YY) new time in 24-hour format (hh mm ss)
        e.g.:setrtc 16 3 21 0 4 0

date:
 Print current date and time

format [<drive#:>]:
 Creates an FAT/exFAT volume on the logical drive.
        e.g.: format 0:

mount [<drive#:>]:
 Register the work area of the volume
        e.g.: mount 0:

unmount <drive#:>:
 Unregister the work area of the volume

chdrive <drive#:>:
 Changes the current directory of the logical drive.
 <path> Specifies the directory to be set as current directory.
        e.g.: chdrive 1:

info [<drive#:>]:
 Print information about an SD card

cd <path>:
 Changes the current directory of the logical drive.
 <path> Specifies the directory to be set as current directory.
        e.g.: cd /dir1

mkdir <path>:
 Make a new directory.
 <path> Specifies the name of the directory to be created.
        e.g.: mkdir /dir1

rm [options] <pathname>:
 Removes (deletes) a file or directory
 <pathname> Specifies the path to the file or directory to be removed
 Options:
 -d Remove an empty directory
 -r Recursively remove a directory and its contents

cp <source file> <dest file>:
 Copies <source file> to <dest file>

mv <source file> <dest file>:
 Moves (renames) <source file> to <dest file>

pwd:
 Print Working Directory

ls [pathname]:
 List directory

cat <filename>:
 Type file contents

simple:
 Run simple FS tests

lliot <physical drive#>:
 !DESTRUCTIVE! Low Level I/O Driver Test
The SD card will need to be reformatted after this test.
        e.g.: lliot 1

bench <drive#:>:
 A simple binary write/read benchmark

big_file_test <pathname> <size in MiB> <seed>:
 Writes random data to file <pathname>.
 Specify <size in MiB> in units of mebibytes (2^20, or 1024*1024 bytes)
        e.g.: big_file_test 0:/bf 1 1
        or: big_file_test 1:big3G-3 3072 3

bft: Alias for big_file_test

cdef:
 Create Disk and Example Files
 Expects card to be already formatted and mounted

swcwdt:
 Stdio With CWD Test
Expects card to be already formatted and mounted.
Note: run cdef first!

loop_swcwdt:
 Run Create Disk and Example Files and Stdio With CWD Test in a loop.
Expects card to be already formatted and mounted.
Note: Hit Enter key to quit.

start_logger:
 Start Data Log Demo

stop_logger:
 Stop Data Log Demo

mem-stats:
 Print memory statistics

help:
 Shows this command help.

```
