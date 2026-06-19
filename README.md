# reTerminal_E1001_offline_dashboard

Offline Dashboard built with a reTerminal E1001 to display weather forecast data — current and future.
The current data is provided by an internal sensor as well as external sensors over Wi-Fi.
Forecasts are pulled from the Internet on request by pressing a button.


# Hardware
## used
- ESP32-S3
- 8MB PSRAM
- 32MB Flash
- power & status LED
- display 800×480px (UC8179, 4-level grayscale ePaper)
- wifi WIFI_AP_STA
- temperature & humidity sensor (SHT4x on I2C)
- buttons (KEY0/left, KEY1/middle, KEY2/right)
- micro SD card (max. 32GB FAT32)
- expansion port: 8-pin expansion header providing VDD, GND, UART, I2C, and GPIO connections

## other & unused
- usb-c for power & update
- power switch
- bluetooth 5.0
- microphone
- buzzer


# Design

## set interrupts
+ set up RTC, wake up every 1 min by an alarm interrupt
+ set the right button (GPIO5) as an interrupt to wake up
- set the middle button (GPIO4) to toggle WIFI AP mode
- set the left button (GPIO3) to toggle wifi STA mode for 2 min, then disable it again

## on power on and wake up
+ turn the power LED on
+ read in temperature and humidity, append them with a time stamp on the SD card in a CSV file
+ update the display
+ turn the power LED off
+ go into deep sleep

## display
+ top: display the date and time in the left corner (date and time of the last weather forecast update in brackets next to it), display the Battery Voltage and status of WIFI STA and AP as tick boxes in the top right corner
+ middle: display the forecast for the next 3 days consisting of an icon (sun, cloudy, rain) and temperature in °C and rain amount in mm for morning, afternoon, night next to it
+ bottom: plot the temperature and humidity for the last week as a graph with two y axes scaled to fit the min/max values, display ticks on all axes

## WIFI AP
- provide a Webserver with a page to update SSID and PW of Hotspot by providing a hardcoded password
- receive and store data including a timestamp, temperature, humidity, pressure, voltage, brightness

## WIFI STA
- connect to the defined Hotspot and request forecast information
- pull the forecast from https://open-meteo.com/, store it for today and the next 2 days on the SD card as JSON


# Implementation

The firmware lives in the `reTerminal/` PlatformIO project. It is self-contained
— the driver code in `ressources/` was used as a reference only and re-implemented
inside the project (no source-level linking back to the example sketches).
