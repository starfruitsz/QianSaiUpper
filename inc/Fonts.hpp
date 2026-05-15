#ifndef FONTS_HPP
#define FONTS_HPP

#include <cstdint>

namespace CTLIB {

struct Font {
    const uint8_t *table;
    uint16_t       width;
    uint16_t       height;
    uint16_t       sizes;
    uint16_t       tableRows;
};

} // namespace CTLIB

#endif // FONTS_HPP
