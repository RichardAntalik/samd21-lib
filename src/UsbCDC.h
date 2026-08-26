#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>
#include "libs/cmsis/Core/Include/cmsis_gcc.h"
#include "libs/cmsis/samd21a/include/samd21e18a.h"

#ifndef GCLK_CLKCTRL
#define GCLK_CLKCTRL  CLKCTRL
#endif
#ifndef GCLK_STATUS
#define GCLK_STATUS   STATUS
#endif

extern "C" { void NVIC_EnableIRQ(IRQn_Type irq); }

// Standard USB requests
static constexpr uint8_t GET_STATUS = 0;
static constexpr uint8_t CLEAR_FEATURE = 1;
static constexpr uint8_t SET_FEATURE = 3;
static constexpr uint8_t SET_ADDRESS_REQ = 5;
static constexpr uint8_t GET_DESCRIPTOR = 6;
static constexpr uint8_t SET_DESCRIPTOR = 7;
static constexpr uint8_t GET_CONFIGURATION = 8;
static constexpr uint8_t SET_CONFIGURATION = 9;
static constexpr uint8_t GET_INTERFACE = 10;
static constexpr uint8_t SET_INTERFACE = 11;

// Endpoint direction/type
static constexpr uint8_t USB_ENDPOINT_OUT(uint8_t addr) { return addr | 0x00; }
static constexpr uint8_t USB_ENDPOINT_IN(uint8_t addr) { return addr | 0x80; }
static constexpr uint8_t EPX_SIZE = 64;

static constexpr uint8_t USB_ENDPOINT_TYPE_CONTROL = 0x00;
static constexpr uint8_t USB_ENDPOINT_TYPE_BULK = 0x02;
static constexpr uint8_t USB_ENDPOINT_TYPE_INTERRUPT = 0x03;

static constexpr uint8_t REQUEST_HOSTTODEVICE = 0x00;
static constexpr uint8_t REQUEST_DEVICETOHOST = 0x80;
static constexpr uint8_t REQUEST_STANDARD = 0x00;
static constexpr uint8_t REQUEST_CLASS = 0x20;
static constexpr uint8_t REQUEST_TYPE = 0x60;
static constexpr uint8_t REQUEST_DEVICE = 0x00;
static constexpr uint8_t REQUEST_INTERFACE = 0x01;
static constexpr uint8_t REQUEST_ENDPOINT = 0x02;
static constexpr uint8_t REQUEST_RECIPIENT = 0x1F;

static constexpr uint8_t USB_DEVICE_DESCRIPTOR_TYPE = 1;
static constexpr uint8_t USB_CONFIGURATION_DESCRIPTOR_TYPE = 2;
static constexpr uint8_t USB_STRING_DESCRIPTOR_TYPE = 3;

// CDC requests
static constexpr uint8_t CDC_SET_LINE_CODING = 0x20;
static constexpr uint8_t CDC_GET_LINE_CODING = 0x21;
static constexpr uint8_t CDC_SET_CONTROL_LINE_STATE = 0x22;
static constexpr uint8_t CDC_SEND_BREAK = 0x23;

// CDC constants
static constexpr uint16_t CDC_V1_10 = 0x0110;
static constexpr uint8_t CDC_COMM_CLASS = 0x02;
static constexpr uint8_t CDC_ACM_SUBCLASS = 0x02;
static constexpr uint8_t CDC_DATA_CLASS = 0x0A;
static constexpr uint8_t CDC_CS_INTERFACE = 0x24;

// Bootloader magic
static constexpr uint32_t BOOTLOADER_MAGIC_VALUE = 0xf01669ef;
static inline volatile uint32_t* bootloader_magic_ptr() {
    return reinterpret_cast<volatile uint32_t*>(HMCRAMC0_ADDR + HMCRAMC0_SIZE - 4);
}

// USB VID/PID (match existing VCP values)
static constexpr uint16_t USB_VID = 0x0666;
static constexpr uint16_t USB_PID = 0x0666;
static constexpr uint16_t USB_VERSION = 0x0100;

static constexpr uint8_t IMANUFACTURER = 1;
static constexpr uint8_t IPRODUCT = 2;
static constexpr uint8_t ISERIAL = 3;

// Endpoint assignment (matches existing VCP)
static constexpr uint8_t CDC_ENDPOINT_ACM = 1;
static constexpr uint8_t CDC_ENDPOINT_OUT = 2;
static constexpr uint8_t CDC_ENDPOINT_IN = 3;
static constexpr uint8_t CDC_ACM_INTERFACE = 0;
static constexpr uint8_t CDC_DATA_INTERFACE = 1;

struct __attribute__((packed)) USBSetup {
    uint8_t bmRequestType;
    uint8_t bRequest;
    uint16_t wValue;
    uint16_t wIndex;
    uint16_t wLength;
};

// Descriptor types from USBCore.h
struct __attribute__((packed)) DeviceDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint16_t usbVersion;
    uint8_t deviceClass;
    uint8_t deviceSubClass;
    uint8_t deviceProtocol;
    uint8_t packetSize0;
    uint16_t idVendor;
    uint16_t idProduct;
    uint16_t deviceVersion;
    uint8_t iManufacturer;
    uint8_t iProduct;
    uint8_t iSerialNumber;
    uint8_t bNumConfigurations;
};

struct __attribute__((packed)) ConfigDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint16_t clen;
    uint8_t numInterfaces;
    uint8_t config;
    uint8_t iconfig;
    uint8_t attributes;
    uint8_t maxPower;
};

struct __attribute__((packed)) InterfaceDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint8_t number;
    uint8_t alternate;
    uint8_t numEndpoints;
    uint8_t interfaceClass;
    uint8_t interfaceSubClass;
    uint8_t protocol;
    uint8_t iInterface;
};

struct __attribute__((packed)) EndpointDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint8_t addr;
    uint8_t attr;
    uint16_t packetSize;
    uint8_t interval;
};

