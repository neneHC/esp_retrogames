# ESP32 NES Retro Arcade 🎮

Emulador de NES (Nintendo Entertainment System / Famicom) multijogos de alta performance para a placa de desenvolvimento **IdeaSpark ESP32** com display integrado ST7789 IPS LCD (240x135) e suporte nativo a controles Bluetooth via **Bluepad32** (testado e otimizado para o **Ace Gamer Pocket**).

---

## 🕹️ Funcionalidades

- **Menu Gráfico Interativo:** Interface nativa para seleção de jogos com indicador em tempo real de status da conexão Bluetooth (`[BUSCA]` / `[BT OK]`).
- **6 Jogos Clássicos Embarcados na Flash:**
  1. 💣 **Bomberman** (Hudson Soft, 1985)
  2. 🦍 **Donkey Kong** (Nintendo, 1983)
  3. 🥋 **Kung Fu** (Irem / Nintendo, 1985)
  4. 🍒 **Pac-Man** (Namco, 1984)
  5. 🍄 **Super Mario Bros** (Nintendo, 1985)
  6. 🛸 **Fusiontroid** (Metroid Engine Demo)
- **Suporte a Controle Bluetooth (Bluepad32):**
  - Compatibilidade com gamepads Bluetooth modernos (Nintendo Switch Pro / Clones, Xbox One, PS4, 8BitDo, etc.).
  - Pareamento persistente na memória NVS (não perde o pareamento ao desligar).
  - Escaneamento de novos dispositivos pausado automaticamente após a conexão para estabilidade e latência mínima.
- **Retorno Limpo ao Menu (Hotkeys):**
  - Segure **`SELECT + START`** por **1 segundo** durante qualquer jogo para sair do emulador e retornar ao menu.
  - **Sem reinicialização de hardware:** O processador não sofre reset (`esp_restart`), permitindo que a conexão Bluetooth permaneça ativa sem desconectar o controle.
  - Botão físico `BOOT` (GPIO 0) da placa também funciona como atalho de emergência (segurar por 2 segundos).
- **Particionamento de Flash Customizado:** Esquema `no_ota.csv` com partição de aplicativo expandida para 2 MB, permitindo múltiplas ROMs e assets gráficos sem esgotar a memória Flash.

---

## 🖥️ Hardware e Pinagem

### Placa Alvo:
- **IdeaSpark ESP32-WROOM-32** (ESP32-D0WD dual-core 240 MHz, 4 MB Flash, 520 KB SRAM).
- Display IPS LCD integrado: **ST7789 (240x135)**.

### Mapeamento dos Pinos (SPI ST7789):
| Função / Pino | GPIO ESP32 | Descrição |
|---|---|---|
| **MOSI** | `GPIO 23` | Linha de dados SPI |
| **SCLK** | `GPIO 18` | Clock SPI (40 MHz) |
| **CS** | `GPIO 15` | Chip Select do display |
| **DC** | `GPIO 2` | Data / Command |
| **RST** | `GPIO 4` | Reset do display |
| **BL** | `GPIO 32` | Backlight (iluminação de fundo) |
| **BOOT** | `GPIO 0` | Botão físico integrado (Start / Fallback) |

---

## 🎮 Mapeamento de Controles (Ace Gamer / Gamepads)

| Ação no NES | Botão no Controle Bluetooth | Botão Físico Placa |
|---|---|---|
| **Direcionais** | D-Pad (Cima / Baixo / Esquerda / Direita) ou Analógico Esquerdo | - |
| **Botão A** (Pulo) | Botão **A**, **Y** ou **R1** | - |
| **Botão B** (Ação / Ataque) | Botão **B**, **X** ou **L1** | - |
| **SELECT** | **SELECT**, **BACK**, **L2** ou **L3** | - |
| **START** (Pausa / Início) | **START**, **HOME**, **R2** ou **R3** | Botão `BOOT` (após 3s) |
| **Sair para o Menu** | **SELECT + START** (segurar por 1 segundo) | Botão `BOOT` (segurar 2s) |

