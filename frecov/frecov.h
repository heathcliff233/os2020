#include <stdint.h>
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