#include "LCD_ST7789.hpp"
#include <cstring>
#include <cmath>

namespace CTLIB {

// ============================================================
// ILCD shared default implementations
// ============================================================

ILCD::ILCD(ICommunication &comm, uint16_t width, uint16_t height)
    : m_comm(comm)
    , m_width(width)
    , m_height(height)
    , m_color(Colors::WHITE)
    , m_backColor(Colors::BLACK)
    , m_direction(Direction::Vertical)
    , m_numFillMode(NumFillMode::FillZero)
    , m_xOffset(0)
    , m_yOffset(0)
    , m_asciiFont(nullptr)
    , m_chFont(nullptr)
{
}

uint16_t ILCD::rgb888to565(uint32_t c) const
{
    uint8_t r = (c >> 16) & 0xFF;
    uint8_t g = (c >> 8)  & 0xFF;
    uint8_t b =  c        & 0xFF;
    return static_cast<uint16_t>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

void ILCD::clear()
{
    setAddress(0, 0, m_width - 1, m_height - 1);
    uint16_t c565 = rgb888to565(m_backColor);
    uint16_t total = m_width * m_height;

    for (uint16_t i = 0; i < total; i++) {
        m_buffer[i % BUFF_SIZE] = c565;
        if ((i + 1) % BUFF_SIZE == 0 || i == total - 1) {
            uint16_t chunk = (i % BUFF_SIZE == BUFF_SIZE - 1) ? BUFF_SIZE : (i % BUFF_SIZE + 1);
            m_comm.writeBulk(m_buffer, chunk);
        }
    }
}

void ILCD::clearRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (x + w > m_width || y + h > m_height) return;
    setAddress(x, y, x + w - 1, y + h - 1);
    uint16_t c565 = rgb888to565(m_backColor);
    uint16_t total = w * h;
    for (uint16_t i = 0; i < total; i++) {
        m_buffer[i % BUFF_SIZE] = c565;
        if ((i + 1) % BUFF_SIZE == 0 || i == total - 1) {
            uint16_t chunk = (i % BUFF_SIZE == BUFF_SIZE - 1) ? BUFF_SIZE : (i % BUFF_SIZE + 1);
            m_comm.writeBulk(m_buffer, chunk);
        }
    }
}

void ILCD::setColor(uint32_t color888)
{
    m_color = color888;
}

void ILCD::setBackColor(uint32_t color888)
{
    m_backColor = color888;
}

void ILCD::setDirection(Direction dir)
{
    m_direction = dir;
    m_comm.writeCommand(0x36);
    switch (dir) {
        case Direction::Horizontal:      m_comm.writeData8(0x60); m_xOffset = 0; m_yOffset = 0; break;
        case Direction::HorizontalFlip:  m_comm.writeData8(0xA0); m_xOffset = 0; m_yOffset = 0; break;
        case Direction::Vertical:        m_comm.writeData8(0x00); m_xOffset = 0; m_yOffset = 0; break;
        case Direction::VerticalFlip:    m_comm.writeData8(0xC0); m_xOffset = 0; m_yOffset = 0; break;
    }
}

void ILCD::setNumFillMode(NumFillMode mode)
{
    m_numFillMode = mode;
}

void ILCD::setAsciiFont(const Font *font)
{
    m_asciiFont = font;
}

void ILCD::setTextFont(const Font *chFont)
{
    m_chFont = chFont;
}

// --- internal helpers ---

void ILCD::setAddress(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    uint16_t xs = x1 + m_xOffset;
    uint16_t ys = y1 + m_yOffset;
    uint16_t xe = x2 + m_xOffset;
    uint16_t ye = y2 + m_yOffset;

    m_comm.writeCommand(0x2A);
    m_comm.writeData8(static_cast<uint8_t>(xs >> 8));
    m_comm.writeData8(static_cast<uint8_t>(xs));
    m_comm.writeData8(static_cast<uint8_t>(xe >> 8));
    m_comm.writeData8(static_cast<uint8_t>(xe));

    m_comm.writeCommand(0x2B);
    m_comm.writeData8(static_cast<uint8_t>(ys >> 8));
    m_comm.writeData8(static_cast<uint8_t>(ys));
    m_comm.writeData8(static_cast<uint8_t>(ye >> 8));
    m_comm.writeData8(static_cast<uint8_t>(ye));

    m_comm.writeCommand(0x2C);
}

void ILCD::writeBuffer(uint16_t *data, uint16_t size)
{
    m_comm.writeBulk(data, size);
}

// --- ASCII text ---

void ILCD::drawChar(uint16_t x, uint16_t y, uint8_t c)
{
    if (!m_asciiFont) return;

    uint16_t charWidth  = m_asciiFont->width;
    uint16_t charHeight = m_asciiFont->height;
    uint16_t charSize   = m_asciiFont->sizes;

    if (c < ' ' || c > '~') return;
    uint16_t index = (c - ' ') * charSize;

    uint16_t buffCount = 0;
    uint16_t buffHeight = (sizeof(m_buffer) / 2) / charWidth;
    if (buffHeight == 0) buffHeight = 1;

    uint16_t yAddr = y;

    for (uint16_t row = 0; row < charHeight; row++) {
        for (uint16_t col = 0; col < charWidth; col++) {
            uint16_t byteIdx = index + row * ((charWidth + 7) / 8) + (col / 8);
            uint8_t  bitPos  = 7 - (col % 8);
            if (m_asciiFont->table[byteIdx] & (1u << bitPos)) {
                m_buffer[buffCount] = rgb888to565(m_color);
            } else {
                m_buffer[buffCount] = rgb888to565(m_backColor);
            }
            buffCount++;
        }
        if (buffCount >= buffHeight * charWidth) {
            setAddress(x, yAddr, x + charWidth - 1, yAddr + buffHeight - 1);
            writeBuffer(m_buffer, charWidth * buffHeight);
            yAddr += buffHeight;
            buffCount = 0;
        }
    }
    if (buffCount > 0) {
        setAddress(x, yAddr, x + charWidth - 1, y + charHeight - 1);
        writeBuffer(m_buffer, buffCount);
    }
}

void ILCD::drawString(uint16_t x, uint16_t y, const char *str)
{
    if (!str || !m_asciiFont) return;
    uint16_t curX = x;
    while (*str) {
        if (*str == '\n') {
            curX = x;
            y += m_asciiFont->height;
        } else {
            drawChar(curX, y, static_cast<uint8_t>(*str));
            curX += m_asciiFont->width;
        }
        str++;
    }
}

// --- Chinese text ---

void ILCD::drawChinese(uint16_t x, uint16_t y, const char *text)
{
    if (!text || !m_chFont) return;

    uint16_t cw = m_chFont->width;
    uint16_t ch = m_chFont->height;

    // GB2312: two bytes per Chinese char
    uint8_t hi = static_cast<uint8_t>(*text);
    uint8_t lo = static_cast<uint8_t>(*(text + 1));

    // search font table
    uint16_t tableRows = m_chFont->tableRows;
    uint16_t sizes     = m_chFont->sizes;

    for (uint16_t i = 0; i < tableRows; i++) {
        uint16_t base = i * (sizes + 2);
        if (m_chFont->table[base + sizes]     == hi &&
            m_chFont->table[base + sizes + 1] == lo) {
            // found — draw
            uint16_t buffCount = 0;
            uint16_t yAddr = y;
            uint16_t buffHeight = (sizeof(m_buffer) / 2) / cw;
            if (buffHeight == 0) buffHeight = 1;

            for (uint16_t row = 0; row < ch; row++) {
                for (uint16_t col = 0; col < cw; col++) {
                    uint16_t byteIdx = base + row * ((cw + 7) / 8) + (col / 8);
                    uint8_t  bitPos  = 7 - (col % 8);
                    if (m_chFont->table[byteIdx] & (1u << bitPos)) {
                        m_buffer[buffCount] = rgb888to565(m_color);
                    } else {
                        m_buffer[buffCount] = rgb888to565(m_backColor);
                    }
                    buffCount++;
                }
                if (buffCount >= buffHeight * cw) {
                    setAddress(x, yAddr, x + cw - 1, yAddr + buffHeight - 1);
                    writeBuffer(m_buffer, cw * buffHeight);
                    yAddr += buffHeight;
                    buffCount = 0;
                }
            }
            if (buffCount > 0) {
                setAddress(x, yAddr, x + cw - 1, y + ch - 1);
                writeBuffer(m_buffer, buffCount);
            }
            return;
        }
    }
}

void ILCD::drawText(uint16_t x, uint16_t y, const char *text)
{
    if (!text || !m_chFont) return;
    uint16_t curX = x;
    while (*text) {
        uint8_t c = static_cast<uint8_t>(*text);
        if (c >= 0xA1 && m_chFont) {
            drawChinese(curX, y, text);
            text += 2;
            curX += m_chFont->width;
        } else if (c >= ' ' && c <= '~' && m_asciiFont) {
            drawChar(curX, y, c);
            text++;
            curX += m_asciiFont->width;
        } else {
            text++;
        }
    }
}

// --- numbers ---

void ILCD::drawNumber(uint16_t x, uint16_t y, int32_t num, uint8_t len)
{
    if (!m_asciiFont) return;
    char buf[16];
    std::memset(buf, 0, sizeof(buf));

    if (m_numFillMode == NumFillMode::FillZero) {
        // right-aligned, zero-padded
        bool neg = (num < 0);
        uint32_t absVal = neg ? static_cast<uint32_t>(-num) : static_cast<uint32_t>(num);
        int idx = len - 1;
        do {
            buf[idx--] = '0' + (absVal % 10);
            absVal /= 10;
        } while (idx >= 0 && absVal > 0);
        while (idx >= 0) buf[idx--] = '0';
        if (neg && len > 0) buf[0] = '-';
        drawString(x, y, buf);
    } else {
        // left-aligned, space-filled
        int n = snprintf(buf, sizeof(buf), "%*d", len, static_cast<int>(num));
        if (n < 0) return;
        drawString(x, y, buf);
    }
}

void ILCD::drawDecimal(uint16_t x, uint16_t y, double num, uint8_t len, uint8_t decs)
{
    if (!m_asciiFont) return;
    char buf[32];
    std::memset(buf, 0, sizeof(buf));
    int n = snprintf(buf, sizeof(buf), "%*.*f", len, decs, num);
    if (n < 0) return;
    drawString(x, y, buf);
}

// --- 2D drawing ---

void ILCD::drawLineH(uint16_t x, uint16_t y, uint16_t w)
{
    if (x + w > m_width) w = m_width - x;
    setAddress(x, y, x + w - 1, y);
    uint16_t c565 = rgb888to565(m_color);
    for (uint16_t i = 0; i < w; i++) {
        m_buffer[i % BUFF_SIZE] = c565;
        if ((i + 1) % BUFF_SIZE == 0 || i == w - 1) {
            uint16_t chunk = (i % BUFF_SIZE == BUFF_SIZE - 1) ? BUFF_SIZE : (i % BUFF_SIZE + 1);
            writeBuffer(m_buffer, chunk);
        }
    }
}

void ILCD::drawLineV(uint16_t x, uint16_t y, uint16_t h)
{
    if (y + h > m_height) h = m_height - y;
    setAddress(x, y, x, y + h - 1);
    uint16_t c565 = rgb888to565(m_color);
    for (uint16_t i = 0; i < h; i++) {
        m_buffer[i % BUFF_SIZE] = c565;
        if ((i + 1) % BUFF_SIZE == 0 || i == h - 1) {
            uint16_t chunk = (i % BUFF_SIZE == BUFF_SIZE - 1) ? BUFF_SIZE : (i % BUFF_SIZE + 1);
            writeBuffer(m_buffer, chunk);
        }
    }
}

void ILCD::drawLine(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    int dx  = static_cast<int>(x2) - static_cast<int>(x1);
    int dy  = static_cast<int>(y2) - static_cast<int>(y1);
    int adx = dx > 0 ? dx : -dx;
    int ady = dy > 0 ? dy : -dy;

    if (adx > ady) {
        if (x1 > x2) { std::swap(x1, x2); std::swap(y1, y2); }
        int err = adx / 2;
        int yi = (y2 > y1) ? 1 : -1;
        uint16_t y = y1;
        for (uint16_t x = x1; x <= x2; x++) {
            drawPoint(x, y, m_color);
            err -= ady;
            if (err < 0) { y += yi; err += adx; }
        }
    } else {
        if (y1 > y2) { std::swap(x1, x2); std::swap(y1, y2); }
        int err = ady / 2;
        int xi = (x2 > x1) ? 1 : -1;
        uint16_t x = x1;
        for (uint16_t y = y1; y <= y2; y++) {
            drawPoint(x, y, m_color);
            err -= adx;
            if (err < 0) { x += xi; err += ady; }
        }
    }
}

void ILCD::drawRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    drawLineH(x, y, w);
    drawLineH(x, y + h - 1, w);
    drawLineV(x, y, h);
    drawLineV(x + w - 1, y, h);
}

