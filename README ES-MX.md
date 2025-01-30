![Modchip + Axolotl = Modxo](images/logo.png) <img src="images/Shalx-TR.png" height="176">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/Team-Resurgent/Repackinator/blob/main/LICENSE.md)
[![.NET](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml/badge.svg)](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml)
[![Discord](https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord)](https://discord.gg/VcdSfajQGK)

# Modxo

Modxo (pronunciado "Modsho") es un firmware RP2040 que convierte una Raspberry Pi Pico (o cualquier dispositivo similar basado en  RP2040) en un dispositivo LPC compatible con la Xbox original.


Modxo puede ser usado para cargar una imagen de BIOS de XBOX Original desde el puerto LPC, asi como para interconectar software compatible con la XBOX con dispositivos periféricos como pantallas HD44480 o LEDs RGB direccionables. 

Modxo *no es un modchip*. Mientras que los modchips tradicionales dependen de hardware obsoleto, como chips de almacenamiento flash LPC o costosos circuitos integrados programables, Modxo es la primer implementación de un dispotivo perférico definido completamente por software de un dispositivo periférico LPC. Es un software de código abierto, mayormente escrito en C, desarrollado usando el SDK oficial de Raspberry Pi Pico y diseñado para correr en hardware basado en el procesador RP2040.

No necesita ningún hardware especializado ni herramientas complicadas para cargar Modxo en un dispositivo compatible -- en la mayoría de los casos, basta con un cable USB. Y la instalación funciona de forma muy similar a los dispositivos antiguos -- todo lo que necesita para la instalación es un dispositivo compatible con el procesador RP2040, algunas resistencias, cable y equipo básico de soldadura. Existen PCBs personlizados que simplifican aún más el proceso de instalación.

## Cómo instalar
### Requisitos
- Una Xbox (cualquier versión) con un puerto LPC funcional. Las Xbox 1.6 necesitarán una reconstrucción del puerto LPC
- Una placa de desarrolo RP2040. Es posible que algunas placas clon no sean compatibles. Se sabe que las placas compatibles que funcionan con Modxo son las siguientes:
- - Raspberry Pi Pico (Oficial)
  - Raspberry Pi Pico W (Oficial)
  - Raspberry Pi Pico 2 (Oficial)
  - YD-RP2040
  - RP2040 Zero/Tiny
  - XIAO RP2040
- 4 resistencias de 100 Ohm (probadas con resistencias de 1/4  de Watt)
- 1 Diodo 1N4148 (tecnicamente opcional pero altamente recomendado con ciertas placas de desarrolo, consulte las instrucciones especificas de instalación de cada placa, mencionadas a continuación para obtener más información)

### Actualización del firmware

#### Empaquetando BIOS
1. Ve a [https://team-resurgent.github.io/modxo/](https://team-resurgent.github.io/Modxo/)
2. Arrastre y suelte su archivo de BIOS
3. Una vez subido el archivo de BIOS, se creará un archivo UF2 con la imagen del BIOS que debe descargar

#### Pasos para Flashear (BIOS simple)

Los siguientes pasos no son requeridos sí pretende utilizar PrometheOS, sólo actualice PrometheOS-{variante de placa}.uf2 directamente...

1. Conecte la placa Raspberry Pi Pico al PC con el botón BOOTSEL (ó cualquier botón equivalente) presionado, Windows detectará una nueva unidad de almacenamiento y la abrirá.
2. Copie Modxo-{variante de placa}.uf2 en la unidad de la Raspberry Pi Pico. Una vez hecho esto, la Raspberry se desconectará automáticamente.
3. Reconecte la Raspberry Pi Pico con el botón BOOTSEL presionado, de modo que la unidad de almacenamiento de la Raspberry Pi Pico aparazca nuevamente.
4. Copie el archivo UF2 de su BIOS en la unidad de la Raspberry Pi Pico.

#### Pasos para actualizar PrometheOS en Modxo

Los siguientes pasos solo son necesarios sí desea actualizar PrometheOS con actualizaciones/correciones de errores de Modxo...

1. Suponiendo que PrometheOS-{variante de placa}.uf2 ya está instalado.
2. Conecte la Raspberry Pi Pico a la PC con el botón BOOTSEL (o cualquier botón equivalente) presionado, y una nueva unidad de almacenamiento aparecerá.
3. Copie Modxo-{variante de placa}.uf2 en la unidad de la Raspberry Pi Pico.
   
### Wiring diagrams
---
#### LPC Header
   ![LPC Header wiring diagram](images/lpc_header_wiring.png)

  Notes:
  * D0 is required for versions 1.0 - 1.5 unless it is grounded.
  * LFrame and LPC 3.3V connections are required by version 1.6 or when connecting the Pico to USB port.
  * LFrame is not required for USB debug.
  * LPC Rebuild is required for version 1.6
---
#### Official Raspberry Pi Pico

   ![LPC Header wiring diagram](images/official_pinout_pico1.png)

   * Note: Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
---
#### Official Raspberry Pi Pico 2

   ![LPC Header wiring diagram](images/official_pinout_pico2.png)

   * Note: Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
---
#### YD-RP2040

   ![LPC Header wiring diagram](images/YD_RP2040_pinout.png)

   * Note: Dont forget to add solder to jumper R68 if using the onboard RGB Led
---
#### RP2040-Zero/Tiny

   ![LPC Header wiring diagram](images/RP2040_Zero_pinout.png)

   * Note:  Please add the diode if connecting the Pico to USB. This avoid powering the LPC 5V Pin from the USB cable which could have unintended consequences.
---
#### XIAO-RP2040

   ![LPC Header wiring diagram](images/XIAO-RP2040_pnout.png)

---

## Firmware Build Instructions

### Windows
1.- Download and Install [Visual Studio Code](https://code.visualstudio.com/download)

2.- Install extension: "Raspberry Pi Pico"

![Pico Extension](images/extension.png)

3.- Ensure SDK 2.0.0 selected as below...

![SDK 2.0.0](images/sdk.png)

4.- Go to Raspberry Pi Pico Tab and click "Configure CMake"

5.- Go to Run and Debug Tab and select Build for your board

6.- Click "Start Debugging" (Green arrow)

7.- UF2 File will be generated on Build folder


### Docker
#### Setup
Build your base docker image with
```
docker build -t modxo-builder .
```

#### Firmware Build
```
docker compose run --rm builder
```

Output will be `out/modxo_[pinout].uf2`

There are also some extra parameters that can be passed to the build script:

- MODXO_PINOUT=`official_pico` | `yd_rp2040` | `rp2040_zero` | `xiao_rp2040` - Default is `official_pico`.

- CLEAN=`y`: triggers a clean build. Default is disabled.

- BUILD_TYPE=`Release` | `Debug` - Default is `Debug`.


_Examples:_
```
MODXO_PINOUT=rp2040_zero BUILD_TYPE=Release docker compose run --rm builder
```
```
CLEAN MODXO_PINOUT=yd_rp2040 docker compose run --rm builder
```

#### Packing Bios locally
Place your bios file named `bios.bin` in this directory or place any bios files (regardless of their name) in the bios directory
```
docker compose run --rm bios2uf2
```

---

## Known bugs
 * Windbg get stuck sometimes when connected to Modxo SuperIO's serial port

## Notes
 * Currently, Modxo uses the ID 0xAF. Any derivative hardware with significant changes should ideally use a different ID. This is so that software like PrometheOS can target features appropriately.

## Attribution Requirement

     a) **Attribution:**  
       If you distribute or modify this work, you must provide proper 
       attribution to the original authors. This includes:
       - Mentioning the original project name: `Modxo`.
       - Listing the original authors: `Shalx / Team Resurgent`.
       - Including a link to the original project repository: 
           `https://github.com/Team-Resurgent/Modxo`.
       - Clearly stating any modifications made.

    b) **Logo and Branding:**  
       Any derivative work or distribution must include the logos provided by
       the original authors in accordance with the branding guidelines. The 
       logos must be displayed prominently in any interface or documentation 
       where the original project is referenced or attributed.

    c) **Branding Guidelines:**  
       You can find the logos and detailed branding guidelines in the 
       `BRANDING.md` file provided with this project. The logos must not be 
       altered in any way that would distort or misrepresent the original 
       branding.
