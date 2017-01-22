#ifndef rx5808_h
#define rx5808_h

#pragma once
#include "Arduino.h"

#define FREQUENCY_MINIMUM	5600
#define FREQUENCY_MAXIMUM	6000

// RTC6715 datasheet defines RSSI output to be
// 0.5 v to 1.1 v (-91 dBm to 5 dBm)
#define RSSI_MIN	0.5 // raw adc reading of ~102
#define RSSI_MAX	1.1 // raw adc reading of ~225

#define DEFAULT_MIN_RSSI 90
#define DEFAULT_MAX_RSSI 220

#define RTC6715_SYNTHESIZER_REGISTER_A					0x00
#define RTC6715_SYNTHESIZER_REGISTER_B					0x01
#define RTC6715_SYNTHESIZER_REGISTER_C					0x02
#define RTC6715_SYNTHESIZER_REGISTER_D					0x03
#define RTC6715_VCO_SWITCH_CAP_CONTROL_REGISTER			0x04
#define RTC6715_DFC_CONTROL_REGISTER					0x05
#define RTC6715_6M_AUDIO_DEMODULATOR_CONTROL_REGISTER	0x06
#define RTC6715_6M5_AUDIO_DEMODULATOR_CONTROL_REGISTER	0x07
#define RTC6715_RECEIVER_CONTROL_REGISTER_1				0x08
#define RTC6715_RECEIVER_CONTROL_REGISTER_2				0x09
#define RTC6715_POWER_DOWN_CONTROL_REGISTER				0x0A
#define RTC6715_STATUS_REGISTER							0x0F

class RX5808
{
public:
	// constructor
	RX5808(void);

	void begin(	
				uint8_t clockPin,
				uint8_t dataPin,
				uint8_t chipSelectPin,
				uint8_t rssiPin);

	// read current signal strength
	uint16_t GetRSSI(uint8_t averages);

	// set the reciever frequency
	uint16_t SetFrequencyMHz(uint16_t frequency);

	uint16_t CurrentFrequencyMHz(void);
	
	uint16_t minRSSI;
	uint16_t maxRSSI;

private:
	// interface pins
	uint8_t	_pChipSelect;
	uint8_t	_pClock;
	uint8_t	_pData;
	uint8_t	_pRSSI;		// analog feedback

	float _frequency;

	uint32_t _ReadRegister(uint8_t address);
	void _WriteRegister(uint8_t address, uint32_t data);

	void _SendAddress(uint8_t address);
	void _SendData(uint32_t data);
	void _ShiftDataOut(uint32_t d, uint8_t s);
	uint32_t _ReceiveData(void);
	void _StrobeClock(void);
	void _ChipSelectAssert(void); 
	void _ChipSelectDeassert(void);
};

#endif