void ILCD::drawCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int curX = 0;
    int curY = static_cast<int>(r);
    int D    = 3 - 2 * static_cast<int>(r);

    while (curY >= curX) {
        drawPoint(x + curX, y + curY, m_color);
        drawPoint(x - curX, y + curY, m_color);
        drawPoint(x + curX, y - curY, m_color);
        drawPoint(x - curX, y - curY, m_color);
        drawPoint(x + curY, y + curX, m_color);
        drawPoint(x - curY, y + curX, m_color);
        drawPoint(x + curY, y - curX, m_color);
        drawPoint(x - curY, y - curX, m_color);

        if (D < 0) {
            D += (curX << 2) + 6;
        } else {
            D += ((curX - curY) << 2) + 10;
            curY--;
        }
        curX++;
    }
}

void ILCD::drawEllipse(int x, int y, int r1, int r2)
{
    int curX = 0;
    int curY = r2;
    int r1sq = r1 * r1;
    int r2sq = r2 * r2;
    int d = r2sq + r1sq / 4 - r1sq * r2;

    while (r2sq * curX <= r1sq * curY) {
        drawPoint(x + curX, y + curY, m_color);
        drawPoint(x - curX, y + curY, m_color);
        drawPoint(x + curX, y - curY, m_color);
        drawPoint(x - curX, y - curY, m_color);
        if (d < 0) {
            d += r2sq * (2 * curX + 3);
        } else {
            d += r2sq * (2 * curX + 3) + r1sq * (-2 * curY + 2);
            curY--;
        }
        curX++;
    }

    d = static_cast<int>(r2sq * (curX + 0.5) * (curX + 0.5) + r1sq * (curY - 1) * (curY - 1) - r1sq * r2sq);
    while (curY >= 0) {
        drawPoint(x + curX, y + curY, m_color);
        drawPoint(x - curX, y + curY, m_color);
        drawPoint(x + curX, y - curY, m_color);
        drawPoint(x - curX, y - curY, m_color);
        if (d > 0) {
            d += r1sq * (-2 * curY + 3);
        } else {
            d += r2sq * (2 * curX + 2) + r1sq * (-2 * curY + 3);
            curX++;
        }
        curY--;
    }
}

