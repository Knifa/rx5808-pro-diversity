#include "RX5808.h"

// PUBLIC FUNCTIONS
RX5808::RX5808(void) :
    minRSSI(DEFAULT_MIN_RSSI),
    maxRSSI(DEFAULT_MAX_RSSI)
{}

void RX5808::begin( uint8_t clockPin,
                    uint8_t dataPin,
                    uint8_t chipSelectPin, 
                    uint8_t rssiPin )
{
    _pClock = clockPin;
    _pData = dataPin;
    _pChipSelect = chipSelectPin; 
    _pRSSI = rssiPin;
    _frequency = 0;

    pinMode(_pChipSelect, OUTPUT); _chipSelectDeassert();
    pinMode(_pClock, OUTPUT); digitalWrite(_pClock, LOW);
    pinMode(_pData, OUTPUT); digitalWrite(_pData, LOW);

    pinMode(_pRSSI, INPUT_PULLUP);    

    setFrequencyMHz(5600);

    // TODO: set Synthesizer Register A
}

uint16_t RX5808::getRSSI(uint8_t averages)
{
    uint32_t sum = 0;
    
    for (uint8_t count = 0; count < averages; count++)
        sum += analogRead(_pRSSI);

    return (sum / averages);
}

uint16_t RX5808::setFrequencyMHz(uint16_t frequency)
{
    uint32_t fLo, synRFnReg, synRFaReg;
    uint32_t synthesizerRegisterB;

    fLo = (frequency - 479) / 2;
    synRFnReg = fLo / 32;
    synRFaReg = fLo % 32;

    synthesizerRegisterB = synRFnReg << 7;
    synthesizerRegisterB += synRFaReg;

    _writeRegister(RTC6715_SYNTHESIZER_REGISTER_B, synthesizerRegisterB);

    uint32_t setFrequency = synRFnReg << 5; // multiply by 32
    setFrequency += synRFaReg;
    setFrequency *= 2;
    setFrequency += 479;

    _frequency = setFrequency;

    return  _frequency;
}

uint16_t RX5808::currentFrequencyMHz(void)
{
    return _frequency;
}

// PRIVATE FUNCTIONS
uint32_t RX5808::_readRegister(uint8_t address)
{
    uint32_t data = 0;

    _chipSelectDeassert();          // shoudln't really need this as we shold have left the bus in an inactive state

    digitalWrite(_pClock, LOW);     // ensure clock pin is low to start
    pinMode(_pData, OUTPUT);        // ensure data pin is an output to start

    _chipSelectAssert();            // assert chip select

    _sendAddress(address);          // send address

    digitalWrite(_pData, LOW);      // set R/W bit low as this is a read
    _strobeClock();

    data = _receiveData();

    _chipSelectDeassert();          // leave the chip inactive

    return data;
}

void RX5808::_writeRegister(uint8_t address, uint32_t data)
{
    _chipSelectDeassert();          // shoudln't really need this as we shold have left the bus in an inactive state
    
    digitalWrite(_pClock, LOW);     // ensure clock pin is low to start
    pinMode(_pData, OUTPUT);        // ensure data pin is an output to start
    
    _chipSelectAssert();            // assert chip select

    _sendAddress(address);          // send address

    digitalWrite(_pData, HIGH);     // set R/W bit high as this is a write
    _strobeClock();

    _sendData(data);                // send data

    _chipSelectDeassert();          // leave the chip inactive
}

inline void RX5808::_sendAddress(uint8_t address)
{
    _shiftDataOut(address, 4);
}

inline void RX5808::_sendData(uint32_t data)
{
    _shiftDataOut(data, 20);
}

inline void RX5808::_shiftDataOut(uint32_t d, uint8_t s)
{
    // FYI: RTC6715 samples incoming data on rising clock edge
    for (; s > 0; s--)
    {
        if (d & 0x1)
            digitalWrite(_pData, HIGH);
        else
            digitalWrite(_pData, LOW);

        _strobeClock();                

        d >>= 1;                    // shift for next bit
    }
}

inline uint32_t RX5808::_receiveData(void)
{
    uint32_t d = 0;
    pinMode(_pData, INPUT);         // make data pin an input to recieve data

    // FYI: RTC6715 sends data on falling clock edge
    for (uint8_t i = 0; i < 20; i++)
    {
        _strobeClock();

        if (digitalRead(_pData))
            d |= 0x100000;
        
        d >>= 1;                    // shift for next bit
    }

    return d;
}

inline void RX5808::_strobeClock(void)
{
    delayMicroseconds(1);           // SETUP DELAY NEEDED? should be 20 ns minimum (t1)
    digitalWrite(_pClock, HIGH);
    delayMicroseconds(1);           // HOLD DLEAY NEEDED? should be 30 ns minimum (t3)
    digitalWrite(_pClock, LOW);
    delayMicroseconds(1);           // PREPARE DELAY NEEDED? should be 20 ns minimum between clock rise and next data (t2)
}

inline void RX5808::_chipSelectAssert(void)
{
    digitalWrite(_pChipSelect, LOW);
    delayMicroseconds(1);           // PREPARE DELAY NEEDED? should be 20 ns minimum (t6)
}

inline void RX5808::_chipSelectDeassert(void)
{
    delayMicroseconds(1);           // HOLD DLEAY NEEDED? should be 30 ns minimum (t4)
    digitalWrite(_pChipSelect, HIGH);
}