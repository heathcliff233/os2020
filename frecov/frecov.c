#include <stdint.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>
#include <unistd.h>
#include <sys/wait.h>
#include "frecov.h"

off_t img_size;
void* img_start;
uintptr_t root_entry;
fat_header_t* fat_header;
int bmpcnt;
bmp_image_t bmp[1100];

int main(int argc, char *argv[]) {
    int fd = open(argv[1], O_RDONLY);
    assert(fd != -1);
    struct stat file_stat;
    stat(argv[1], &file_stat);
    img_size = file_stat.st_size;
    img_start = mmap(NULL, file_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    assert(img_start != MAP_FAILED);
    close(fd);
    bmpcnt = 0;

    fat_info();
    scan_bmp();

    return 0;
}

void fat_info() {
    fat_header = (fat_header_t*)img_start;
    root_entry = (uintptr_t)img_start + (fat_header->BPB_ResvdSecCnt + fat_header->BPB_NumFATs * fat_header->BPB_FATSz32) * fat_header->BPB_BytsPerSec;
}

void scan_bmp() {
    dir_entry_t* cur = (dir_entry_t*)root_entry;
    char cmd[150];
    while((uintptr_t)cur < (uintptr_t)(img_start) + img_size &&bmpcnt < 1010) {
      if(cur->extname[0] == 'B' && cur->extname[1] == 'M' && cur->extname[2] == 'P' && (uint8_t)cur->filename[0] != 0xe5) {
        //对比目录项扩展名发现bmp图片
        bmpcnt ++;
        bmp[bmpcnt].id = bmpcnt;
        bmp[bmpcnt].start_clst = ((cur->start_hi) << 16) + cur->start_lo;
        bmp[bmpcnt].size = cur->filesz;
        strncpy(bmp[bmpcnt].full_name, cur->filename, 15);
        /*
        if(cur->ntres == 0x0) {
          int ret = get_long_name(cur, &bmp[bmpcnt]);
          if(ret == 0) {bmpcnt --;}
        }
        */
        snprintf(cmd, 150, "sha1sum %s", bmp[bmpcnt].full_name);
        system(cmd);

      }
      cur = (dir_entry_t*)(((uintptr_t)cur) + sizeof(dir_entry_t));
    }
}

