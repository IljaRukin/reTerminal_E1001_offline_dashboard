# reTerminal_E1001_offline_dashboard

Offline Dashboard build with a reTerminal E1001 to display weather forecast data. Current and future.
The current data is provided by a internal sensor as well as external sensors over Wi-fi.
Forecasts are pulled from the Internet on request by pressing a button.


# Hardware
## used
- ESP32-S3
- 8MB PSRAM
- 32MB Flash
- power & status LED
- display 800×480px
- wifi WIFI_AP_STA
- temperature & humidity sensor
- buttons
- micro SD card (max. 32GB FAT32)
- expansion port: 8-pin expansion header providing VDD, GND, UART, I2C, and GPIO connections

# other & unused
- usb-c for power & update
- power switch
- bluetooth 5.0
- microphone
- buzzer

# design

## set interrupts
+ set up rtc, wake up every 30min by a interrupt timer
+ set the rigth button (GPIO5) as an interrupt to wake up
- set the middle button (GPIO4) to toggle WIFI AP mode
- set the left button (GPIO3) to toggle wifi STA  mode for 2min, then disable it again

## on power on and wake up
+ turn the power led on
+ read in temperature and humidity, append them with a time stamp on the sd card in a csv file
+ update the display
+ turn the power led off
+ go into deep sleep

## display
+ top 0/5 to 1/5: display the date and time in the left corner (date and time of the last weather forcast update in brackets next to it), display the Battery Voltage and status of WIFI STA and AP as tick boxes in the top right corner
+ middle 1/5 to 3/5: plot the temperature and humidity for the last week as a graph with two y axis scaled to fit the min/max values, display ticks on all axis
+ bottom 3/5 to 5/5: display the forecast for the next 3 days consistng of an icon (sun, cloudy, rain) and temperature in °C and rain amount in mm for morning, afternoon, night next to it

## WIFI AP
- provide a Webserver with a page to update SSID and PW of Hotspot by providing a hardcoded password
- receive and store data including a timestamp, temperature, humidity, pressure, Voltage, Brightness

## WIFI STA
- connect to the defined Hotspot and request forecast information
- pull the forecast from https://open-meteo.com/, store it for today and the next 2 days on the sd card as json
