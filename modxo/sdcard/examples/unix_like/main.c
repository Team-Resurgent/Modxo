/* main.c
Copyright 2021 Carl John Kugler III

Licensed under the Apache License, Version 2.0 (the License); you may not use
this file except in compliance with the License. You may obtain a copy of the
License at

   http://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software distributed
under the License is distributed on an AS IS BASIS, WITHOUT WARRANTIES OR
CONDITIONS OF ANY KIND, either express or implied. See the License for the
specific language governing permissions and limitations under the License.
*/

/* This example illustrates use of Unix style drive prefix.
See http://elm-chan.org/fsw/ff/doc/filename.html. */

/* For this example, we 
  #define FF_STR_VOLUME_ID	2
in include/ffconf.h, add
  const char *VolumeStr[] = {"sd0", "sd1"};	
in hw_config.c, and add include to the target_include_directories
in CMakeLists.txt.
*/

/* Expected output:
Hello, world!
Writing to /sd0/file0.txt
Writing to /sd1/file1.txt
Goodbye, world!
*/

/* NOTE: for this to work, src\ff15\source\ffconf.h must be removed or renamed. */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//
#include "pico/stdlib.h"
//
#include "f_util.h"
#include "hw_config.h"
#include "sd_card.h"

static bool write_file(const char *pathname) {
    printf("Writing to %s\n", pathname);
    FIL fil = {};
    FRESULT fr = f_open(&fil, pathname, FA_OPEN_APPEND | FA_WRITE);
    if (FR_OK != fr && FR_EXIST != fr) {
        printf("f_open(%s) error: %s (%d)\n", pathname, FRESULT_str(fr), fr);
        return false;
    }
    if (f_printf(&fil, "Hello, world!\n") < 0) {
        printf("f_printf failed\n");
        return false;
    }
    fr = f_close(&fil);
    if (FR_OK != fr) {
        printf("f_close error: %s (%d)\n", FRESULT_str(fr), fr);
        return false;
    }
    return true;
}

int main() {
    stdio_init_all();

    // This must be called before sd_get_drive_prefix:
    sd_init_driver(); 

    puts("Hello, world!");

    // See FatFs - Generic FAT Filesystem Module, "Application Interface",
    // http://elm-chan.org/fsw/ff/00index_e.html

    for (size_t i = 0; i < sd_get_num(); ++i) {
        sd_card_t *sd_card_p = sd_get_by_num(i);
        assert(sd_card_p);
        char const *drive_prefix = sd_get_drive_prefix(sd_card_p);

        FRESULT fr = f_mount(&sd_card_p->state.fatfs, drive_prefix, 1);
        if (FR_OK != fr) {
            printf("f_mount error: %s (%d)\n", FRESULT_str(fr), fr);
            exit(1);
        }
        char buf[128];
        snprintf(buf, sizeof buf, "%s/file%d.txt", drive_prefix, i);
        if (!write_file(buf))
            exit(2);

        // ls(drive_prefix);

        f_unmount(drive_prefix);
    }

    puts("Goodbye, world!");
    for (;;)
        ;
}
