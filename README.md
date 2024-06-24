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
2. [WIZnet WIZPoE-P1](https://docs.wiznet.io/Product/Open-Source-Hardware/PoE/WIZPoE-P1)
3. [Coral Micro Board](https://coral.ai/docs/dev-board-micro/get-started/)
4. [I2C level shifter module](https://www.aliexpress.com/w/wholesale-txs0102-module.html)
5. [0.96" I2C LCD module](https://www.aliexpress.com/item/33008480580.html)
6. A mobile phone with Whatsapp installed

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
   if everything goes smooth, you should see the following output
   [![Build Coral](/doc/image/build-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/build-coral.png)
6. flash firmware by typing command "python3 coralmicro/scripts/flashtool.py --build_dir out --elf_path out/coralmicro-app"  
   if everything goes smooth, you should see the following output
   [![Flash Coral](/doc/image/flash-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/flash-coral.png)
7. Disconnect USB cable between PC and Coral Micro Board, launch a serial terminal (e.g. GTKTerm) with 115200 8N1
8. Connect USB cable between PC and Coral Micro Board, if everything go smooth, you you see the following output
   [![Run Coral](/doc/image/run-coral.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/run-coral.png)
9. ### (IMPORTANT) Disconnect USB cable between PC and Coral Micro Board.


## Build and flash Surf5 firmware
1. Follow the instruction on WIZnet web site to install Surf5 SDK on VS code  
   https://docs.wiznet.io/Product/Open-Source-Hardware/surf5/getting-started/install-vscode-guide
2. Clone this repository by "git clone https://github.com/teamprof/surf5-alarm4deaf"
3. copy the "surf5-alarm4deaf/src-surf5" directory to the installed Surf5 SDK directory
4. change to the directory "<Surf5 SDK>/src-surf5"
5. build the firmware by clicking the "Build" icon on VS code. If everything goes smooth, you should see the following
   [![Build surf5](/doc/image/build-surf5.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/build-surf5.png)
6. Follow the instruction on WIZnet web site to download and install Surf5 ISP tool  
   https://docs.wiznet.io/Product/Open-Source-Hardware/surf5/getting-started/flashing-surf5
7. flash the built Surf5 firmware via the ISP tool. If everything goes smooth, you should see the following
   [![Flash surf5](/doc/image/flash-surf5.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/flash-surf5.png)
8. launch a serial terminal (e.g. TeraTerm) to connect Surf5 at 115200 8N1. Click the "Reset" button on Surf5, if everything goes smooth, you should see the following
   [![Run surf5](/doc/image/run-surf5.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/run-surf5.png)
9. plug-in Ethernet cable to Surf5. If Surf5 got IP address, you should see the following
   [![ip-assigned surf5](/doc/image/ip-surf5.jpg)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/ip-surf5.jpg)
10. ### (IMPORTANT) Disconnect USB cable between PC and Surf5.


## Complete demo
## DO NOT connect USB cable to either Surf 5 Board or Coral Miro Board. Otherwise, boards MAY be permanently damaged! 
1. wiring between WIZnet Surf5 and Coral Micro Board according to schematic design
2. short/close the jumper J5 to connect Surf5 5V to Coral Micro board 5V
3. mount WIZPoE-P1 onto Surf5 
4. connect the Surf5 with an Ethernet cable to a PoE Hub. If everything goes smooth, you should see the following
   [![poe surf5](/doc/image/poe-surf5.png)](https://github.com/teamprof/surf5-alarm4deaf/blob/main/doc/image/poe-surf5.png)


## Software explanation in high level
Communication between Surf5 and Coral occurs via I2C. Surf5 acts as the I2C master, while Coral serves as the slave. After Surf5 initializes the network code and receives an assigned IP address, it polls Coral’s status every second. When Coral is initialized and receives the “CommandGetStatus” command from Surf5, it responds with the status “StatusRunning.” Once Surf5 receives this response, it continues to poll Coral’s inference result by sending the I2C command “CommandGetResult” every 0.5 seconds. Coral’s inference result can be either “ResultAlarmOn” or “ResultAlarmOff,” depending on the alarm sound detection outcome. Additionally, Surf5 sends a WhatsApp text message when there is an update in the alarm detection result.
```
    surf5                                 coral
      |                                     |
      |      CommandGetStatus @ 1Hz         |
      | ----------------------------------> |
      |                                     |
      |          StatusRunning              |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |                                     |
      |      CommandGetResult @ 2Hz         |
      | ----------------------------------> |
      |                                     |
      |   ResultAlarmOn / ResultAlarmOff    |
      | <---------------------------------- |
      |                                     |

```

## Demo
Video demo is available on:
1. [surf5 alarm4deaf video 1](https://www.youtube.com/watch?v=j0oOIK2erMc)  
2. [surf5 alarm4deaf video 2](https://www.youtube.com/watch?v=dCqINISsorI)  

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



