![Modchip + Axolotl = Modxo](images/logo.png) <img src="images/Shalx-TR.png" height="176">
#
Modxo (pronnounced as "Modsho") is a Xbox LPC Port firmware that converts the Raspberry Pi Pico
into an Original Xbox Modchip that allows running a bios.

# How to Install
### 1. Requirements
- Working LPC Port
- Original Raspberry Pi Pico or RP2040 Zero (There are some clone boards that are not compatible)
- 4 100 Ohm resistors (tested with 1/4 W resistors)

### 2. Wiring diagrams

#### LPC Header
![LPC Header wiring diagram](images/lpc_header_wiring.png)

* Note: D0 is only needed by versions different to 1.6
* Note: LFrame and LPC 3.3 connections are required by version 1.6 or when connecting the Pico to USB port.

#### Official pico

![LPC Header wiring diagram](images/official_pinout.png)

#### YD2040

![LPC Header wiring diagram](images/YDRP2040_pinout.png)


### 3. Flashing firmware

#### Packing Bios
1. Go to https://shalxmva.github.io/modxo/
2. Drag and Drop your bios file
3. UF2 File with bios image will be downloaded

#### Flashing steps
1. Connect Raspberry Pi Pico with BOOTSEL button pressed to a PC and one new drive will appear.
2. Copy Modxo.uf2 into the Raspberry Pi Pico Drive.
3. Reconnect Raspberry Pi Pico with BOOTSEL button pressed, so the previous drive will showup again.
4. Copy your bios UF2 file into the drive

# Firmware Build Instructions

#### Windows
1.- Download and Install [Visual Studio Code](https://code.visualstudio.com/download)
2.- Install extension: "Raspberry Pi Pico"
3.- After SDK is installed, git submodules must be updated from command line by running:
```
cd %HOMEPATH%\.pico-sdk\sdk\1.5.1
git submodule update --init --recursive
```
4.- Go to Raspberry Pi Pico Tab and click "Configure CMake"

5.- Go to Run and Debug Tab and select Build for your board

6.- Click "Start Debugging" (Green arrow)

7.- UF2 File will be generated on Build folder


#### Linux
  -Todo

#### Mac
  -Todo

# Known bugs
 * Windbg get stuck sometimes when connected to Modxo SuperIO's serial port