// --- filled shapes ---

void ILCD::fillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    if (x + w > m_width  || y + h > m_height) return;
    setAddress(x, y, x + w - 1, y + h - 1);
    uint16_t c565 = rgb888to565(m_color);
    uint16_t total = w * h;
    for (uint16_t i = 0; i < total; i++) {
        m_buffer[i % BUFF_SIZE] = c565;
        if ((i + 1) % BUFF_SIZE == 0 || i == total - 1) {
            uint16_t chunk = (i % BUFF_SIZE == BUFF_SIZE - 1) ? BUFF_SIZE : (i % BUFF_SIZE + 1);
            writeBuffer(m_buffer, chunk);
        }
    }
}

void ILCD::fillCircle(uint16_t x, uint16_t y, uint16_t r)
{
    int curX = 0;
    int curY = static_cast<int>(r);
    int D    = 3 - 2 * static_cast<int>(r);

    while (curY >= curX) {
        drawLineH(x - curY, y - curX, 2 * curY);
        drawLineH(x - curY, y + curX, 2 * curY);
        drawLineH(x - curX, y - curY, 2 * curX);
        drawLineH(x - curX, y + curY, 2 * curX);

        if (D < 0) {
            D += (curX << 2) + 6;
        } else {
            D += ((curX - curY) << 2) + 10;
            curY--;
        }
        curX++;
    }
}

