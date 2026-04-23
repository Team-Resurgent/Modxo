# stdio_buffering

This example illustrates the speedup that can be provided by taking advantage of the 
C library's Standard Input/Output (stdio) buffering.

Typical output:
```
Hello, world!
Time without buffering: 7840 ms
Time with buffering: 3207 ms
Goodbye, world!
```
showing a speedup of greater than 2X.

If you have a record-oriented application, 
and the records are multiples of 512 bytes in size,
you might not see a significant speedup.
However, if, for example, you are writing text files with 
no fixed record length, the speedup can be great.
