#ifndef SPITRANSPORT_HPP
#define SPITRANSPORT_HPP

#include "ICommunication.hpp"
#include <cstdint>

namespace CTLIB {

class SPITransport : public ICommunication {
public:
    struct PinConfig {
        void *sckPort;  uint16_t sckPin;
        void *sdaPort;  uint16_t sdaPin;
        void *csPort;   uint16_t csPin;
        void *dcPort;   uint16_t dcPin;
        void *blPort;   uint16_t blPin;
    };

    struct SPIConfig {
        void *instance;
    };

    SPITransport(const PinConfig &pins, const SPIConfig &spi);
    ~SPITransport() override;

    void init() override;
    void writeCommand(uint8_t cmd) override;
    void writeData8(uint8_t data) override;
    void writeData16(uint16_t data) override;
    void writeBulk(uint16_t *buffer, uint16_t size) override;
    void csLow() override;
    void csHigh() override;
    void dcCommand() override;
    void dcData() override;
    void backlightOn() override;
    void backlightOff() override;
    void delayMs(uint32_t ms) override;

private:
    PinConfig  m_pins;
    SPIConfig  m_spi;
    void       spiMspInit();
    void       spiPeriphInit();
};

} // namespace CTLIB

#endif // SPITRANSPORT_HPP
