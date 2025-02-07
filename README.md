# FireSport Stopwatch Wireless

## Description
FireSport Stopwatch Wireless is a precision timing solution designed specifically for FireSport competitions, where participants run 100 meters and take down two targets with spray of water. This project aims to establish a wireless connection between a base unit (which starts, counts, and displays the time) and two endpoints (Left and Right targets) that stop the timer when a magnetic switch is pressed. The visual signalization is handled via an 8x8 LED matrix using the MAX7219 driver. 

The communication between the units is facilitated by the nRF24L01+PA+LNA modules, and the system is powered by Arduino Nano microcontrollers. The time is displayed using 7-segment displays using the MAX7219 driver, with smaller ones for training and a DIY version for competitions.

## Features
- Timing with precision up to 1-10 ms (to be measured)
- Timing range: 0:00.00s to 9:59.99 minutes
- Wireless connection range of approximately 100 meters (to be measured)
- Battery-powered (2S 18650, battery life to be measured)
- Arduino Nano as the main controller
- nRF24L01 for wireless communication
- MAX7219 for driving 7-segment displays and 8x8 LED matrix
- Magnetic switches for stopping the timer
- Start timing via button press (automatic start via sound may be added)

## Installation
To get started, you can find the Arduino code in the [GitHub repository](https://github.com/yourusername/FireSport_Stopwatch-Wireless). The code is well-documented for easy understanding and modification.

Additionally, an article detailing the PCB design and 3D printed parts will be added soon. For ordering and assembly inquiries, please contact me via email.

## Usage
This stopwatch can be used for timing fire attack drills and low-level competitions. Please note that it has not yet obtained any precision certification.

## Contributing
Contributions are welcome! You can review the code and suggest changes to optimize it or propose additional features. Feel free to open issues or pull requests.

## License
This project is licensed under the [GPL-3.0 License](https://opensource.org/licenses/GPL-3.0). The code is written in the Arduino IDE, and as per Arduino's guidelines, it is intended to be free and cannot be monetized.

## Contact
For questions, feedback, or collaboration inquiries, please reach out to me at [your-email@example.com].

## Additional Sections
### Testing
A testing section will be added to report on:
- Distance tests of the nRF24L01 module
- Battery life tests to determine how long the system can operate on a single battery pack

Stay tuned for updates!
