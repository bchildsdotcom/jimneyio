#include "environment.hpp"
#include "drivers/bme68x/bme68x.hpp"
#include "common/pimoroni_i2c.hpp"

I2C i2c(BOARD::BREAKOUT_GARDEN);
BME68X bme68x(&i2c, BME68X::ALTERNATE_I2C_ADDRESS);

const float MIN_GAS = 100000.0;
const float MAX_GAS = 0.0;
const int ALTITUDE = 0;

int units = CELSIUS;

void initEnvironment() {
  bme68x.init();
}

float adjustToSeaPressure(float pressureHpa, float temperature, float altitude) {
    
    // Adjust pressure based on your altitude.
    // credits to @cubapp https://gist.github.com/cubapp/23dd4e91814a995b8ff06f406679abcf
  
    // Adjusted-to-the-sea barometric pressure
    return pressureHpa + ((pressureHpa * 9.80665 * altitude) / (287 * (273 + temperature + (altitude / 400))));
}

void renderEnvironmentFrame(PicoGraphics& graphics, Pens& pens) {
    const int TEMPERATURE_OFFSET = 9;

    bme68x_data data;
    auto result = bme68x.read_forced(&data, 300, 100);
    (void)result;
    
    graphics.set_pen(pens.BLACK);
    graphics.clear();

    // sprintf(stringBuffer, "%dus, %dus", frameTime, loopTime);
    // Point text_location(0, 0);
    // graphics.set_pen(pens.WHITE);

    // sprintf(stringBuffer, "%.2f, %.2f, %.2f, %.2f, 0x%x",
    //     data.temperature,
    //     data.pressure,
    //     data.humidity,
    //     data.gas_resistance,
    //     data.status);
        
    // graphics.text(stringBuffer, text_location, 240);

    auto correctedTemperature = data.temperature - TEMPERATURE_OFFSET;
    auto dewpoint = data.temperature - ((100 - data.humidity) / 5);
    auto correctedHumidity = 100 - (5 * (correctedTemperature - dewpoint));
    auto correctedTemperatureF = (correctedTemperature * 9/5)+32;

    // auto gas = MAX(MIN(MAX_GAS, data.gas_resistance), MIN_GAS);
    // auto pressureHpa = adjustToSeaPressure(data.pressure / 100, data.temperature, ALTITUDE);

    if(data.status & BME68X_HEAT_STAB_MSK) 
    {        
        char primaryBuffer[16], secondaryBuffer[16];

        if(units == CELSIUS) {
          sprintf(primaryBuffer, "%.0f°C", correctedTemperature);
          sprintf(secondaryBuffer, "%.0f°F", correctedTemperatureF);
        }
        else {
          sprintf(primaryBuffer, "%.0f°F", correctedTemperatureF);
          sprintf(secondaryBuffer, "%.0f°C", correctedTemperature);
        }

        uint16_t primaryTextWidth = graphics.measure_text(primaryBuffer, 8) - 8;
        uint16_t secondaryTextWidth = graphics.measure_text(secondaryBuffer, 3) - 3;
        uint16_t centeredPrimaryTextX = (WIDTH-primaryTextWidth) / 2;
        uint16_t centeredSecondaryTextX = WIDTH-secondaryTextWidth - 12;

        graphics.set_pen(pens.YELLOW);
        graphics.text(primaryBuffer, Point(centeredPrimaryTextX, 90), false, 8);

        graphics.set_pen(pens.WHITE);
        graphics.text(secondaryBuffer, Point(centeredSecondaryTextX, 205), false, 3);

        sprintf(primaryBuffer, "%.0f%%", correctedHumidity);
        graphics.text(primaryBuffer, Point(42, 205), false, 3);

        
        graphics.set_pen(pens.LIGHT_BLUE);
        graphics.circle(Point(25, 219), 8);

        std::vector<Point> poly;
        poly.push_back(Point(19, 215));
        poly.push_back(Point(25, 205));
        poly.push_back(Point(31, 215));
        graphics.polygon(poly);
    }
}