#pragma once

#include <cstdlib>
#include "pico.h"
#include "types.hpp"

enum SAVED_STATE_FLAGS
{
    // 32 BIT (4 bytes) saved in EEPROM
    // EEPROM is erased to all 1s 0xFF
    // 
    // BYTE 0
    // 
    // VALID?:
    //  11 => ALL BITS 0xFFFFFFFF unused slot
    //  01 => used slot
    //
    // STATE:
    //   000        => ENVIRONMENT
    //   001        => INCLINOMETER
    //   010-111    => RESERVED
    //
    // UN (unit):
    //   0 => CELSIUS
    //   1 => FAHRENHEIT
    //
    // BYTE 1-3 (RESERVED)
    //
    // | 00 | 01 | 02 | 03 | 04 | 05 | 06 | 07 |
    // |  VALID? |    STATE     | UN |
    // | 08 | 09 | 10 | 11 | 12 | 13 | 14 | 15 |
    // | RESERVED                              |
    // | 16 | 17 | 18 | 19 | 20 | 21 | 22 | 23 |
    // | RESERVED                              |
    // | 24 | 25 | 26 | 27 | 28 | 29 | 30 | 31 |
    // | RESERVED                              |
    // 
    EMPTY = 0b00000000,
    
    UNUSED_SLOT         = 0b11111111,
    USED_SLOT           = 0b00000010,

    MODE_ENVIRONMENT   = 0b00000000,
    MODE_INCLINOMETER  = 0b00000100,
    
    UNITS_CELSIUS       = 0b00000000,
    UNITS_FAHRENHEIT    = 0b00100000,
};

static const size_t EEPROM_SIZE = 0x200000; // 2Mb
static const size_t PAGE_SIZE = 0x1000; // 4Kb

static const uint8_t* STATE_BEGIN_READ = (uint8_t*)(XIP_BASE + EEPROM_SIZE - PAGE_SIZE);
static const uint8_t* STATE_END_READ = (uint8_t*)(XIP_BASE + EEPROM_SIZE);

static const size_t STATE_BEGIN_WRITE = EEPROM_SIZE - PAGE_SIZE;

extern size_t firstUnusedByte;
extern int scanTime;
extern int lastError;

struct State {
    uint8_t state[4];

    State(MODE mode, UNIT units);
    State(uint8_t* data);

    UNIT getUnits();
    MODE getMode();
};

void saveStateIfNeeded(State state);
State loadState();