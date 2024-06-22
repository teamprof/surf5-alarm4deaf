## Introduction
A fire alarm sound detector for deaf.
Once Edge ML IC detected alarm, a message will be sent via the Internet connection provided by Surf5.

[![cover-image](/doc/image/cover-image.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/cover-image.png)

This project demonstrates how to leverage Surf5, WIZPoE-P1 and Edge machine learning (ML) module to implement an IoT platform with ML capability, powered by Ethernet cable (without AC adapter).

This project is targeted toward deaf individuals who are living in an apartment. It monitors sounds inside a building. If it detects a fire alarm sound, it sends an alert message to the user’s mobile device, signalling them to escape immediately.

WIZPoE-P1 provides 5V power to both Surf5 and Edge ML module. Surf5(W7500) acts as a main controller which provides Ethernet connectivity for sending an alert message to cloud. Edge ML module monitoring sound inside the apartment, it sends Surf5 an alert message if fire alarm sound is detected.

This platform can be expanded to support various applications by upgrading the ML models while keeping the power and controller circuitry.


[![License: GPL v3](https://img.shields.io/badge/License-GPL_v3-blue.svg)](https://github.com/teamprof/no-os-surf5-alarm4deaf/blob/main/LICENSE)

<a href="https://www.buymeacoffee.com/teamprof" target="_blank"><img src="https://cdn.buymeacoffee.com/buttons/v2/default-yellow.png" alt="Buy Me A Coffee" style="height: 28px !important;width: 108px !important;" ></a>

---
## Hardware
The following components are required for this project:
1. [WIZnet Surf 5 Board](https://docs.wiznet.io/Product/Open-Source-Hardware/surf5)
2. [Coral Micro Board](https://coral.ai/docs/dev-board-micro/get-started/)
3. [I2C level shifter module](https://www.aliexpress.com/w/wholesale-txs0102-module.html)
4. [0.96" I2C LCD module](https://www.aliexpress.com/item/33008480580.html)
5. A mobile phone with Whatsapp installed

---
## Software 
1. [WIZnet Surf5 SDK](https://docs.wiznet.io/Product/Open-Source-Hardware/surf5/getting-started)
2. [Coral Dev Board Micro SDK](https://coral.ai/docs/dev-board-micro/get-started)

---
# Getting Started
## DO NOT connect USB cable to both Surf 5 Board and Coral Miro Board simultaneously. Otherwise, boards MAY be permanently damaged! 
There are two projects, one for WIZnet Surf5 and the other one for Google Coral Micro Board. They are under subdirectories "src-surf" and "src-coral" respectively.

## Build and flash alarm sound detection firmware on Coral Micro Board
1. Install [Coral Dev Board Micro](https://coral.ai/docs/dev-board-micro/get-started)
2. Clone this repository by "git clone https://github.com/teamprof/surf5-alarm4deaf"
3. copy the "surf5-alarm4deaf/src-coral" directory to the installed Coral SDK directory
4. change to the directory "<Coral SDK>/src-coral"
5. build firmware by typing commands "cmake -B out -S ." followed by "make -C out -j4"  
   if everything go smooth, you you see the following output
   [![Build Coral](/doc/image/build-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/build-coral.png)
6. flash firmware by typing command "python3 coralmicro/scripts/flashtool.py --build_dir out --elf_path out/coralmicro-app"  
   if everything go smooth, you you see the following output
   [![Flash Coral](/doc/image/flash-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/flash-coral.png)
7. Disconnect USB cable between PC and Coral Micro Board, launch a serial terminal (e.g. GTKTerm) with 115200 8N1
8. Connect USB cable between PC and Coral Micro Board, if everything go smooth, you you see the following output
   [![Run Coral](/doc/image/run-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/run-coral.png)
9. (IMPORTANT) Disconnect USB cable between PC and Coral Micro Board.





---
### Troubleshooting
If you get compilation errors, more often than not, you may need to install a newer version of the coralmicro.

Sometimes, the project will only work if you update the board core to the latest version because I am using newly added functions.

---
### Issues
Submit issues to: [no-os-surf5-alarm4deaf issues](https://github.com/teamprof/surf5-alarm4deaf/issues) 

---
### TO DO
1. Search for bug and improvement.
---

### Contributions and Thanks
Many thanks to the following authors who have developed great software and libraries.
1. [WIZnet](https://docs.wiznet.io/Product/Open-Source-Hardware/surf5/getting-started)
2. [Coral team](https://coral.ai/about-coral/)

#### Many thanks for everyone for bug reporting, new feature suggesting, testing and contributing to the development of this project.
---

### Contributing
If you want to contribute to this project:
- Report bugs and errors
- Ask for enhancements
- Create issues and pull requests
- Tell other people about this library
---

### License
- The project is licensed under GNU GENERAL PUBLIC LICENSE Version 3
---

### Copyright
- Copyright 2024 teamprof.net@gmail.com. All rights reserved.