struct __attribute__((packed)) IADDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint8_t firstInterface;
    uint8_t interfaceCount;
    uint8_t functionClass;
    uint8_t funtionSubClass;
    uint8_t functionProtocol;
    uint8_t iInterface;
};

struct __attribute__((packed)) CDCCSDescriptor {
    uint8_t len;
    uint8_t dtype;
    uint8_t subtype;
    uint8_t d0;
    uint8_t d1;
};

struct __attribute__((packed)) CDCCSDescriptor4 {
    uint8_t len;
    uint8_t dtype;
    uint8_t subtype;
    uint8_t d0;
};

// Descriptor macros
#define D_DEVICE(_class,_sub,_proto,_psz,_vid,_pid,_ver,_im,_ip,_is,_cfg) \
    { 18, 1, 0x200, _class,_sub,_proto,_psz,_vid,_pid,_ver,_im,_ip,_is,_cfg }

#define D_CONFIG(_total,_ifaces) \
    { 9, 2, _total,_ifaces, 1, 0, static_cast<uint8_t>(0xC0 | 0x20), static_cast<uint8_t>((_total > 255 ? 255 : (500)/2)) }

#define D_INTERFACE(_n,_eps,_class,_sub,_proto) \
    { 9, 4, _n, 0, _eps, _class,_sub, _proto, 0 }

#define D_ENDPOINT(_addr,_attr,_psz,_intv) \
    { 7, 5, _addr,_attr,_psz, _intv }

#define D_IAD(_first,_count,_class,_sub,_proto) \
    { 8, 11, _first, _count, _class, _sub, _proto, 0 }

#define D_CDCCS(_subtype,_d0,_d1) { 5, CDC_CS_INTERFACE, _subtype, _d0, _d1 }
#define D_CDCCS4(_subtype,_d0)   { 4, CDC_CS_INTERFACE, _subtype, _d0 }

// Device descriptors - two variants for composite detection
static const DeviceDescriptor DEVICE_DESC_COMPOSITE = 
    D_DEVICE(0xEF, 0x02, 0x01, EPX_SIZE, USB_VID, USB_PID, USB_VERSION, IMANUFACTURER, IPRODUCT, ISERIAL, 1);

static const DeviceDescriptor DEVICE_DESC_STANDARD = 
    D_DEVICE(0x00, 0x00, 0x00, EPX_SIZE, USB_VID, USB_PID, USB_VERSION, IMANUFACTURER, IPRODUCT, ISERIAL, 1);

static const uint16_t STRING_LANG[2] = { ((3<<8) | (2+2)), 0x0409 };
static constexpr const char* STR_MANUFACTURER = "ISS";
static constexpr const char* STR_PRODUCT = "Random little device";

struct __attribute__((packed)) LineInfo {
    uint32_t dwDTERate;
    uint8_t bCharFormat;
    uint8_t bParityType;
    uint8_t bDataBits;
};

// CDC descriptor structure (matches Arduino's CDCDescriptor)
struct CDCDescriptor {
    IADDescriptor iad;
    InterfaceDescriptor cif;
    CDCCSDescriptor header;
    CDCCSDescriptor4 controlManagement;
    CDCCSDescriptor functionalDescriptor;
    CDCCSDescriptor callManagement;
    EndpointDescriptor cifin;
    InterfaceDescriptor dif;
    EndpointDescriptor in_ep;
    EndpointDescriptor out_ep;
};

// Ring buffer for RX (extracted from Arduino's RingBuffer, no dependencies)
class RingBuffer {
public:
    static constexpr size_t SIZE = 256;
    
    RingBuffer() : _head(0), _tail(0) {}
    
    size_t availableForStore() const {
        return (SIZE - 1) - available();
    }
    
    size_t available() const {
        return (_head - _tail + SIZE) % SIZE;
    }
    
    void store_char(uint8_t c) {
        size_t next = (_head + 1) % SIZE;
        if (next != _tail) {
            _buffer[_head] = c;
            _head = next;
        }
    }
    
    uint8_t read_char() {
        uint8_t c = _buffer[_tail];
        _tail = (_tail + 1) % SIZE;
        return c;
    }
    
    int peek() const {
        if (_head == _tail) return -1;
        return _buffer[_tail];
    }

private:
    volatile uint8_t _buffer[SIZE];
    volatile size_t _head;
    volatile size_t _tail;
};

// USB hardware wrapper (extracted from SAMD21_USBDevice.h, no Arduino deps)
class USBDevice_SAMD21 {
public:
    USBDevice_SAMD21() : usb(USB->DEVICE), EP() {}

    inline void reset();
    inline void calibrate();
    
    inline void enable()  { usb.CTRLA.bit.ENABLE = 1; }
    inline void disable() { usb.CTRLA.bit.ENABLE = 0; }
    inline void setUSBDeviceMode() { usb.CTRLA.bit.MODE = USB_CTRLA_MODE_DEVICE_Val; }
    inline void runInStandby() { usb.CTRLA.bit.RUNSTDBY = 1; }
    inline void setDataSensitiveQoS() { usb.QOSCTRL.bit.DQOS = 2; }
    inline void setConfigSensitiveQoS() { usb.QOSCTRL.bit.CQOS = 2; }
    inline void setFullSpeed() { usb.CTRLB.bit.SPDCONF = USB_DEVICE_CTRLB_SPDCONF_FS_Val; }
    inline void attach() { usb.CTRLB.bit.DETACH = 0; }

    inline bool isEndOfResetInterrupt() { return usb.INTFLAG.bit.EORST; }
    inline void ackEndOfResetInterrupt() { usb.INTFLAG.reg = USB_DEVICE_INTFLAG_EORST; }
    inline void enableEndOfResetInterrupt() { usb.INTENSET.bit.EORST = 1; }

