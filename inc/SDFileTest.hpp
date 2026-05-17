#ifndef SDFILETEST_HPP
#define SDFILETEST_HPP

#include <cstring>
#include <cstdio>
#include "fatfs.h"
#include "UartTransport.hpp"

namespace CTLIB
{

/* ============================================================ */
/* SDFileTest - Init SD + create User folder + write test.txt    */
/* Checks for existing FS first; only formats blank/unformatted  */
/* ============================================================ */

inline bool SDFormatAndTest(UartTransport &uart)
{
    FRESULT res;
    FATFS fs;
    FIL fil;
    UINT bw;

    uart.Print("\r\n=== SD Init & Test Start ===\r\n");

    /* 0. Probe: try mounting to see if FS exists */
    uart.Print("[0/3] Probing SD card...\r\n");
    res = f_mount(&fs, SDPath, 1);
    if (res == FR_NO_FILESYSTEM) {
        /* No valid FS 闂?format */
        uart.Print("  No filesystem, formatting...\r\n");
        BYTE work[512];
        res = f_mkfs(SDPath, FM_FAT32, 0, work, sizeof(work));
        if (res != FR_OK) {
            uart.Print("  FAIL: f_mkfs returned %d\r\n", (int)res);
            return false;
        }
        uart.Print("  Format OK\r\n");
        /* Re-mount after format */
        res = f_mount(&fs, SDPath, 1);
    }
    if (res != FR_OK) {
        uart.Print("  FAIL: mount returned %d\r\n", (int)res);
        return false;
    }
    {
        DWORD nclst;
        FATFS *pfs;
        if (f_getfree(SDPath, &nclst, &pfs) == FR_OK) {
            DWORD tot = (pfs->n_fatent - 2) * pfs->csize;
            uart.Print("  Mounted: %lu MB free / %lu MB total\r\n",
                       (nclst * pfs->csize) / 2048,
                       (tot * 512uLL) / 1048576uLL);
        }
    }

    /* 1. Create User folder */
    uart.Print("[1/3] Creating /User folder...\r\n");
    char userPath[16];
    snprintf(userPath, sizeof(userPath), "%sUser", SDPath);
    res = f_mkdir(userPath);
    if (res != FR_OK && res != FR_EXIST) {
        uart.Print("  FAIL: f_mkdir returned %d\r\n", (int)res);
        return false;
    }
    uart.Print(res == FR_EXIST ? "  Already exists\r\n" : "  OK\r\n");

    /* 2. Write test.txt */
    uart.Print("[2/3] Writing /User/test.txt...\r\n");
    const char *content =
        "=== Test Document ===\r\n"
        "Created by STM32F407 + FATFS\r\n"
        "\r\n"
        "System Info:\r\n"
        "  MCU:    STM32F407ZGT6 @ 168MHz\r\n"
        "  LCD:    ST7789 240x240\r\n"
        "  FS:     FATFS R0.12c\r\n"
        "\r\n"
        "Test Data:\r\n"
        "  PI = 3.1415926535\r\n"
        "  E  = 2.7182818284\r\n"
        "\r\n"
        "=== End of Document ===\r\n";

    char testPath[32];
    snprintf(testPath, sizeof(testPath), "%sUser/test.txt", SDPath);
    res = f_open(&fil, testPath, FA_CREATE_ALWAYS | FA_WRITE);
    if (res != FR_OK) {
        uart.Print("  FAIL: f_open returned %d\r\n", (int)res);
        return false;
    }
    f_write(&fil, content, strlen(content), &bw);
    f_close(&fil);
    uart.Print("  OK, %u bytes written\r\n", bw);

    /* 3. Check font files */
    uart.Print("[3/3] Checking font files...\r\n");
    FILINFO fno;
    char fontPath1[64];
    snprintf(fontPath1, sizeof(fontPath1), "%sUTF-8/srcdata/GB2312_H2424.FON", SDPath);
    res = f_stat(fontPath1, &fno);
    if (res == FR_OK) {
        uart.Print("  GB2312_H2424.FON: %lu bytes\r\n", fno.fsize);
    } else {
      uart.Print("  GB2312_H2424.FON: NOT FOUND\r\n");
    }
    if (res == FR_OK) {
    } else {
    }

    f_mount(NULL, SDPath, 1);
    uart.Print("=== Done ===\r\n");
    return true;
}

}  /* namespace CTLIB */

#endif
