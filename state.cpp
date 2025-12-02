
#include "state.hpp"

#include "pico/flash.h"
#include "hardware/flash.h"
#include "pico/time.h"

size_t firstUnusedByte = 0;
int scanTime = 0;
int lastError = 0;

// Initialize state with default values
State currentState = State(ENVIRONMENT, CELSIUS);
State pendingState = State(ENVIRONMENT, CELSIUS);

static const size_t RANGE_SIZE = 0x100;

static void saveState(void* param) {
    uint8_t data[RANGE_SIZE];
    for(size_t i = 0; i < RANGE_SIZE; i++)
    {
        data[i] = 0xFF;
    }

    // Reached the end of the available EEPROM PAGE need to erase it
    if(firstUnusedByte >= PAGE_SIZE) {
        firstUnusedByte = 0;
        flash_range_erase(STATE_BEGIN_WRITE, 4096);
    }
    
    size_t flashOffset = firstUnusedByte - firstUnusedByte % RANGE_SIZE;
    size_t stateOffsetInRange = firstUnusedByte % RANGE_SIZE;

    for(size_t i = 0; i < stateOffsetInRange; i++) {
        data[i] = 0x00;
    }

    data[stateOffsetInRange] = pendingState.state[0];
    data[stateOffsetInRange+1] = pendingState.state[1];
    data[stateOffsetInRange+2] = pendingState.state[2];
    data[stateOffsetInRange+3] = pendingState.state[3];

    firstUnusedByte += 4;
    
    flash_range_program(STATE_BEGIN_WRITE + flashOffset, data, RANGE_SIZE);
}

void saveStateIfNeeded(State newState) {
  if(newState.state[0] != currentState.state[0] ||
    newState.state[1] != currentState.state[1] ||
    newState.state[2] != currentState.state[2] ||
    newState.state[3] != currentState.state[3] 
  )
  {
    pendingState = newState;
    lastError = flash_safe_execute(saveState, NULL, 500);
    currentState = pendingState;
  }
}

State loadState() {
  auto scan_start = get_absolute_time();
  uint8_t* ptr = (uint8_t*)STATE_BEGIN_READ;
  while(*ptr != 0xFF && ptr < STATE_END_READ) {
    ptr += 4;
  }
  
  firstUnusedByte = ptr - STATE_BEGIN_READ;

  // Move back one slot to find the valid data
  ptr -= 4;

  auto scan_end = get_absolute_time();
  scanTime = absolute_time_diff_us(scan_start, scan_end);
  
  return State(ptr);
}

State::State(MODE mode, UNIT units)
{
    state[0] = USED_SLOT;

    switch(mode) {
        case INCLINOMETER:
            state[0] |= MODE_INCLINOMETER;
            break;
        case ENVIRONMENT:
        default:
            state[0] |= MODE_ENVIRONMENT;
            break;
    }

    switch(units) {
        case FAHRENHEIT:
            state[0] |= UNITS_FAHRENHEIT;
            break;
        case CELSIUS:
        default:
            state[0] |= UNITS_CELSIUS;
            break;
    }
    
    // BYTES 1-3 UNUSED
    state[1] = state[2] = state[3] = 0x00;
}

State::State(uint8_t *data)
{
    state[0] = data[0];
    state[1] = data[1];
    state[2] = data[2];
    state[3] = data[3];
}

UNIT State::getUnits()
{
    if(state[0] & USED_SLOT) {
        if(state[0] & UNITS_CELSIUS) return CELSIUS;
        else if (state[0] & UNITS_FAHRENHEIT) return FAHRENHEIT;
    }

    return CELSIUS;
}

MODE State::getMode()
{
    if(state[0] & USED_SLOT) {
        if(state[0] & MODE_ENVIRONMENT) return ENVIRONMENT;
        else if (state[0] & MODE_INCLINOMETER) return INCLINOMETER;
    }

    return ENVIRONMENT;
}
