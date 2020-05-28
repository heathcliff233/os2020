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
//#include "frecov.h"

typedef struct fat32_boot_sector{
    uint8_t BS_jmpBoot[3];
    uint8_t BS_OEMName[8];
    uint16_t BPB_BytsPerSec;
    uint8_t BPB_SecPerClus;
    uint16_t BPB_ResvdSecCnt;
    uint8_t BPB_NumFATs;
    uint16_t BPB_RootEntCnt;
    uint16_t BPB_TotSec16;
    uint8_t BPB_Media;
    uint16_t BPB_FATSz16;
    uint16_t BPB_SecPerTrk;
    uint16_t BPB_NumHeads;
    uint32_t BPB_HiddSec;
    uint32_t BPB_TotSec32;
    uint32_t BPB_FATSz32;
    uint16_t BPB_ExtFlags;
    uint16_t BPB_FSVer;
    uint32_t BPB_RootClus;
    uint16_t BPB_FSInfo;
    uint16_t BPB_BkBootSec;
    uint8_t BPB_Reserved[12];
    uint8_t BS_DrvNum;
    uint8_t BS_Reserved1;
    uint8_t BS_BootSig;
    uint32_t BS_VolID;
    uint8_t BS_VolLab[11];
    uint8_t BS_FilSysType[8];
} __attribute__((packed)) fat_header_t;


typedef struct dir_entry {
  char filename[8]; //文件名
  char extname[3]; //扩展名
  uint8_t attr; //文件类型
  uint8_t ntres; //系统保留
  uint8_t create_time_ms; //创建时间的10ms位
  uint16_t create_time; //创建时间
  uint16_t create_date; 
  uint16_t last_acc_date;
  uint16_t start_hi; //起始簇号的高16位
  uint16_t last_change_time;
  uint16_t last_change_date;
  uint16_t start_lo; //起始簇号的低16位
  uint32_t filesz; //文件长度
}__attribute__((packed)) dir_entry_t;

typedef struct BMP_IMAGE {
  int id;
  uint32_t start_clst;
  uint32_t size;
  uint32_t color;
  char short_name[15];
  char full_name[128];
}__attribute__((packed)) bmp_image_t;

void fat_info();
void scan_bmp();
void print_sha();

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

