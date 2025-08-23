![Modchip + Axolotl = Modxo](images/logo.png) <img src="images/Shalx-TR.png" height="176">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/Team-Resurgent/Repackinator/blob/main/LICENSE.md)
[![.NET](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml/badge.svg)](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml)
[![Discord](https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord)](https://discord.gg/VcdSfajQGK)
<!--Traducción realizada por: Emmanuelito18-->

# Versiones traducidas

*Nota: algunas traducciones pueden estár desactualizadas, y puede tardar un poco en actualizarse en comparación con la versión en inglés.

 - [English](https://github.com/Team-Resurgent/Modxo)
 - [Español](https://github.com/Team-Resurgent/Modxo/blob/main/README%20ES-MX.md)
 - [Portuguese](https://github.com/Team-Resurgent/Modxo/blob/main/README%20PT-BR.md)
 - Más traducciones en el futuro

# Modxo

Modxo (pronunciado "Modsho") es un firmware para RP2040/RP2350 que convierte una Raspberry Pi Pico (o cualquier dispositivo similar basado en  RP2040/RP2350) en un periférico LPC compatible con la Xbox original.


Modxo puede ser usado para cargar una imagen de BIOS de XBOX Original desde el puerto LPC, asi como para interconectar software de XBOX compatible con dispositivos periféricos como pantallas HD44480 o LEDs RGB direccionables. 

Modxo *no es un modchip*. Mientras que los modchips tradicionales dependen de hardware obsoleto, como chips de memoria flash LPC o costosos circuitos integrados programables, Modxo es la primer implementación de un dispotivo perférico completamente por software de un dispositivo periférico LPC. Es software de código abierto, mayormente escrito en C, desarrollado usando el SDK oficial de Raspberry Pi Pico y diseñado para correr en hardware basado en el procesador RP2040/RP2350.

No necesita ningún hardware especializado ni herramientas complicadas para cargar Modxo en un dispositivo compatible -- en la mayoría de los casos, basta con un cable USB. Y la instalación funciona de forma muy similar a los dispositivos antiguos -- todo lo que necesita para la instalación es un dispositivo compatible con el procesador RP2040/RP2350, algunas resistencias, cable y equipo básico de soldadura. Existen PCBs personlizados que simplifican aún más el proceso de instalación.

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
   
### Diagramas de cableado
---
#### Encabezado LPC
   ![Diagrama de cableado del LPC](images/lpc_header_wiring.png)

  Notas:
  * D0 es requerido para versiones 1.0 - 1.5 a menos esté conectado a tierra.
  * Las conexiones LFrame y LPC 3.3v solo son necesarios para la versión 1.6 o cuando se conecta la Raspberry Pi Pico al puerto USB.
  * LFrame no es requerido para la depuración USB.
  * Se requiere una reconstrucción del LPC para la versión 1.6
---
#### Raspberry Pi Pico (Oficial) y Raspberry Pi Pico W (Oficial)

   ![Diagrama de cableado del LPC](images/official_pinout_pico1.png)

   * Nota: Agregue el diosdo si conecta la Raspberry Pi Pico al USB. Esto evita que se alimente el el pin LPC de 5v desde el cable USB, lo que podría tener consecuencias no deseadas.
---
#### Raspberry Pi Pico 2 (Oficial)

   ![Diagrama de cableado del LPC](images/official_pinout_pico2.png)

   *Nota: Agregue el diodo si conecta la Raspberry Pi Pico al USB. Esto evita que se alimente el pin LPC de 5v desde el cable USB, lo que podría tener consecuencias no deseadas.
---
#### YD-RP2040

   ![Diagrama de cableado del LPC](images/YD_RP2040_pinout.png)

   * Nota: No olvide puentear R68 si usa LEDS RGB integrados en la placa
---
#### RP2040-Zero/Tiny

   ![LPC Header wiring diagram](images/RP2040_Zero_pinout.png)

   * Nota:  Por favor agrege el diodo si conecta la Raspberry Pi Pico al USB. Esto evita que se alimente el pin LPC de 5v desde el cable USB, lo que podría tener consecuencias no deseadas.
---
#### XIAO-RP2040

   ![LPC Header wiring diagram](images/XIAO-RP2040_pnout.png)

---

## Instrucciones de compilación del firmware

### Windows
1.- Descargue e instale  [Visual Studio Code](https://code.visualstudio.com/download)

2.- Instale la extensión: "Raspberry Pi Pico"

![Pico Extension](images/extension.png)

3.- Asegurese de de el SDK 2.0.0 esté seleccionado...

![SDK 2.0.0](images/sdk.png)

4.- Ir a la pestanaña Raspberry Pi Pico y haga click en "Configure CMake"

5.- Ir a la pestaña ejecutar y depurar y seleccione Compilar para su placa

6.- Haga click en "Start Debugging" (flecha verde)

7.- El archivo UF2 se generará en la carpeta Build


### Docker
#### Configuración
Construya su imagen base de docker con
```
docker build -t modxo-builder .
```

#### Compilación de Firmware
```
docker compose run --rm builder
```

La salidad será `out/modxo_[pinout].uf2`

También hay algunos parámetros extra que se pueden pasar al script de compilación:

- MODXO_PINOUT=`official_pico` | `yd_rp2040` | `rp2040_zero` | `xiao_rp2040` - El valor predeterminado es `official_pico`.

- CLEAN=`y`: inicia una compiliación limpia. El valor predeterminado es desactivado.

- BUILD_TYPE=`Release` | `Debug` - El valor predeterminado es `Debug`.


_Ejemplos:_
```
MODXO_PINOUT=rp2040_zero BUILD_TYPE=Release docker compose run --rm builder
```
```
CLEAN MODXO_PINOUT=yd_rp2040 docker compose run --rm builder
```

#### Empaquetando Bios localmente
Coloque su archivo de bios nombrada `bios.bin` en este directorio o coloque cualquier archivo de bios (sin importar su nombre) en el directorio de bios
```
docker compose run --rm bios2uf2
```

---

## Errores conocidos
 * Windbg aveces se bloquea cuando se conecte al puerto serial de Modxo SuperIO's

## Notas
 * Actualmete, Modxo usa el ID 0xAF. Idealmente cualquier hardware derivado con cambios significativos debería usar un ID diferente. Esto es para que un software como PrometheOS puedan apuntar a sus caracterisiticas de forma adecuada.

## Requerimientos para atribuciones

     a) **Atribuciones:**  
       Sí distribuye o modifica este trabajo, debes proporcionar la
       a atribución adecuada a los autores originales. Esto incluye:
       - Mencionar el nombre original del proyecto: `Modxo`.
       - Listar los autores originales: `Shalx / Team Resurgent`.
       - Incluir el enlace (link) al repositorio original del proyecto:
            `https://github.com/Team-Resurgent/Modxo`.
       - Declarar claramente cualquier modificación realizada.

    b) **Logo y marca:**  
       Cualquier derivado del trabajo o distribución debe incluir los logos proporcionado por
       los autores originales de acuerdo con las pautas de marca. Los 
       logos deben ser mostrados de manera destacada en cualquier interfaz o documentación
       donde se haga referenia o atribuya al proyecto original.

    c) **Lineamientos de marca**  
       Puede encontrar los logos y los Lineamientos de marca detallados en el 
       archivo `BRANDING ES-MX.md` proporcoinado con este proyecto. Los logos no deben ser
       aleterado de ninguna forma que distorciono o tergiverse la marca original.
