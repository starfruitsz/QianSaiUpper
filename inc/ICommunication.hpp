#ifndef ICOMMUNICATION_HPP
#define ICOMMUNICATION_HPP

#include <cstdint>

namespace CTLIB
{

// ============================================================
// 通信传输层抽象接口（CRTP 模板基类）
// 使用编译期静态分发，零虚函数开销
// 子类 Impl 需实现以下接口：
//   implInit, implWriteCommand, implWriteData8/16, implWriteBulk,
//   implCsLow/High, implDcCommand/Data, implBacklightOn/Off, implDelayMs
// ============================================================

template <typename Impl>
class ICommunication
{
public:
    // 初始化通信外设和引脚
    inline void init()
    {
        impl().implInit();
    }

    // 写入 LCD 命令字节
    inline void writeCommand(uint8_t c)
    {
        impl().implWriteCommand(c);
    }

    // 写入 8 位数据
    inline void writeData8(uint8_t d)
    {
        impl().implWriteData8(d);
    }

    // 写入 16 位数据（RGB565 像素）
    inline void writeData16(uint16_t d)
    {
        impl().implWriteData16(d);
    }

    // 批量写入像素数据到显存
    inline void writeBulk(uint16_t *b, uint16_t s)
    {
        impl().implWriteBulk(b, s);
    }

    // 片选控制
    inline void csLow()
    {
        impl().implCsLow();
    }
    inline void csHigh()
    {
        impl().implCsHigh();
    }

    // 数据/命令选择控制
    inline void dcCommand()
    {
        impl().implDcCommand();
    }
    inline void dcData()
    {
        impl().implDcData();
    }

    // 背光控制
    inline void backlightOn()
    {
        impl().implBacklightOn();
    }
    inline void backlightOff()
    {
        impl().implBacklightOff();
    }

    // 毫秒级延时
    inline void delayMs(uint32_t ms)
    {
        impl().implDelayMs(ms);
    }

protected:
    ICommunication() = default;
    ~ICommunication() = default;

private:
    // CRTP 静态向下转型
    Impl &impl()
    {
        return static_cast<Impl&>(*this);
    }
    const Impl &impl() const
    {
        return static_cast<const Impl&>(*this);
    }
};

} // namespace CTLIB

#endif