    inline bool isStartOfFrameInterrupt() { return usb.INTFLAG.bit.SOF; }
    inline void ackStartOfFrameInterrupt() { usb.INTFLAG.reg = USB_DEVICE_INTFLAG_SOF; }
    inline void enableStartOfFrameInterrupt() { usb.INTENSET.bit.SOF = 1; }

    inline void setAddress(uint32_t addr) {
        usb.DADD.bit.DADD = addr;
        usb.DADD.bit.ADDEN = 1;
    }

    // Endpoint config
    inline void epBank0SetType(uint8_t ep, uint8_t type) { usb.DeviceEndpoint[ep].EPCFG.bit.EPTYPE0 = type; }
    inline void epBank1SetType(uint8_t ep, uint8_t type) { usb.DeviceEndpoint[ep].EPCFG.bit.EPTYPE1 = type; }

    // Endpoint interrupts
    inline bool epHasPendingInterrupts(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.reg != 0; }
    inline bool epBank0IsSetupReceived(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.bit.RXSTP; }
    inline bool epBank1IsTransferComplete(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.bit.TRCPT1; }
    inline bool epBank0IsTransferComplete(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.bit.TRCPT0; }

    inline void epAckPendingInterrupts(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTFLAG.reg = 0x7F; }
    inline void epBank0AckSetupReceived(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_RXSTP; }
    inline void epBank1AckTransferComplete(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT(2); }
    inline void epBank0AckTransferComplete(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTFLAG.reg = USB_DEVICE_EPINTFLAG_TRCPT(1); }

    inline void epBank0EnableSetupReceived(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTENSET.bit.RXSTP = 1; }
    inline void epBank1EnableTransferComplete(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTENSET.bit.TRCPT1 = 1; }
    inline void epBank0EnableTransferComplete(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTENSET.bit.TRCPT0 = 1; }

    // Endpoint status
    inline bool epBank1IsReady(uint8_t ep) { return usb.DeviceEndpoint[ep].EPSTATUS.bit.BK1RDY; }
    inline void epBank1SetReady(uint8_t ep) { usb.DeviceEndpoint[ep].EPSTATUSSET.bit.BK1RDY = 1; }
    inline void epBank1ResetReady(uint8_t ep) { usb.DeviceEndpoint[ep].EPSTATUSCLR.bit.BK1RDY = 1; }
    inline bool epBank0IsReady(uint8_t ep) { return usb.DeviceEndpoint[ep].EPSTATUS.bit.BK0RDY; }
    inline void epBank0SetReady(uint8_t ep) { usb.DeviceEndpoint[ep].EPSTATUSSET.bit.BK0RDY = 1; }
    inline void epBank0ResetReady(uint8_t ep) { usb.DeviceEndpoint[ep].EPSTATUSCLR.bit.BK0RDY = 1; }

    // Endpoint stall
    inline bool epBank1IsStalled(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.bit.STALL1; }
    inline void epBank1DisableStalled(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTENCLR.bit.STALL1 = 1; }
    inline bool epBank0IsStalled(uint8_t ep) { return usb.DeviceEndpoint[ep].EPINTFLAG.bit.STALL0; }
    inline void epBank0DisableStalled(uint8_t ep) { usb.DeviceEndpoint[ep].EPINTENCLR.bit.STALL0 = 1; }

    // Endpoint packets
    inline uint16_t epBank0ByteCount(uint8_t ep) { return EP[ep].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT; }
    inline void epBank0SetByteCount(uint8_t ep, uint16_t bc) { EP[ep].DeviceDescBank[0].PCKSIZE.bit.BYTE_COUNT = bc; }
    inline void epBank1SetByteCount(uint8_t ep, uint16_t bc) { EP[ep].DeviceDescBank[1].PCKSIZE.bit.BYTE_COUNT = bc; }

    inline void epBank0SetAddress(uint8_t ep, void *addr) { EP[ep].DeviceDescBank[0].ADDR.reg = (uint32_t)addr; }
    inline void epBank1SetAddress(uint8_t ep, void *addr) { EP[ep].DeviceDescBank[1].ADDR.reg = (uint32_t)addr; }

    inline void epBank0SetSize(uint8_t ep, uint16_t size) { 
        EP[ep].DeviceDescBank[0].PCKSIZE.bit.SIZE = ep_pcksize_size(size); 
    }
    inline void epBank1SetSize(uint8_t ep, uint16_t size) { 
        EP[ep].DeviceDescBank[1].PCKSIZE.bit.SIZE = ep_pcksize_size(size); 
    }

    inline void epBank0SetMultiPacketSize(uint8_t ep, uint16_t s) {
        EP[ep].DeviceDescBank[0].PCKSIZE.bit.MULTI_PACKET_SIZE = s;
    }
    inline void epBank1SetMultiPacketSize(uint8_t ep, uint16_t s) {
        EP[ep].DeviceDescBank[1].PCKSIZE.bit.MULTI_PACKET_SIZE = s;
    }

    inline void epBank1EnableAutoZLP(uint8_t ep) { EP[ep].DeviceDescBank[1].PCKSIZE.bit.AUTO_ZLP = 1; }

    // Endpoint transaction helpers
    inline void epReleaseOutBank0(uint8_t ep, uint16_t s) {
        epBank0SetMultiPacketSize(ep, s);
        epBank0SetByteCount(ep, 0);
        epBank0ResetReady(ep);
    }

private:
    UsbDevice &usb;
    __attribute__((aligned(4))) UsbDeviceDescriptor EP[USB_EPT_NUM];

    static uint8_t ep_pcksize_size(uint16_t size) {
        switch (size) {
            case 8: return 0;
            case 16: return 1;
            case 32: return 2;
            case 64: return 3;
            case 128: return 4;
            case 256: return 5;
            case 512: return 6;
            default: return 0;
        }
    }
};

inline void USBDevice_SAMD21::reset() {
    usb.CTRLA.bit.SWRST = 1;
    memset(EP, 0, sizeof(EP));
    while (usb.SYNCBUSY.bit.SWRST) {}
    usb.DESCADD.reg = (uint32_t)(EP);
}

