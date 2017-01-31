#include <Arduino.h>
#include <avr/pgmspace.h>

#include "settings.h"
#include "settings_eeprom.h"
#include "receiver.h"
#include "channels.h"


static void updateRssiLimits();


namespace Receiver {
    uint8_t activeReceiver = RECEIVER_A;
    uint8_t activeChannel = 0;
    uint32_t lastChannelSwitchTime = 0;

    uint8_t rssiA = 0;
    uint16_t rssiARaw = 0;
    #ifdef USE_DIVERSITY
        uint8_t rssiB = 0;
        uint16_t rssiBRaw = 0;
        #define RECEIVER_COUNT 2
    #else
        #define RECEIVER_COUNT 1
    #endif

    RX5808 hReceivers[RECEIVER_COUNT];

    void begin(void)
    {
        hReceivers[RECEIVER_A].begin(PIN_SPI_CLOCK, PIN_SPI_DATA, PIN_SPI_SLAVE_SELECT_A, PIN_RSSI_A);
        pinMode(PIN_LED_A, OUTPUT);
#ifdef USE_DIVERSITY
        hReceivers[RECEIVER_B].begin(PIN_SPI_CLOCK, PIN_SPI_DATA, PIN_SPI_SLAVE_SELECT_B, PIN_RSSI_B);
        pinMode(PIN_LED_B, OUTPUT);
#endif
    }

    void setChannel(uint8_t channel)
    {
        hReceivers[RECEIVER_A].setFrequencyMHz(Channels::getFrequency(channel));
#if defined(USE_DIVERSITY) && defined(INDIVIDUAL_DIVERSITY_CONTROL)
        hReceivers[RECEIVER_B].setFrequencyMHz(Channels::getFrequency(channel));
#endif

        lastChannelSwitchTime = millis();
        activeChannel = channel;
    }

    void setActiveReceiver(uint8_t receiver) {
        #ifdef USE_DIVERSITY
            digitalWrite(PIN_LED_A, receiver == RECEIVER_A);
            digitalWrite(PIN_LED_B, receiver == RECEIVER_B);
        #else
            digitalWrite(PIN_LED_A, HIGH);
        #endif

        activeReceiver = receiver;
    }

    //
    // Blocks until MIN_TUNE_TIME has been reached since last channel switch.
    //
    void waitForStableRssi() {
        uint16_t timeSinceChannelSwitch = millis() - lastChannelSwitchTime;
        if (timeSinceChannelSwitch < MIN_TUNE_TIME) {
            delay(MIN_TUNE_TIME - timeSinceChannelSwitch);
        }
    }

    uint16_t updateRssi() {
        waitForStableRssi();

        rssiARaw = hReceivers[RECEIVER_A].getRSSI(RSSI_READS);
#ifdef USE_DIVERSITY
        rssiBRaw = hReceivers[RECEIVER_B].getRSSI(RSSI_READS);
#endif

        updateRssiLimits();

        rssiA = map(
            rssiARaw,
            EepromSettings.rssiAMin,
            EepromSettings.rssiAMax,
            1,
            100);
        #ifdef USE_DIVERSITY
            rssiB = map(
                rssiBRaw,
                EepromSettings.rssiBMin,
                EepromSettings.rssiBMax,
                1,
                100);
        #endif
    }

    void setDiversityMode(uint8_t mode) {
        EepromSettings.diversityMode = mode;
        switchDiversity();
    }

    void switchDiversity() {
        static uint8_t diversityCheckTick = 0;
        uint8_t bestReceiver = activeReceiver;

        if (EepromSettings.diversityMode == DIVERSITY_AUTO) {
            uint8_t rssiDiff =
                (int) abs(((rssiA - rssiB) / (float) rssiB) * 100.0f);

            if (rssiDiff >= DIVERSITY_CUTOVER) {
                if(rssiA > rssiB && diversityCheckTick > 0)
                    diversityCheckTick--;

                if(rssiA < rssiB && diversityCheckTick < DIVERSITY_MAX_CHECKS)
                    diversityCheckTick++;

                // Have we reached the maximum number of checks to switch
                // receivers?
                if (diversityCheckTick == 0 ||
                    diversityCheckTick >= DIVERSITY_MAX_CHECKS
                ) {
                    bestReceiver =
                        (diversityCheckTick == 0) ?
                        RECEIVER_A :
                        RECEIVER_B;
                }
            }
        } else {
            switch (EepromSettings.diversityMode) {
                case DIVERSITY_FORCE_A:
                    bestReceiver = RECEIVER_A;
                    break;

                case DIVERSITY_FORCE_B:
                    bestReceiver = RECEIVER_B;
                    break;
            }
        }

        setActiveReceiver(bestReceiver);
    }

    void update() {
        updateRssi();

        #ifdef USE_DIVERSITY
            switchDiversity();
        #endif
    }
}


static void updateRssiLimits() {
    // TEMP: Until logic for setting RSSI max is rewritten.

    if (Receiver::rssiARaw > EepromSettings.rssiAMax)
        EepromSettings.rssiAMax = Receiver::rssiARaw;

    if (Receiver::rssiARaw < EepromSettings.rssiAMin)
        EepromSettings.rssiAMin = Receiver::rssiARaw;

    #ifdef USE_DIVERSITY
        if (Receiver::rssiBRaw > EepromSettings.rssiBMax)
            EepromSettings.rssiBMax = Receiver::rssiBRaw;

        if (Receiver::rssiBRaw < EepromSettings.rssiBMin)
            EepromSettings.rssiBMin = Receiver::rssiBRaw;
    #endif
}
