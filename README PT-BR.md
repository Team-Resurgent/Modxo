![Modchip + Axolotl = Modxo](images/logo.png) <img src="images/Shalx-TR.png" height="176">

[![License: GPL v3](https://img.shields.io/badge/License-GPLv3-blue.svg)](https://github.com/Team-Resurgent/Repackinator/blob/main/LICENSE.md)
[![.NET](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml/badge.svg)](https://github.com/Team-Resurgent/Modxo/actions/workflows/BundleModxo.yml)
[![Discord](https://img.shields.io/badge/chat-on%20discord-7289da.svg?logo=discord)](https://discord.gg/VcdSfajQGK)
<!--Traduzido por: nascimentolh-->

# Versões traduzidas

> [!NOTE]
> Algumas traduções podem estar desatualizadas e podem demorar um tempo para serem atualizadas em relação à versão em inglês.

 - [English](https://github.com/Team-Resurgent/Modxo)
 - [Español](https://github.com/Team-Resurgent/Modxo/blob/main/README%20ES-MX.md)
 - [Portuguese](https://github.com/Team-Resurgent/Modxo/blob/main/README%20PT-BR.md)
 - [Nederlands](https://github.com/Team-Resurgent/Modxo/blob/main/README%20NL.md)
 - Mais traduções em breve

# Modxo

Modxo (pronunciado "Modsho") é um firmware RP2040 que converte um Raspberry Pi Pico (ou dispositivo similar baseado em RP2040) em um dispositivo periférico LPC compatível com Xbox Original.

O Modxo pode ser usado para carregar uma imagem de BIOS do Xbox através da porta LPC, bem como para fazer a interface de software compatível com Xbox com dispositivos periféricos como displays HD44780 ou LEDs RGB endereçáveis.

O Modxo *não* é um modchip. Enquanto os modchips legados dependem de hardware amplamente obsoleto como chips de armazenamento flash LPC ou ICs de lógica programável caros, o Modxo é a primeira implementação totalmente definida por software de um dispositivo periférico LPC. É software de código aberto, principalmente escrito em C, desenvolvido usando o SDK oficial do Raspberry Pi Pico e projetado para rodar em hardware baseado em RP2040.

Nenhum hardware especializado ou ferramentas complicadas são necessárias para carregar o Modxo em um dispositivo compatível -- na maioria dos casos apenas um cabo USB é tudo que é necessário. E a instalação funciona muito como dispositivos legados -- tudo que é necessário para instalação é um dispositivo compatível baseado em RP2040, alguns resistores, fios e equipamentos básicos de solda. PCBs customizadas existem para simplificar ainda mais o processo de instalação.

## Como Instalar
### Requisitos
- Um Xbox (qualquer revisão) com uma Porta LPC funcionando. Xbox's 1.6 precisarão de uma reconstrução LPC.
- Uma placa de desenvolvimento RP2040. Pode haver algumas placas clones que não são compatíveis. As seguintes placas são conhecidas por funcionar com o Modxo:
- - Raspberry Pi Pico Oficial
  - Raspberry Pi Pico 2 Oficial
  - YD-RP2040
  - RP2040 Zero/Tiny
  - XIAO RP2040
- 4 resistores de 100 Ohm (testado com resistores de 1/4 W)
- 1 Diodo 1N4148 (tecnicamente opcional mas altamente recomendado com certas placas de desenvolvimento, veja as instruções de instalação específicas da placa abaixo para mais informações)

### Gravando o firmware

#### Empacotando a BIOS
1. Vá para [https://team-resurgent.github.io/modxo/](https://team-resurgent.github.io/Modxo/)
2. Arraste e solte seu arquivo de BIOS
3. O arquivo UF2 com a imagem da BIOS será baixado

#### Passos para gravação (BIOS única)

Os seguintes passos não são necessários se você pretende usar o PrometheOS, apenas grave o PrometheOS-{variante-da-placa}.uf2 diretamente...

> [!NOTE]
> Os seguintes passos não são necessários se você pretende usar o PrometheOS, apenas grave o PrometheOS-{variante-da-placa}.uf2 diretamente...

1. Conecte o Raspberry Pi Pico a um PC com o botão BOOTSEL (ou botão equivalente em hardware RP2040 compatível) pressionado e uma nova unidade ficará disponível.
2. Copie o Modxo-{variante da placa}.uf2 para a unidade do Raspberry Pi Pico.
3. Reconecte o Raspberry Pi Pico com o botão BOOTSEL pressionado, para que a unidade anterior apareça novamente.
4. Copie seu arquivo UF2 da BIOS para a unidade

#### Passos para atualizar o Modxo do PrometheOS

> [!IMPORTANT]
> Os seguintes passos são necessários apenas se quiser atualizar o PrometheOS com atualizações/correções do Modxo...


1. Assumindo que o PrometheOS-{variante-da-placa}.uf2 já está gravado.
2. Conecte o Raspberry Pi Pico a um PC com o botão BOOTSEL (ou botão equivalente em hardware RP2040 compatível) pressionado e uma nova unidade ficará disponível.
3. Copie o Modxo-{variante da placa}.uf2 para a unidade do Raspberry Pi Pico.
   
### Diagramas de fiação
---
#### Cabeçalho LPC
   ![Diagrama de fiação do cabeçalho LPC](images/lpc_header_wiring.png)

  > [!NOTE]
  > * D0 é necessário para versões 1.0 - 1.5 a menos que esteja aterrado.
  > * Conexões LFrame e LPC 3.3V são necessárias pela versão 1.6 ou quando conectar o Pico à porta USB.
  > * LFrame não é necessário para debug USB.
  > * Reconstrução LPC é necessária para versão 1.6
---
#### Raspberry Pi Pico Oficial

   ![Diagrama de fiação do cabeçalho LPC](images/official_pinout_pico1.png)

   > [!NOTE]
   > Por favor adicione o diodo se conectar o Pico ao USB. Isso evita alimentar o pino LPC 5V através do cabo USB, o que poderia ter consequências não intencionais.
---
#### Raspberry Pi Pico 2 Oficial

   ![Diagrama de fiação do cabeçalho LPC](images/official_pinout_pico2.png)

   > [!NOTE]
   > Por favor adicione o diodo se conectar o Pico ao USB. Isso evita alimentar o pino LPC 5V através do cabo USB, o que poderia ter consequências não intencionais.
---
#### YD-RP2040

   ![Diagrama de fiação do cabeçalho LPC](images/YD_RP2040_pinout.png)

   > [!NOTE]
   > Não esqueça de adicionar solda ao jumper R68 se usar o LED RGB integrado
---
#### RP2040-Zero/Tiny

   ![Diagrama de fiação do cabeçalho LPC](images/RP2040_Zero_pinout.png)

   > [!NOTE]
   > Por favor adicione o diodo se conectar o Pico ao USB. Isso evita alimentar o pino LPC 5V através do cabo USB, o que poderia ter consequências não intencionais.
---
#### XIAO-RP2040

   ![Diagrama de fiação do cabeçalho LPC](images/XIAO-RP2040_pnout.png)

---

## Instruções de Compilação do Firmware

### Windows
1.- Baixe e instale o [Visual Studio Code](https://code.visualstudio.com/download)

2.- Instale a extensão: "Raspberry Pi Pico"

![Extensão Pico](images/extension.png)

3.- Certifique-se de que o SDK 2.0.0 esteja selecionado como abaixo...

![SDK 2.0.0](images/sdk.png)

4.- Vá para a aba Raspberry Pi Pico e clique em "Configure CMake"

5.- Vá para a aba Run and Debug e selecione Build para sua placa

6.- Clique em "Start Debugging" (seta verde)

7.- O arquivo UF2 será gerado na pasta Build


### Docker
#### Configuração
Compile sua imagem docker base com
```
docker build -t modxo-builder .
```

#### Compilação do Firmware
```
docker compose run --rm builder
```

A saída será `out/modxo_[pinout].uf2`

Também há alguns parâmetros extras que podem ser passados para o script de compilação:

- MODXO_PINOUT=`official_pico` | `yd_rp2040` | `rp2040_zero` | `xiao_rp2040` - O padrão é `official_pico`.

- CLEAN=`y`: dispara uma compilação limpa. O padrão é desabilitado.

- BUILD_TYPE=`Release` | `Debug` - O padrão é `Debug`.


_Exemplos:_
```
MODXO_PINOUT=rp2040_zero BUILD_TYPE=Release docker compose run --rm builder
```
```
CLEAN MODXO_PINOUT=yd_rp2040 docker compose run --rm builder
```

#### Empacotando BIOS localmente
Coloque seu arquivo de BIOS nomeado `bios.bin` neste diretório ou coloque quaisquer arquivos de BIOS (independentemente do nome) no diretório bios
```
docker compose run --rm bios2uf2
```

---

## Bugs conhecidos
 * Windbg às vezes trava quando conectado à porta serial SuperIO do Modxo

## Notas
 > [!IMPORTANT]
 > Atualmente, Modxo usa o ID 0xAF. Idealmente, qualquer hardware derivado com mudanças significativas deveria usar um ID diferente. Isso é para que um software como o PrometheOS possa mirar suas características de forma adequada.

## Requisito de Atribuição

     a) **Atribuição:**  
       Se você distribuir ou modificar este trabalho, deve fornecer atribuição 
       apropriada aos autores originais. Isso inclui:
       - Mencionar o nome do projeto original: `Modxo`.
       - Listar os autores originais: `Shalx / Team Resurgent`.
       - Incluir um link para o repositório do projeto original: 
           `https://github.com/Team-Resurgent/Modxo`.
       - Declarar claramente quaisquer modificações feitas.

    b) **Logo e Marca:**  
       Qualquer trabalho derivado ou distribuição deve incluir os logos 
       fornecidos pelos autores originais de acordo com as diretrizes de marca. 
       Os logos devem ser exibidos proeminentemente em qualquer interface ou 
       documentação onde o projeto original for referenciado ou atribuído.

    c) **Diretrizes de Marca:**  
       Você pode encontrar os logos e diretrizes detalhadas de marca no 
       arquivo `BRANDING PT-BR.md` fornecido com este projeto. Os logos não devem 
       ser alterados de qualquer forma que distorça ou deturpe a marca original.