inline void USBDevice_SAMD21::calibrate() {
    // INTENTIONALLY HARDCODED - bad chips out there!
    usb.PADCAL.bit.TRANSP = 29;
    usb.PADCAL.bit.TRANSN = 5;
    usb.PADCAL.bit.TRIM   = 3;
}

// ============================================================================
// USBSerial - CDC-ACM serial (extracted from ArduinoCore SAMD, no deps)
// Fully interrupt-driven: ISR handles all USB events. Main thread just reads/writes.
// No tusb_task() polling needed. User calls chip.usb.print("hello") and it works.
// ============================================================================

class USBSerial {
public:
    // Initialize USB device + CDC endpoints. Call once from Chip::init().
    void usb_init() {
        PM->APBBMASK.reg |= PM_APBBMASK_USB;

        PORT->Group[0].PINCFG[PIN_PA24G_USB_DM].bit.PMUXEN = 1;
        PORT->Group[0].PMUX[PIN_PA24G_USB_DM / 2].reg &= ~(0xF << (4 * (PIN_PA24G_USB_DM & 0x01u)));
        PORT->Group[0].PMUX[PIN_PA24G_USB_DM / 2].reg |= MUX_PA24G_USB_DM << (4 * (PIN_PA24G_USB_DM & 0x01u));
        PORT->Group[0].PINCFG[PIN_PA25G_USB_DP].bit.PMUXEN = 1;
        PORT->Group[0].PMUX[PIN_PA25G_USB_DP / 2].reg &= ~(0xF << (4 * (PIN_PA25G_USB_DP & 0x01u)));
        PORT->Group[0].PMUX[PIN_PA25G_USB_DP / 2].reg |= MUX_PA25G_USB_DP << (4 * (PIN_PA25G_USB_DP & 0x01u));

        GCLK->CLKCTRL.reg = GCLK_CLKCTRL_ID(6) | GCLK_CLKCTRL_GEN_GCLK1 | GCLK_CLKCTRL_CLKEN;
        while (GCLK->STATUS.bit.SYNCBUSY) {}

        usbd.reset();
        usbd.calibrate();
        usbd.setDataSensitiveQoS();
        usbd.setConfigSensitiveQoS();
        usbd.setUSBDeviceMode();
        usbd.runInStandby();
        usbd.setFullSpeed();

        NVIC_SetPriority((IRQn_Type)USB_IRQn, 0UL);
        NVIC_EnableIRQ((IRQn_Type)USB_IRQn);

        usbd.attach();
        usbd.enableEndOfResetInterrupt();
        usbd.enableStartOfFrameInterrupt();
        usbd.enable();

        _usbConfiguration = 0;
        initEP(0, USB_ENDPOINT_TYPE_CONTROL);
    }

    // Main ISR handler - called from vector table. Handles ALL USB events inline.
    void ISRHandler() {
        // End-Of-Reset
        if (usbd.isEndOfResetInterrupt()) {
            usbd.ackEndOfResetInterrupt();
            initEP(0, USB_ENDPOINT_TYPE_CONTROL);
            usbd.epBank0EnableSetupReceived(0);
            _usbConfiguration = 0;
        }

        // Start-Of-Frame (unused but ack'd)
        if (usbd.isStartOfFrameInterrupt()) {
            usbd.ackStartOfFrameInterrupt();
        }

        // Clear stalls on EP0
        if (usbd.epBank1IsStalled(0)) usbd.epBank1DisableStalled(0);
        if (usbd.epBank0IsStalled(0)) usbd.epBank0DisableStalled(0);

        // Setup packet received on EP0
        if (usbd.epBank0IsSetupReceived(0)) {
            USBSetup setup;
            memcpy(&setup, udd_ep_out_cache_buffer[0], sizeof(USBSetup));

            usbd.epBank0SetByteCount(0, 0);
            usbd.epBank0ResetReady(0);

            bool ok = false;
            if (REQUEST_STANDARD == (setup.bmRequestType & REQUEST_TYPE)) {
                ok = handleStandardSetup(setup);
            } else {
                ok = handleCDCSetup(setup);
            }

            // All handlers that return true already call ctrl_send() which handles
            // data stage + status IN inline. Only stall on failure.
            if (!ok) {
                stallEP(0);
            }

            if (usbd.epBank1IsStalled(0)) usbd.epBank1DisableStalled(0);
            // Ack RXSTP only — don't clear TRCPT flags that ctrl_send() polls for
            usbd.epBank0AckSetupReceived(0);
        }
        // Handle EP0 OUT completion (host status stage) independently of setup
        if (usbd.epBank0IsTransferComplete(0)) {
            usbd.epBank0AckTransferComplete(0);
            usbd.epBank0ResetReady(0);
            usbd.epBank0SetReady(0);  // Re-arm for next OUT/setup
        }

        // Data endpoint interrupts (only after configuration)
        if (_usbConfiguration != 0) {
            for (int ep = 1; ep < USB_EPT_NUM; ep++) {
                if (usbd.epHasPendingInterrupts(ep)) {
                    handleEPData(ep);
                }
            }

            // Drain RX double-buffer into ring buffer
            drainRX();
        }
    }

    // ---- Public serial API ----

    bool configured() const { return _usbConfiguration != 0; }

