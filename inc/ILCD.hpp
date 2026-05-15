#ifndef ILCD_HPP
#define ILCD_HPP

#include "ICommunication.hpp"
#include "Fonts.hpp"
#include <cstdint>

namespace CTLIB {

// Display direction
enum class Direction : uint8_t {
    Horizontal       = 0,
    HorizontalFlip   = 1,
    Vertical         = 2,
    VerticalFlip     = 3
};

// Number display fill mode
enum class NumFillMode : uint8_t {
    FillZero  = 0,
    FillSpace = 1
};

// 24-bit RGB color helpers
constexpr uint32_t RGB888(uint8_t r, uint8_t g, uint8_t b) {
    return (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
}

// Common colors (RGB888 -> converted to RGB565 internally)
namespace Colors {
    constexpr uint32_t WHITE       = 0xFFFFFF;
    constexpr uint32_t BLACK       = 0x000000;
    constexpr uint32_t BLUE        = 0x0000FF;
    constexpr uint32_t GREEN       = 0x00FF00;
    constexpr uint32_t RED         = 0xFF0000;
    constexpr uint32_t CYAN        = 0x00FFFF;
    constexpr uint32_t MAGENTA     = 0xFF00FF;
    constexpr uint32_t YELLOW      = 0xFFFF00;
    constexpr uint32_t GREY        = 0x2C2C2C;
    constexpr uint32_t LIGHT_BLUE  = 0x8080FF;
    constexpr uint32_t LIGHT_GREEN = 0x80FF80;
    constexpr uint32_t LIGHT_RED   = 0xFF8080;
    constexpr uint32_t LIGHT_CYAN  = 0x80FFFF;
    constexpr uint32_t LIGHT_MAGENTA = 0xFF80FF;
    constexpr uint32_t LIGHT_YELLOW = 0xFFFF80;
    constexpr uint32_t LIGHT_GREY  = 0xA3A3A3;
    constexpr uint32_t DARK_BLUE   = 0x000080;
    constexpr uint32_t DARK_GREEN  = 0x008000;
    constexpr uint32_t DARK_RED    = 0x800000;
    constexpr uint32_t DARK_CYAN   = 0x008080;
    constexpr uint32_t DARK_MAGENTA = 0x800080;
    constexpr uint32_t DARK_YELLOW = 0x808000;
    constexpr uint32_t DARK_GREY   = 0x404040;
}

class ILCD {
public:
    ILCD(ICommunication &comm, uint16_t width, uint16_t height);
    virtual ~ILCD() = default;

    // --- init/clear ---
    virtual void init() = 0;
    void clear();
    void clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

    // --- properties ---
    void setColor(uint32_t color888);
    void setBackColor(uint32_t color888);
    void setDirection(Direction dir);
    void setNumFillMode(NumFillMode mode);

    uint32_t color()     const { return m_color; }
    uint32_t backColor() const { return m_backColor; }
    uint16_t width()     const { return m_width; }
    uint16_t height()    const { return m_height; }

    // --- ASCII text ---
    void setAsciiFont(const Font *font);
    void drawChar(uint16_t x, uint16_t y, uint8_t c);
    void drawString(uint16_t x, uint16_t y, const char *str);

    // --- Chinese + mixed text ---
    void setTextFont(const Font *chFont);
    void drawChinese(uint16_t x, uint16_t y, const char *text);
    void drawText(uint16_t x, uint16_t y, const char *text);

    // --- numbers ---
    void drawNumber(uint16_t x, uint16_t y, int32_t num, uint8_t len);
    void drawDecimal(uint16_t x, uint16_t y, double num, uint8_t len, uint8_t decs);

    // --- 2D primitives ---
    virtual void drawPoint(uint16_t x, uint16_t y, uint32_t color888) = 0;
    void drawLineH(uint16_t x, uint16_t y, uint16_t w);
    void drawLineV(uint16_t x, uint16_t y, uint16_t h);
    void drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void drawCircle(uint16_t x, uint16_t y, uint16_t r);
    void drawEllipse(int x, int y, int r1, int r2);

    // --- filled ---
    void fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
    void fillCircle(uint16_t x, uint16_t y, uint16_t r);

    // --- image ---
    void drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *pImage);

protected:
    ICommunication &m_comm;
    uint16_t m_width;
    uint16_t m_height;
    uint32_t m_color;
    uint32_t m_backColor;
    Direction m_direction;
    NumFillMode m_numFillMode;
    uint8_t m_xOffset;
    uint8_t m_yOffset;

    const Font *m_asciiFont;
    const Font *m_chFont;

    static constexpr uint16_t BUFF_SIZE = 1024;
    uint16_t m_buffer[BUFF_SIZE];

    // helpers called by subclasses
    void setAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);
    void writeBuffer(uint16_t *data, uint16_t size);
    uint16_t rgb888to565(uint32_t color888) const;
};

} // namespace CTLIB

#endif // ILCD_HPP
