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
3. [An I2C level shifter module](https://www.aliexpress.com/w/wholesale-txs0102-module.html)
4. A mobile phone with Whatsapp installed

---
## Software 
1. Clone this repository by "git clone https://github.com/teamprof/surf5-alarm4deaf"
2. Install [Coral Dev Board Micro](https://coral.ai/docs/dev-board-micro/get-started)
---











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



