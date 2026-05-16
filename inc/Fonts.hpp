#ifndef FONTS_HPP
#define FONTS_HPP

#include <cstdint>
#include <utility>

namespace CTLIB
{

/* ============================================================ */
/* 字体结构体 */
/* 与原 lcd_fonts.h 的 pFONT 完全兼容 */
/* ============================================================ */

struct Font
{
    const uint8_t *table;  /* 字模数组首地址 */
    uint16_t       width;  /* 单个字符的字模宽度（像素） */
    uint16_t       height;  /* 单个字符的字模高度（像素） */
    uint16_t       sizes;  /* 单个字符的字模字节数 */
    uint16_t       tableRows;  /* 中文字库二维数组行数（ASCII 字体为 0） */
};

/* ============================================================ */
/* 编译期字体表（C++17 constexpr） */
/* ============================================================ */

template <size_t N>
struct FontTable
{
    static constexpr size_t count = N;
    using Entry = std::pair<const uint8_t*, const char*>;
    Entry entries[N];
};

/* ============================================================ */
/* 从编译期数组创建 Font 的便捷工厂函数 */
/* ============================================================ */

template <size_t W, size_t H, size_t BytesPerChar>
constexpr Font makeFont(const uint8_t (&data)[], uint16_t tableRows = 0)
{
    return Font{data, W, H, BytesPerChar, tableRows};
}

}  /* namespace CTLIB */

#endif