// --- image ---

void ILCD::drawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *pImage)
{
    if (!pImage) return;

    uint16_t buffHeight = (sizeof(m_buffer) / 2) / w;
    if (buffHeight == 0) buffHeight = 1;

    uint16_t xAddr = x;
    uint16_t yAddr = y;
    uint16_t buffCount = 0;

    for (uint16_t row = 0; row < h; row++) {
        for (uint16_t col = 0; col < (w + 7) / 8; col++) {
            uint8_t byte = pImage[row * ((w + 7) / 8) + col];
            for (uint8_t bit = 0; bit < 8; bit++) {
                if (col * 8 + bit >= w) break;
                if (byte & (1u << bit)) {
                    m_buffer[buffCount] = rgb888to565(m_color);
                } else {
                    m_buffer[buffCount] = rgb888to565(m_backColor);
                }
                buffCount++;
            }
        }

        if (buffCount >= buffHeight * w) {
            setAddress(x, yAddr, x + w - 1, yAddr + buffHeight - 1);
            writeBuffer(m_buffer, w * buffHeight);
            yAddr += buffHeight;
            buffCount = 0;
        }

        if ((row + 1) == h && buffCount > 0) {
            setAddress(x, yAddr, x + w - 1, row + y);
            writeBuffer(m_buffer, buffCount);
        }
    }
}