---

## 📡 Como Parear o Controle Ace Gamer

1. Ligue a placa ESP32 conectando-a à alimentação USB. O display iniciará no menu mostrando `[BUSCA]` no canto superior direito.
2. Coloque o controle **Ace Gamer Pocket** em modo de pareamento:
   - Segure o botão **Home / Sync** até os LEDs frontais começarem a piscar rapidamente de um lado para o outro.
3. O ESP32 detectará o controle em poucos segundos. O display mudará de `[BUSCA]` para `[BT OK]` em verde e o terminal serial exibirá o modelo e nível de bateria.
4. Navegue pelo menu usando o D-pad e pressione **A** ou **START** para iniciar o jogo desejado.
5. Para trocar de jogo, segure **`SELECT + START`** por 1 segundo: o jogo será fechado e o controle continuará conectado imediatamente.

---

## 🛠️ Como Compilar e Gravar

O projeto é configurado para compilação via **PlatformIO CLI** ou extensão PlatformIO no VS Code.

### 1. Pré-requisitos
- Python 3.8+ e PlatformIO instalados:
  ```bash
  pip install --user platformio
  ```

### 2. Compilação
```bash
pio run
```

### 3. Gravação na Placa
Conecte a placa via USB (normalmente `/dev/ttyUSB0` no Linux ou `COMx` no Windows):
```bash
pio run -t upload
```

### 4. Monitor Serial
Para acompanhar logs de depuração e status de conexão Bluetooth (115200 baud):
```bash
pio device monitor -b 115200
```

---

## 📁 Estrutura de Arquivos

```
esp_retrogames/
├── backup_firmware.bin         # Backup completo original de fábrica (4 MB)
├── Donkey Kong (World).nes     # ROM avulsa para referência
├── platformio.ini              # Configurações do PlatformIO, flags ST7789 e Bluepad32
├── platformio_original.ini     # Cópia do platformio.ini original
├── main_original.cpp.bak       # Cópia do firmware de teste original
├── .gitignore                  # Pastas de build ignoradas (.pio, .vscode)
├── README.md                   # Esta documentação
├── lib/
│   └── nofrendo/               # Emulador Nofrendo NES otimizado para ESP32
│       ├── library.properties
│       └── src/
└── src/
    ├── main.cpp                # Ponto de entrada, menu interativo e loop principal
    ├── games_roms.h            # Declarações e metadados das 6 ROMs
    ├── games_roms.c            # Binários das 6 ROMs embarcados diretamente em Flash
    ├── display_tft.h           # Interface gráfica do display
    ├── display_tft.cpp         # Implementação de renderização e inicialização TFT_eSPI
    ├── controller_bp32.h       # Interface do controle Bluepad32
    ├── controller_bp32.cpp     # Tarefa Bluetooth (Core 0), mapeamento e hotkeys
    └── osd_esp32.c             # Integração do Nofrendo com o display e inputs do ESP32
```

---

## 💾 Restauração da Firmware de Fábrica

Caso deseje restaurar o dispositivo para a firmware demo original de fábrica que acompanhava a placa:
```bash
esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 460800 write_flash 0x0 backup_firmware.bin
```

---

## 📜 Créditos e Tecnologias

- **Nofrendo:** Emulador NES portátil em C criado por Matthew Conte.
- **Bluepad32:** Pilha Bluetooth open-source para gamepads criada por Ricardo Quesada ([Bluepad32](https://github.com/ricardoquesada/bluepad32)).
- **TFT_eSPI:** Biblioteca gráfica de alta performance desenvolvida por Bodmer ([TFT_eSPI](https://github.com/Bodmer/TFT_eSPI)).
- **IdeaSpark:** Placa ESP32 com display ST7789 integrado.
