<br />

**Please refer to [getting_started.md](getting_started.md) for examples usage.**

<br />

# W7500x-Surf5

W7500x-Surf5 library is developed based on the W7500x_StdPeriph_Lib, designed for use with the W7500 MCU. 
For detailed information about W7500x_StdPeriph_Lib, please refer to the [W7500x_StdPeriph_Lib README](https://github.com/Wiznet/W7500x_StdPeriph_Lib/blob/master/README.md).

## Supported devices and toolchains

### Supported Surf 5 devices

The W7500x-Surf5 Library supports [Surf 5](https://docs.wiznet.io/Product/Open-Source-Hardware/surf5) devices.

<br />

### Supported toolchains

The Standard Peripherals Library supports the following toolchains:
- MDK-ARM (KEIL 5)
- GNU MCU (Eclipse)
- GNU Arm Embbeded Toolchain(VS Code)

<br />
<br />

## How to use the Library

### Create a project

Create a project in various toolchains. 

If you want to create a project in the Keil 5 environment, use the template project under Project\W7500x_StdPeriph_Templates within the library. 

Alternatively, if you want to create a project in the VS Code environment, use the template project under Project\W7500x_StdPeriph_Examples within the library.

<br />

### Configure w7500x.h

The Library entry point is w7500x.h (under Libraries\CMSIS\Device\WIZnet\W7500\Include), user has to include it in the application main and configure it:


- Select the target system clock source to be used, comment/uncomment the right define:
  ```
  /**
   * @brief In the following line adjust the value of External oscillator clock (8MHz ~ 24MHz)
   used in your application

   Tip: If you want to use the Internal 8MHz RC oscillator clock, uncomment the line below.
   */
  //#define OCLK_VALUE 12000000UL
  ```
- Change the PLL value to be used.
  ```
  /**
   * @brief In the following line adjust the value of PLL
   */
  #define PLL_VALUE 1
  ```
  
<br />
<br />

### Add the system_w7500x.c

Add the system_w7500x.c (under Libraries\CMSIS\Device\WIZnet\W7500\Source) file in your application, this file provide functions to setup the W7500x system.

<br />
<br />

## How to use the Examples

### Copy and Paste

Copy all source files from under Projects\W7500x_StdPeriph_Examples\xxx\xxx folder to the under src folder.

<br />
<br />

## Revision History

### v1.0.0
- First release

<br />

