## Introduction
A fire alarm sound detector for deaf.
Once Edge ML IC detected alarm, a message will be sent via the Internet connection provided by Surf5.

[![cover-image](/doc/image/cover-image.png)](https://github.com/teamprof/no-os-surf5-alarm4deaf/blob/main/doc/image/cover-image.png)

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
3. [An I2C level shifter module](https://www.aliexpress.com/w/wholesale-txs0102-module.html)
4. [Motion Sensor Wall Lamp](https://www.aliexpress.com/item/1005006418352522.html)
4. [TCL LED Lamp](https://detail.tmall.com/item.htm?_u=t2dmg8j26111&id=720090273230)
5. A mobile phone with Whatsapp installed

---
## Software 
1. Clone this repository by "git clone https://github.com/teamprof/github-no-os-surf5-alarm4deaf"
2. Install [Coral Dev Board Micro](https://coral.ai/docs/dev-board-micro/get-started)
---


## DO NOT connect USB cable to both Surf 5 Board and Coral Miro Board simultaneously. Otherwise, boards MAY be permanently damaged! 

## Flash firmware on Coral Micro Board
1. IMPORTANT: Disconnect all USB cables between Coral Micro Board and PC, as well as Surf5 Board and PC
2. Follow the instruction on "https://coral.ai/docs/dev-board-micro/get-started/#5-try-the-face-detection-example" to install tools for flashing Coral Micro Board
3. Hold the "User Button", connect a USB cable between Coral Micro Board and PC
4. Launch a terminal and type the following command to flash the firmware for the Coral Micro Board  
      python3 coralmicro/scripts/flashtool.py --build_dir coral-firmware --elf_path coral-firmware/coralmicro-app
5. Disconnect the USB cable between Coral Micro Board and PC. (NO need to connect after flashing)

note: please refer to !!! about the firmware code for the Coral Micro Board

## Build and Flash firmware on Surf 5 Board 
1. Clone this repository by "git clone https://github.com/teamprof/github-no-os-surf5-alarm4deaf"
2. Follow the instruction on "https://docs.wiznet.io/Product/Open-Source-Hardware/surf5" to build and flash the Surf 5 firmware.

## Run the demo
**DO NOT connect USB cable to both Surf 5 Board and Coral Miro Board simultaneously. Otherwise, boards MAY be permanently damaged!**
1. Launch a Serial terminal, set baud rate to 115200 and connect it to the USB port of the Surf 5 Board  

2. If everything goes smooth, you should see the followings on the serial terminal:
[![serial terminal screen](/doc/image/serial-terminal.png)](https://github.com/teamprof/github-no-os-surf5-alarm4deaf/blob/main/doc/image/serial-terminal.png)
3. Play a "fire alarm sound" with your mobile / PC. The serial terminal should show "Fire alarm detected". User should receive a Whatsapp message thereafter.


---
## Hardware connection between Surf 5 and Coral Dev Board Micro 
```
+-------------------------+   +--------------------------+     
|         Surf 5          |   | Coral Dev Board Micro    |     
+------+------------------+   +-------+------------------+     
| GPIO | description      |   | GPIO  | description      |     
+------+------------------+   +-------+------------------+     
| PC04 | mcuSCL1 (master) |   | kSCL1 | I2C1 SCL (slave) |     
| PC05 | mcuSDA1 (master) |   | kSDA1 | I2C1 SDA (slave) |     
| VBUS | VBUS             |   | VSYS  | VSYS             |     
| GND  | GND              |   | GND   | GND              |     
+------+------------------+   +-------+------------------+     
```

## I2C communication format
```
  command: GetStatus
                   +----------------+----------------+
  I2cCommand       |     byte 0     |     byte 1     |
                   |     bEvent     |     bParam     |
                   +----------------+----------------+
  master -> slave  |    GetStatus   |       0        |
                   +----------------+----------------+

                   +----------------+----------------+----------------+
  I2cResponse      |     byte 0     |     byte 1     |    byte 2-3    |
                   |     bEvent     |     bParam     |     wParam     |
                   +----------------+----------------+----------------+
  master <- slave  |   NpuReady /   |        0       |        0       |
                   |   NpuRunning   |                |                |
                   +----------------+----------------+----------------+


  command: GetResult
                   +----------------+----------------+
  I2cCommand       |     byte 0     |     byte 1     |
                   |     bEvent     |     bParam     |
                   +----------------+----------------+
  master -> slave  |    GetResult   |       0        |
                   +----------------+----------------+

                   +----------------+----------------+----------------+
  I2cResponse      |     byte 0     |     byte 1     |    byte 2-3    |
                   |     bEvent     |     bParam     |     wParam     |
                   +----------------+----------------+----------------+
  master <- slave  |        /       |        0       |        0       |
                   |                |                |                |
                   +----------------+----------------+----------------+


  command: Start
                   +----------------+----------------+
  I2cCommand       |     byte 0     |     byte 1     |
                   |     bEvent     |     bParam     |
                   +----------------+----------------+
  master -> slave  |      Start     |       0        |
                   +----------------+----------------+

                   +----------------+----------------+----------------+
  I2cResponse      |     byte 0     |     byte 1     |    byte 2-3    |
                   |     bEvent     |     bParam     |     wParam     |
                   +----------------+----------------+----------------+
  master <- slave  |   NpuRunning   |        0       |        0       |
                   +----------------+----------------+----------------+


  command: Stop
                   +----------------+----------------+
  I2cCommand       |     byte 0     |     byte 1     |
                   |     bEvent     |     bParam     |
                   +----------------+----------------+
  master -> slave  |      Stop      |       0        |
                   +----------------+----------------+
                   +----------------+----------------+----------------+
  I2cResponse      |     byte 0     |     byte 1     |    byte 2-3    |
                   |     bEvent     |     bParam     |     wParam     |
                   +----------------+----------------+----------------+
  master <- slave  |     NpuReady   |        0       |        0       |
                   +----------------+----------------+----------------+


```

## Flow of communication between Surf5 Board and Coral Dev Board Micro
```
    surf5                                 coral
      |                                     |
      |   icc::Event::GetStatus             |
      | ----------------------------------> |
      |                                     |
      |   icc::Response::NpuReady           |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |   icc::Event::Start                 |
      | ----------------------------------> |
      |                                     |
      |   icc::Response::NpuRunning         |
      | <---------------------------------- |
      |                                     |
      |                                     |
      |                                     |
      |   (loop poll inference result)      |
      |                                     |
      |   icc::Event::GetResult             |
      | ----------------------------------> |
      |                                     |
      |   icc::Response:: ???               |
      | <---------------------------------- |
      |                                     |
      |                                     |

    note:
    1. surf5 polls coral every 0.5s
```

## LED on Surf5 Board
- User LED: on when an IP is assigned, off otherwise.

## LED on Coral Micro Board
- Edge TPU LED: on when Edge TPU is inferencing, off otherwise
- User LED: on when fire alarm sound is detected, off otherwise
- Status LED: flash when receiving I2C data

[![coral-top-view](/doc/image/coral-top-view.png)](https://github.com/teamprof/no-os-surf5-alarm4deaf/blob/main/doc/image/coral-top-view.png)
---

## Code explanation
// !!!
### I2C address
The I2C slave address of ESP32 is defined in the macro "I2C_DEV_ADDR" in the file "./src/app/thread/ThreadReporter.cpp"
Here's an example of setting the I2C slave address to "0x55".
```
#define I2C_DEV_ADDR ((uint8_t)0x55)
```

### ThreadReporter
The I2C communication code is implemented in the file "./src/app/thread/ThreadReporter.cpp". "ThreadReporter" is responsible for the following tasks:
1. Pool ESP32 on I2C bus regularly, to check Bluetooth (earphone) connection status on ESP32
2. listens event from "ThreadInference", sends commands to ESP32 via I2C

### ThreadInference
"ThreadInference" (./src/app/thread/ThreadInference.cpp) is responsible for the following tasks:
1. initialize tesnsorflow machine learning code 
2. perform objects detection regularly
3. computes swimmer location and sends event to "ThreadInference"

### QueueMainM7
"QueueMainM7" (./src/app/thread/QueueMainM7.cpp) is responsible for the following tasks:
1. Turn "User LED" on when detected lane
2. Turn "Status LED" on when Bluetooth speaker is connected

### Please refer to source code for details

---

## Demo
// !!!
Video demo is available on [no-os-surf5-alarm4deaf video](https://www.youtube.com/watch?v=D5fEQ51aty8)  


---
### Debug
Enable or disable log be defining/undefining macro on "src/AppLog.h"

Debug is disabled by "#undef DEBUG_LOG_LEVEL"
Enable trace debug by "#define DEBUG_LOG_LEVEL Debug"

Example of AppLog.h
```
// enable debug log by defining the following macro
#define DEBUG_LOG_LEVEL Debug
// disable debug log by comment out the macro DEBUG_LOG_LEVEL 
// #undef DEBUG_LOG_LEVEL
```
---
### Troubleshooting
If you get compilation errors, more often than not, you may need to install a newer version of the coralmicro.

Sometimes, the project will only work if you update the board core to the latest version because I am using newly added functions.

---
### Issues
Submit issues to: [no-os-surf5-alarm4deaf issues](https://github.com/teamprof/no-os-alarm4deaf/issues) 

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