// ============================================================
// LCD_ST7789 specific
// ============================================================

LCD_ST7789::LCD_ST7789(ICommunication &comm, uint16_t width, uint16_t height)
    : ILCD(comm, width, height)
{
}

void LCD_ST7789::writeRegisterSequence()
{
    m_comm.writeCommand(0x36);
    m_comm.writeData8(0x00);

    m_comm.writeCommand(0x3A);
    m_comm.writeData8(0x05);

    // VDV/VRH power control
    m_comm.writeCommand(0xB2);
    m_comm.writeData8(0x0C);
    m_comm.writeData8(0x0C);
    m_comm.writeData8(0x00);
    m_comm.writeData8(0x33);
    m_comm.writeData8(0x33);

    m_comm.writeCommand(0xB7);
    m_comm.writeData8(0x35);

    m_comm.writeCommand(0xBB);
    m_comm.writeData8(0x19);

    m_comm.writeCommand(0xC0);
    m_comm.writeData8(0x2C);

    m_comm.writeCommand(0xC2);
    m_comm.writeData8(0x01);

    m_comm.writeCommand(0xC3);
    m_comm.writeData8(0x12);

    m_comm.writeCommand(0xC4);
    m_comm.writeData8(0x20);

    m_comm.writeCommand(0xC6);
    m_comm.writeData8(0x0F);

    m_comm.writeCommand(0xD0);
    m_comm.writeData8(0xA4);
    m_comm.writeData8(0xA1);

    // Gamma+ correction
    m_comm.writeCommand(0xE0);
    m_comm.writeData8(0xD0);
    m_comm.writeData8(0x04);
    m_comm.writeData8(0x0D);
    m_comm.writeData8(0x11);
    m_comm.writeData8(0x13);
    m_comm.writeData8(0x2B);
    m_comm.writeData8(0x3F);
    m_comm.writeData8(0x54);
    m_comm.writeData8(0x4C);
    m_comm.writeData8(0x18);
    m_comm.writeData8(0x0D);
    m_comm.writeData8(0x0B);
    m_comm.writeData8(0x1F);
    m_comm.writeData8(0x23);

    // Gamma- correction
    m_comm.writeCommand(0xE1);
    m_comm.writeData8(0xD0);
    m_comm.writeData8(0x04);
    m_comm.writeData8(0x0C);
    m_comm.writeData8(0x11);
    m_comm.writeData8(0x13);
    m_comm.writeData8(0x2C);
    m_comm.writeData8(0x3F);
    m_comm.writeData8(0x44);
    m_comm.writeData8(0x51);
    m_comm.writeData8(0x2F);
    m_comm.writeData8(0x1F);
    m_comm.writeData8(0x1F);
    m_comm.writeData8(0x20);
    m_comm.writeData8(0x23);

    // Inversion on
    m_comm.writeCommand(0x21);

    // Sleep out
    m_comm.writeCommand(0x11);
    m_comm.delayMs(120);

    // Display on
    m_comm.writeCommand(0x29);
}

void LCD_ST7789::init()
{
    m_comm.init();
    m_comm.delayMs(10);

    m_comm.csLow();
    writeRegisterSequence();
    m_comm.csHigh();

    setDirection(Direction::Vertical);
    setBackColor(Colors::BLACK);
    setColor(Colors::WHITE);
    clear();

    m_comm.backlightOn();
}

void LCD_ST7789::drawPoint(uint16_t x, uint16_t y, uint32_t color888)
{
    if (x >= m_width || y >= m_height) return;
    uint16_t c565 = rgb888to565(color888);
    setAddress(x, y, x, y);
    m_comm.writeData16(c565);
}

} // namespace CTLIB
