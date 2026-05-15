#ifndef LCD_ST7789_HPP
#define LCD_ST7789_HPP

#include "ILCD.hpp"

namespace CTLIB {

class LCD_ST7789 : public ILCD {
public:
    LCD_ST7789(ICommunication &comm, uint16_t width = 240, uint16_t height = 240);
    ~LCD_ST7789() override = default;

    void init() override;
    void drawPoint(uint16_t x, uint16_t y, uint32_t color888) override;

private:
    void writeRegisterSequence();
};

} // namespace CTLIB

#endif // LCD_ST7789_HPP
