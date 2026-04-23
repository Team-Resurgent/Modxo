# block_device

**WARNING**: Any filesystem format on the target drive will be destroyed! 
You can format the card to replace the filesystem, 
but you will lose any preexisting data.

This example illustrates the direct use of the block device API. The API implements the *Media Access Interface* described in [FatFs - Generic FAT Filesystem Module](http://elm-chan.org/fsw/ff/00index_e.html) (also see *Required Functions* in [FatFs Module Application Note](http://elm-chan.org/fsw/ff/doc/appnote.html)).
The declarations are in `src/ff15/source/diskio.h`.

If you don't require a filesystem on an SD card,
or if you want to use a different filesystem,
you can operate at the block device level.
At the block device interface, the SD card appears to contain a long sequence of
numbered blocks of 512 bytes each. I.e., the smallest addressable unit is a block of 512 bytes.
The address of a block is its "logical block address" (LBA).
Blocks can be addressed by their LBA and read and written individually or as sequences with a starting address and length.

This example can use SPI or SDIO attached SD cards.