    int available() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        size_t avail = _rx_buffer.available();
        if (saved == 0) __enable_irq();
        return static_cast<int>(avail) + (_peek != -1);
    }

    int availableForWrite() { return EPX_SIZE - 1; }

    int peek() {
        if (_peek != -1) return _peek;
        _peek = read();
        return _peek;
    }

    int read() {
        if (_peek != -1) {
            int c = _peek;
            _peek = -1;
            return c;
        }
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        size_t avail = _rx_buffer.available();
        int c = -1;
        if (avail > 0) {
            c = _rx_buffer.read_char();
        }
        if (saved == 0) __enable_irq();
        return c;
    }

    void flush() {
        if (usbd.epBank1IsReady(CDC_ENDPOINT_IN)) {
            usbd.epBank1SetReady(CDC_ENDPOINT_IN);
            usbd.epBank1AckTransferComplete(CDC_ENDPOINT_IN);
        }
    }

    size_t write(uint8_t c) { return write(&c, 1); }

    size_t write(const uint8_t *buffer, size_t size) {
        if (!_usbConfiguration || !dtr() || size == 0) return 0;
        
        const uint8_t *ptr = buffer;
        size_t total = 0;

        while (size > 0) {
            bool bankReady = usbd.epBank1IsReady(CDC_ENDPOINT_IN);
            
            if (bankReady) {
                // Wait for transfer completion with timeout (~70ms)
                uint32_t timeout = microsecondsToClockCycles(70000) / 23;
                while (!usbd.epBank1IsTransferComplete(CDC_ENDPOINT_IN)) {
                    if (timeout-- == 0) {
                        usbd.epBank1ResetReady(CDC_ENDPOINT_IN);
                        return total > 0 ? total : 0;
                    }
                }
            }

            uint32_t len = (size >= EPX_SIZE) ? EPX_SIZE : size;

            if (size >= EPX_SIZE) {
                usbd.epBank1EnableAutoZLP(CDC_ENDPOINT_IN);
            }

            memcpy(&udd_ep_in_cache_buffer[CDC_ENDPOINT_IN], ptr, len);
            usbd.epBank1SetAddress(CDC_ENDPOINT_IN, &udd_ep_in_cache_buffer[CDC_ENDPOINT_IN]);
            usbd.epBank1SetByteCount(CDC_ENDPOINT_IN, len);
            usbd.epBank1AckTransferComplete(CDC_ENDPOINT_IN);
            usbd.epBank1SetReady(CDC_ENDPOINT_IN);

            total += len;
            ptr += len;
            size -= len;
        }
        return total;
    }

    // Line coding accessors
    uint32_t baud() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        uint32_t r = _lineInfo.dwDTERate;
        if (saved == 0) __enable_irq();
        return r;
    }
    uint8_t stopbits() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        uint8_t r = _lineInfo.bCharFormat;
        if (saved == 0) __enable_irq();
        return r;
    }
    uint8_t paritytype() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        uint8_t r = _lineInfo.bParityType;
        if (saved == 0) __enable_irq();
        return r;
    }
    uint8_t numbits() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        uint8_t r = _lineInfo.bDataBits;
        if (saved == 0) __enable_irq();
        return r;
    }
    
    bool dtr() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        bool r = (_lineState & 0x01) != 0;
        if (saved == 0) __enable_irq();
        return r;
    }
    bool rts() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        bool r = (_lineState & 0x02) != 0;
        if (saved == 0) __enable_irq();
        return r;
    }

    operator bool() {
        uint8_t saved = __get_PRIMASK();
        __disable_irq();
        bool r = (_lineState & 0x01) != 0;
        if (saved == 0) __enable_irq();
        return r;
    }

