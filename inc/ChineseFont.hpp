#ifndef CHINESEFONT_HPP
#define CHINESEFONT_HPP

#include "Fonts.hpp"
#include "fatfs.h"
#include <cstdio>
#include <cstdint>
#include <functional>

namespace CTLIB
{

struct ChineseFont24
{
    static constexpr uint16_t kWidth         = 24;
    static constexpr uint16_t kHeight        = 24;
    static constexpr uint16_t kBytesPerGlyph = 72;

    static constexpr Font descriptor()
    {
        return Font{nullptr, kWidth, kHeight, kBytesPerGlyph, 2};
    }
};

class FatfsReadChinese
{
public:
    FatfsReadChinese() : mFontOpen(false) {}
    ~FatfsReadChinese() { Close(); }

    bool Init()
    {
        if (mFontOpen) return true;
        FRESULT res;
        res = f_mount(&SDFatFS, SDPath, 1);
        if (res != FR_OK) return false;
        char fontPath[64];
        snprintf(fontPath, sizeof(fontPath), "%sUTF-8/srcdata/GB2312_H2424.FON", SDPath);
        res = f_open(&mFontFile, fontPath, FA_READ);
        if (res != FR_OK) { f_mount(NULL, SDPath, 1); return false; }
        mFontOpen = true;
        return true;
    }

    void Close()
    {
        if (mFontOpen) { f_close(&mFontFile); mFontOpen = false; }
        f_mount(NULL, SDPath, 1);
    }

    void operator()(uint32_t unicode, uint8_t *buf)
    {
        if (!mFontOpen || !buf) return;
        uint16_t gbOffset = unicodeToGbOffset(unicode);
        if (gbOffset == 0xFFFFu) return;
        UINT br;
        uint32_t offset = static_cast<uint32_t>(gbOffset) * ChineseFont24::kBytesPerGlyph;
        f_lseek(&mFontFile, offset);
        f_read(&mFontFile, buf, ChineseFont24::kBytesPerGlyph, &br);
    }

private:
    FIL  mFontFile;
    bool mFontOpen;

    uint16_t unicodeToGbOffset(uint32_t unicode)
    {
        if (unicode > 0xFFFFu) return 0xFFFFu;

        /* ff_convert: dir=0 = Unicode(UTF-16) -> OEM(GBK) */
        WCHAR gbk = ff_convert(static_cast<WCHAR>(unicode), 0);
        if (gbk == 0) return 0xFFFFu;

        uint8_t hi = static_cast<uint8_t>(gbk >> 8);
        uint8_t lo = static_cast<uint8_t>(gbk & 0xFF);

        /* GB2312 zone: hi in [0xA1, 0xF7], lo in [0xA1, 0xFE] */
        if (hi < 0xA1u || hi > 0xF7u) return 0xFFFFu;
        if (lo < 0xA1u || lo > 0xFEu) return 0xFFFFu;

        uint16_t area = static_cast<uint16_t>(hi) - 0xA0u;
        uint16_t pos  = static_cast<uint16_t>(lo) - 0xA0u;
        if (area < 1u || area > 87u || pos < 1u || pos > 94u) return 0xFFFFu;

        return (area - 1u) * 94u + (pos - 1u);
    }
};

}  /* namespace CTLIB */

#endif