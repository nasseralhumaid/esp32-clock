# ESP32-Clock

A smart clock which grabs time and weather from the internet.



### How To Use:

1. ##### Use the precombiled binary
   
   - Check that your board is supported under **supported boards**
   
   - Get the binary (.bin) file from **releases**
   
   - Flash it to your board using online tools
2. ##### Build and flash yourself
   - Clone this repository and use PlatformIO to build and upload it to your board.
   
   - You will need to edit platformio.ini if your board is not the ESP32-S3 N16R8

### Supported Boards:

- ESP32-S3 N16R8

- Support for more boards will come soon when the project is more mature



### Planned Features:

- A real name for the project

- A plugin system/a way to select different modules to display on the right side of the screen

- A web ui for configuration on a mobile device or computer

- Potentially integrate with a computer to show things like currently playing song, WPM etc.

- Make better use of ESP32-S3 features like using both cores

- Port to low powered boards like the ESP32-C3 and STM32

- Make the display show more information about what it is doing (connecting to WiFi, fetching weather, display how outdated weather info is, etc.)



### Current Issues:

- Around 30% of the screen is unused.



### AI Usage

- No AI agents were used to make the code

- Only web based AI was used for help with libraries and built in functions.