private:
    USBDevice_SAMD21 usbd;
    RingBuffer _rx_buffer;
    
    volatile uint32_t _usbConfiguration;
    int _peek = -1;
    volatile LineInfo _lineInfo = { 115200, 0x00, 0x00, 0x08 };
    volatile uint8_t _lineState;

    // Static buffers (aligned for USB DMA)
    alignas(4) static uint8_t udd_ep_out_cache_buffer[7][64];
    alignas(4) static uint8_t udd_ep_in_cache_buffer[7][64];
    static uint8_t EndPoints[];

    // ---- Descriptor helpers (from USBCore.cpp) ----

    bool sendStringDescriptor(const char *str, uint32_t maxlen) {
        if (maxlen < 2) return false;
        
        size_t slen = strlen(str);
        size_t dlen = slen * 2 + 2;
        if (dlen > maxlen) dlen = maxlen;

        // Convert to UTF-16LE in-place on stack buffer
        uint8_t buf[256];
        buf[0] = static_cast<uint8_t>(dlen);
        buf[1] = USB_STRING_DESCRIPTOR_TYPE;
        
        size_t i = 2;
        while (i < dlen && slen > 0) {
            buf[i++] = static_cast<uint8_t>(*str);
            buf[i++] = 0;
            str++;
            slen--;
        }

        ctrl_send(buf, dlen);
        return true;
    }

    bool sendDescriptor(const USBSetup &setup) {
        uint8_t type = setup.wValue >> 8;

        if (type == USB_CONFIGURATION_DESCRIPTOR_TYPE) {
            return sendConfiguration(setup.wLength);
        }

        if (type == USB_DEVICE_DESCRIPTOR_TYPE) {
            const DeviceDescriptor *desc;
            if (setup.wLength == 8) {
                desc = &DEVICE_DESC_COMPOSITE;
            } else {
                desc = &DEVICE_DESC_STANDARD;
            }
            
            uint8_t len = desc->len;
            if (len > setup.wLength) len = setup.wLength;
            ctrl_send(reinterpret_cast<const uint8_t*>(desc), len);
            return true;
        }

        if (type == USB_STRING_DESCRIPTOR_TYPE) {
            uint8_t idx = setup.wValue & 0xFF;
            if (idx == 0) {
                ctrl_send(reinterpret_cast<const uint8_t*>(&STRING_LANG), 
                          (setup.wLength < sizeof(STRING_LANG)) ? setup.wLength : sizeof(STRING_LANG));
                return true;
            }
            if (idx == IPRODUCT) return sendStringDescriptor(STR_PRODUCT, setup.wLength);
            if (idx == IMANUFACTURER) return sendStringDescriptor(STR_MANUFACTURER, setup.wLength);
            if (idx == ISERIAL) {
                char sn[33];
                getSerialNumber(sn);
                return sendStringDescriptor(sn, setup.wLength);
            }
            return false;
        }

        return false;
    }

    bool sendConfiguration(uint32_t maxlen) {
        CDCDescriptor cdc = buildCDCDescriptor();
        
        uint16_t total = sizeof(ConfigDescriptor) + sizeof(CDCDescriptor);
        ConfigDescriptor cfg = D_CONFIG(total, 2);

        if (maxlen == sizeof(ConfigDescriptor)) {
            ctrl_send(reinterpret_cast<const uint8_t*>(&cfg), sizeof(cfg));
            return true;
        }

        // Send config header then CDC descriptors in one go
        uint8_t buf[256];
        memcpy(buf, &cfg, sizeof(cfg));
        memcpy(buf + sizeof(cfg), &cdc, sizeof(cdc));
        ctrl_send(buf, total);
        return true;
    }

    CDCDescriptor buildCDCDescriptor() {
        CDCDescriptor cdc = {
            D_IAD(CDC_ACM_INTERFACE, 2, CDC_COMM_CLASS, CDC_ACM_SUBCLASS, 0),
            D_INTERFACE(CDC_ACM_INTERFACE, 1, CDC_COMM_CLASS, CDC_ACM_SUBCLASS, 0),
            D_CDCCS(0x00, CDC_V1_10 & 0xFF, (CDC_V1_10 >> 8) & 0xFF),
            D_CDCCS4(0x02, 6),
            D_CDCCS(0x06, CDC_ACM_INTERFACE, CDC_DATA_INTERFACE),
            D_CDCCS(0x01, 1, 1),
            D_ENDPOINT(USB_ENDPOINT_IN(CDC_ENDPOINT_ACM), USB_ENDPOINT_TYPE_INTERRUPT, 0x10, 0x10),
            D_INTERFACE(CDC_DATA_INTERFACE, 2, CDC_DATA_CLASS, 0, 0),
            D_ENDPOINT(USB_ENDPOINT_OUT(CDC_ENDPOINT_OUT), USB_ENDPOINT_TYPE_BULK, EPX_SIZE, 0),
            D_ENDPOINT(USB_ENDPOINT_IN(CDC_ENDPOINT_IN), USB_ENDPOINT_TYPE_BULK, EPX_SIZE, 0)
        };
        return cdc;
    }

    // ---- Control transfer (from USBCore.cpp) ----

    void ctrl_send_in(uint8_t ep, const uint8_t *data, uint32_t len) {
        memcpy(&udd_ep_in_cache_buffer[ep], data, len);
        usbd.epBank1SetAddress(ep, &udd_ep_in_cache_buffer[ep]);
        usbd.epBank1SetMultiPacketSize(ep, 0);
        usbd.epBank1SetByteCount(ep, len);
    }

    void ctrl_send(const uint8_t *data, uint32_t len) {
        uint32_t pos = 0;
        while (len > 0) {
            uint32_t chunk = (len > EPX_SIZE) ? EPX_SIZE : len;
            ctrl_send_in(0, data + pos, chunk);

            usbd.epBank1SetAddress(0, &udd_ep_in_cache_buffer[0]);
            usbd.epBank1SetByteCount(0, chunk);
            usbd.epBank1AckTransferComplete(0);
            usbd.epBank1SetReady(0);

            while (!usbd.epBank1IsTransferComplete(0)) {}

            pos += chunk;
            len -= chunk;
        }

        // ZLP if needed (exact multiple of max packet or zero-length status IN)
        if (len == 0 && (pos & (EPX_SIZE - 1)) == 0) {
            usbd.epBank1SetByteCount(0, 0);
            usbd.epBank1AckTransferComplete(0);
            usbd.epBank1SetReady(0);
            while (!usbd.epBank1IsTransferComplete(0)) {}
        }
    }

    uint8_t ctrl_recv(uint8_t ep) {
        usbd.epBank0SetAddress(ep, &udd_ep_out_cache_buffer[ep]);
        usbd.epBank0SetMultiPacketSize(ep, EPX_SIZE);
        usbd.epBank0SetByteCount(ep, 0);
        usbd.epBank0ResetReady(ep);
        while (!usbd.epBank0IsReady(ep)) {}
        while (!usbd.epBank0IsTransferComplete(ep)) {}
        return static_cast<uint8_t>(usbd.epBank0ByteCount(ep));
    }

    void stallEP(uint8_t ep) {
        USB->DEVICE.DeviceEndpoint[ep].EPSTATUSSET.reg = USB_DEVICE_EPSTATUSSET_STALLRQ(2);
    }

    // ---- Standard request handler (from USBCore.cpp) ----

    bool handleStandardSetup(USBSetup &setup) {
        switch (setup.bRequest) {
        case GET_STATUS:
            if (setup.bmRequestType == 0 || setup.bmRequestType == REQUEST_ENDPOINT) {
                uint8_t buff[2] = { 0, 0 };
                ctrl_send(buff, 2);
                return true;
            }
            break;

        case CLEAR_FEATURE:
            if (setup.wValue == 1) { // Remote wakeup
                uint8_t buff[2] = { 0, 0 };
                ctrl_send(buff, 2);
                return true;
            } else { // Endpoint halt
                ctrl_send(nullptr, 0);
                return true;
            }

        case SET_FEATURE:
            if (setup.wValue == 1 || setup.wValue == 0) {
                ctrl_send(nullptr, 0);
                return true;
            }
            break;

        case SET_ADDRESS_REQ:
            // Complete current control transfer first
            usbd.epBank1SetByteCount(0, 0);
            usbd.epBank1AckTransferComplete(0);
            usbd.epBank1SetReady(0);
            while (!usbd.epBank1IsTransferComplete(0)) {}
            usbd.setAddress(setup.wValue);
            return true;

        case GET_DESCRIPTOR:
            return sendDescriptor(setup);

        case GET_CONFIGURATION: {
            uint32_t cfg = _usbConfiguration;
            ctrl_send(reinterpret_cast<const uint8_t*>(&cfg), 1);
            return true;
        }

        case SET_CONFIGURATION:
            if (REQUEST_DEVICE == (setup.bmRequestType & REQUEST_RECIPIENT)) {
                initEndpoints();
                _usbConfiguration = setup.wValue;
                enableCDCInterrupts();
                ctrl_send(nullptr, 0);
                return true;
            }
            break;

        case GET_INTERFACE:
            ctrl_send(nullptr, 0);
            return true;

        case SET_INTERFACE:
            ctrl_send(nullptr, 0);
            return true;
        }
        return false;
    }

    // ---- CDC request handler (from CDC.cpp) ----

    bool handleCDCSetup(USBSetup &setup) {
        if (setup.wIndex != CDC_ACM_INTERFACE) return false;

        uint8_t reqType = setup.bmRequestType;
        uint8_t req = setup.bRequest;

        // Isolate the direction bit (Bit 7) to check if it's Device-to-Host (0x80) or Host-to-Device (0x00)
        uint8_t direction = reqType & 0x80;

        if (direction == REQUEST_DEVICETOHOST) {
            if (req == CDC_GET_LINE_CODING) {
                LineInfo info;
                uint8_t saved = __get_PRIMASK();
                __disable_irq();
                memcpy(&info, const_cast<LineInfo*>(&_lineInfo), sizeof(info));
                if (saved == 0) __enable_irq();
                ctrl_send(reinterpret_cast<const uint8_t*>(&info), 7);
                return true;
            }
        }

        if (direction == REQUEST_HOSTTODEVICE) {
            if (req == CDC_SET_LINE_CODING) {
                uint8_t len = ctrl_recv(0);
                LineInfo info;
                memcpy(&info, udd_ep_out_cache_buffer[0], 
                    (len < sizeof(info)) ? len : sizeof(info));
                uint8_t saved = __get_PRIMASK();
                __disable_irq();
                memcpy(const_cast<LineInfo*>(&_lineInfo), &info, sizeof(info));
                if (saved == 0) __enable_irq();
            }

            if (req == CDC_SET_CONTROL_LINE_STATE) {
                uint32_t savedRate;
                uint8_t newState;
                uint8_t saved = __get_PRIMASK();
                __disable_irq();
                savedRate = _lineInfo.dwDTERate;
                newState = setup.wValue;
                _lineState = newState;
                if (saved == 0) __enable_irq();

                // 1200-baud auto-reset into bootloader
                if (savedRate == 1200 && (newState & 0x01) == 0) {
                    *bootloader_magic_ptr() = BOOTLOADER_MAGIC_VALUE;
                    NVIC_SystemReset();
                    for (;;) {}
                }
            }

            if (req == CDC_SEND_BREAK) {
                // Break signal received - ignored in bare-metal
            }

            if (req == CDC_SET_LINE_CODING || req == CDC_SET_CONTROL_LINE_STATE || 
                req == CDC_SEND_BREAK) {
                ctrl_send(nullptr, 0);
                return true;
            }
        }

        return false;
}

    // ---- Endpoint initialization (from USBCore.cpp) ----

    void initEP(uint8_t ep, uint32_t config) {
        if (config == (USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0))) {
            usbd.epBank1SetSize(ep, EPX_SIZE);
            usbd.epBank1SetAddress(ep, &udd_ep_in_cache_buffer[ep]);
            usbd.epBank1SetType(ep, 4); // INTERRUPT IN
        }
        else if (config == (USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0))) {
            initEPBulkOut(ep);
        }
        else if (config == (USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0))) {
            usbd.epBank1SetSize(ep, EPX_SIZE);
            usbd.epBank1SetAddress(ep, &udd_ep_in_cache_buffer[ep]);
            usbd.epBank1SetType(ep, 3); // BULK IN
        }
        else if (config == USB_ENDPOINT_TYPE_CONTROL) {
            usbd.epBank0SetSize(ep, EPX_SIZE);
            usbd.epBank0SetAddress(ep, &udd_ep_out_cache_buffer[ep]);
            usbd.epBank0SetType(ep, 1); // CONTROL OUT/SETUP

            usbd.epBank1SetSize(ep, EPX_SIZE);
            usbd.epBank1SetAddress(ep, &udd_ep_in_cache_buffer[ep]);
            usbd.epBank1SetType(ep, 1); // CONTROL IN

            usbd.epReleaseOutBank0(ep, EPX_SIZE);
        }
    }

    void initEndpoints() {
        for (uint8_t i = 1; i < 7; i++) {
            if (EndPoints[i] != 0) {
                initEP(i, EndPoints[i]);
            }
        }
    }

    // ---- BULK OUT endpoint with ring buffer (from SAMD21_USBDevice.h + CDC.cpp) ----

    void initEPBulkOut(uint8_t ep) {
        _ep_bulkout = ep;
        _ep_out_current = 0;
        _ep_out_incoming = 0;
        _ep_out_first0 = 0;
        _ep_out_last0 = 0;
        _ep_out_ready0 = false;
        _ep_out_first1 = 0;
        _ep_out_last1 = 0;
        _ep_out_ready1 = false;
        _ep_out_notify = false;

        usbd.epBank0SetSize(ep, EPX_SIZE);
        usbd.epBank0SetType(ep, 3); // BULK OUT
        usbd.epBank0SetAddress(ep, const_cast<uint8_t*>(_ep_out_data0));
        usbd.epBank0EnableTransferComplete(ep);
        
        releaseOutEP(ep);
    }

    void enableCDCInterrupts() {
        usbd.epBank0EnableTransferComplete(CDC_ENDPOINT_OUT);
    }

    void handleEPData(uint8_t ep) {
        if (ep == _ep_bulkout && usbd.epBank0IsTransferComplete(ep)) {
            usbd.epBank0AckTransferComplete(ep);
            uint32_t received = usbd.epBank0ByteCount(ep);
            
            if (received == 0) {
                releaseOutEP(ep);
            } else if (_ep_out_incoming == 0) {
                _ep_out_last0 = received;
                _ep_out_incoming = 1;
                usbd.epBank0SetAddress(ep, const_cast<uint8_t*>(_ep_out_data1));

                uint8_t saved = __get_PRIMASK();
                //__disable_irq();
                _ep_out_ready0 = true;
                bool notify = _ep_out_ready1;
                if (!notify) {
                    releaseOutEP(ep);
                }
                _ep_out_notify = notify;
                if (saved == 0) __enable_irq();
            } else {
                _ep_out_last1 = received;
                _ep_out_incoming = 0;
                usbd.epBank0SetAddress(ep, const_cast<uint8_t*>(_ep_out_data0));

                uint8_t saved = __get_PRIMASK();
                //__disable_irq();
                _ep_out_ready1 = true;
                bool notify = _ep_out_ready0;
                if (!notify) {
                    releaseOutEP(ep);
                }
                _ep_out_notify = notify;
                if (saved == 0) __enable_irq();
            }
        }
    }

    void drainRX() {
        size_t space = _rx_buffer.availableForStore();

        if (_ep_out_current == 0) {
            if (!_ep_out_ready0) return;

            // Drain data0 into ring buffer
            while (space > 0 && _ep_out_first0 < _ep_out_last0) {
                _rx_buffer.store_char(_ep_out_data0[_ep_out_first0++]);
                space--;
            }

            if (_ep_out_first0 == _ep_out_last0) {
                _ep_out_first0 = 0;
                _ep_out_current = 1;
                _ep_out_ready0 = false;
                if (_ep_out_notify) {
                    _ep_out_notify = false;
                    releaseOutEP(_ep_bulkout);
                }
            }
        } else {
            if (!_ep_out_ready1) return;

            // Drain data1 into ring buffer
            while (space > 0 && _ep_out_first1 < _ep_out_last1) {
                _rx_buffer.store_char(_ep_out_data1[_ep_out_first1++]);
                space--;
            }

            if (_ep_out_first1 == _ep_out_last1) {
                _ep_out_first1 = 0;
                _ep_out_current = 0;
                _ep_out_ready1 = false;
                if (_ep_out_notify) {
                    _ep_out_notify = false;
                    releaseOutEP(_ep_bulkout);
                }
            }
        }
    }

    void releaseOutEP(uint8_t ep) {
        usbd.epReleaseOutBank0(ep, EPX_SIZE);
    }

    // ---- Serial number from chip UID (from CDC.cpp) ----

    static void getSerialNumber(char *name) {
        volatile uint32_t* SN0 = const_cast<volatile uint32_t*>(reinterpret_cast<const volatile uint32_t*>(0x0080A00C));
        volatile uint32_t* SN1 = const_cast<volatile uint32_t*>(reinterpret_cast<const volatile uint32_t*>(0x0080A040));
        volatile uint32_t* SN2 = const_cast<volatile uint32_t*>(reinterpret_cast<const volatile uint32_t*>(0x0080A044));
        volatile uint32_t* SN3 = const_cast<volatile uint32_t*>(reinterpret_cast<const volatile uint32_t*>(0x0080A048));

        uint32_t w0 = *SN0;
        uint32_t w1 = *SN1;
        uint32_t w2 = *SN2;
        uint32_t w3 = *SN3;

        for (int i = 0; i < 8; i++) {
            int d = (w0 >> (4 * (7 - i))) & 0xF;
            name[i] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        for (int i = 0; i < 8; i++) {
            int d = (w1 >> (4 * (7 - i))) & 0xF;
            name[8 + i] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        for (int i = 0; i < 8; i++) {
            int d = (w2 >> (4 * (7 - i))) & 0xF;
            name[16 + i] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        for (int i = 0; i < 8; i++) {
            int d = (w3 >> (4 * (7 - i))) & 0xF;
            name[24 + i] = d > 9 ? 'A' + d - 10 : '0' + d;
        }
        name[32] = '\0';
    }

    // ---- Helper: microseconds to clock cycles (Arduino replacement) ----
    
    static uint32_t microsecondsToClockCycles(uint32_t us) {
        return us * (48000000UL / 1000000UL); // 48 MHz CPU
    }

    // BULK OUT double-buffer state
    uint8_t _ep_bulkout;
    volatile uint32_t _ep_out_current;
    volatile uint32_t _ep_out_incoming;
    
    alignas(4) volatile uint8_t _ep_out_data0[64];
    uint32_t _ep_out_first0;
    volatile uint32_t _ep_out_last0;
    volatile bool _ep_out_ready0;

    alignas(4) volatile uint8_t _ep_out_data1[64];
    uint32_t _ep_out_first1;
    volatile uint32_t _ep_out_last1;
    volatile bool _ep_out_ready1;

    volatile bool _ep_out_notify;
};

// Static member definitions (inline for C++17, allows header inclusion from multiple TUs)
inline uint8_t USBSerial::udd_ep_out_cache_buffer[7][64];
inline uint8_t USBSerial::udd_ep_in_cache_buffer[7][64];
inline uint8_t USBSerial::EndPoints[] = {
    USB_ENDPOINT_TYPE_CONTROL,  // EP0
    (USB_ENDPOINT_TYPE_INTERRUPT | USB_ENDPOINT_IN(0)),   // EP1 - ACM notify
    (USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_OUT(0)),       // EP2 - bulk OUT (RX)
    (USB_ENDPOINT_TYPE_BULK | USB_ENDPOINT_IN(0)),        // EP3 - bulk IN (TX)
    0, 0, 0
};

// Global serial instance (inline variable, C++17)
inline USBSerial chip_usb_cdc;

// ISR handler entry point for vector table (inline to avoid multiple definitions)
extern "C" inline void USBSerial_ISRHandler() {
    chip_usb_cdc.ISRHandler();
}
