#include "Rx5808.h"

// PUBLIC FUNCTIONS
Rx5808::Rx5808(void) {
    minRSSI = DEFAULT_MIN_RSSI;
    maxRSSI = DEFAULT_MAX_RSSI;
}

void Rx5808::begin( uint8_t clockPin,
                    uint8_t dataPin,
                    uint8_t chipSelectPin, 
                    uint8_t rssiPin )
{
    pClock = clockPin;
    pData = dataPin;
    pChipSelect = chipSelectPin; 
    pRSSI = rssiPin;
    frequency = 0;

    pinMode(pChipSelect, OUTPUT); chipSelectDeassert();
    pinMode(pClock, OUTPUT); digitalWrite(pClock, LOW);
    pinMode(pData, OUTPUT); digitalWrite(pData, LOW);

    pinMode(pRSSI, INPUT_PULLUP);    

    setFrequencyMHz(5600);

    // TODO: set Synthesizer Register A
}

uint16_t Rx5808::getRSSI(uint8_t averages) {
    uint32_t sum = 0;
    
    for (uint8_t count = 0; count < averages; count++)
        sum += analogRead(pRSSI);

    return (sum / averages);
}

uint16_t Rx5808::setFrequencyMHz(uint16_t freq) {
    uint32_t fLo, synRFnReg, synRFaReg;
    uint32_t synthesizerRegisterB;

    fLo = (freq - 479) / 2;
    synRFnReg = fLo / 32;
    synRFaReg = fLo % 32;

    synthesizerRegisterB = synRFnReg << 7;
    synthesizerRegisterB += synRFaReg;

    writeRegister(RTC6715_SYNTHESIZER_REGISTER_B, synthesizerRegisterB);

    uint32_t setFrequency = synRFnReg << 5; // multiply by 32
    setFrequency += synRFaReg;
    setFrequency *= 2;
    setFrequency += 479;

    frequency = setFrequency;

    return  frequency;
}

uint16_t Rx5808::currentFrequencyMHz(void) {
    return frequency;
}

// PRIVATE FUNCTIONS
uint32_t Rx5808::_readRegister(uint8_t address) {
    uint32_t data = 0;

    chipSelectDeassert();          // shoudln't really need this as we shold have left the bus in an inactive state

    digitalWrite(pClock, LOW);     // ensure clock pin is low to start
    pinMode(pData, OUTPUT);        // ensure data pin is an output to start

    chipSelectAssert();            // assert chip select

    sendAddress(address);          // send address

    digitalWrite(pData, LOW);      // set R/W bit low as this is a read
    strobeClock();

    data = receiveData();

    chipSelectDeassert();          // leave the chip inactive

    return data;
}

void Rx5808::writeRegister(uint8_t address, uint32_t data) {
    chipSelectDeassert();          // shoudln't really need this as we shold have left the bus in an inactive state
    
    digitalWrite(pClock, LOW);     // ensure clock pin is low to start
    pinMode(pData, OUTPUT);        // ensure data pin is an output to start
    
    chipSelectAssert();            // assert chip select

    sendAddress(address);          // send address

    digitalWrite(pData, HIGH);     // set R/W bit high as this is a write
    strobeClock();

    sendData(data);                // send data

    chipSelectDeassert();          // leave the chip inactive
}

inline void Rx5808::sendAddress(uint8_t address) {
    shiftDataOut(address, 4);
}

inline void Rx5808::sendData(uint32_t data) {
    shiftDataOut(data, 20);
}

inline void Rx5808::shiftDataOut(uint32_t d, uint8_t s) {
    // FYI: RTC6715 samples incoming data on rising clock edge
    for (; s > 0; s--)
    {
        if (d & 0x1)
            digitalWrite(pData, HIGH);
        else
            digitalWrite(pData, LOW);

        strobeClock();                

        d >>= 1;                    // shift for next bit
    }
}

inline uint32_t Rx5808::receiveData(void) {
    uint32_t d = 0;
    pinMode(pData, INPUT);         // make data pin an input to recieve data

    // FYI: RTC6715 sends data on falling clock edge
    for (uint8_t i = 0; i < 20; i++)
    {
        strobeClock();

        if (digitalRead(pData))
            d |= 0x100000;
        
        d >>= 1;                    // shift for next bit
    }

    return d;
}

inline void Rx5808::strobeClock(void) {
    delayMicroseconds(1);           // SETUP DELAY NEEDED? should be 20 ns minimum (t1)
    digitalWrite(pClock, HIGH);
    delayMicroseconds(1);           // HOLD DLEAY NEEDED? should be 30 ns minimum (t3)
    digitalWrite(pClock, LOW);
    delayMicroseconds(1);           // PREPARE DELAY NEEDED? should be 20 ns minimum between clock rise and next data (t2)
}

inline void Rx5808::chipSelectAssert(void) {
    digitalWrite(pChipSelect, LOW);
    delayMicroseconds(1);           // PREPARE DELAY NEEDED? should be 20 ns minimum (t6)
}

inline void Rx5808::chipSelectDeassert(void) {
    delayMicroseconds(1);           // HOLD DLEAY NEEDED? should be 30 ns minimum (t4)
    digitalWrite(pChipSelect, HIGH);
}
