#include <Arduino.h>
#include <Bluepad32.h>
#include "controller_bp32.h"

extern "C" void main_quit(void);

static ControllerPtr myGamepad = nullptr;

static void onConnectedController(ControllerPtr ctl) {
    if (myGamepad == nullptr) {
        myGamepad = ctl;
        ControllerProperties properties = ctl->getProperties();
        Serial.println("\n========================================");
        Serial.println("[BT] *** CONTROLE CONECTADO COM SUCESSO! ***");
        Serial.printf("[BT] Modelo detectado: %s\n", ctl->getModelName().c_str());
        Serial.printf("[BT] VID: 0x%04x, PID: 0x%04x\n", properties.vendor_id, properties.product_id);
        Serial.printf("[BT] Nível de Bateria: %d%%\n", ctl->battery());
        Serial.println("========================================\n");

        // Desativa a descoberta/inquiry para não sobrecarregar o rádio e estabilizar a conexão
        BP32.enableNewBluetoothConnections(false);
    }
}

static void onDisconnectedController(ControllerPtr ctl) {
    if (myGamepad == ctl) {
        Serial.println("\n[BT] *** CONTROLE DESCONECTADO! ***\n");
        myGamepad = nullptr;
        // Reativa a descoberta para permitir reconexão
        BP32.enableNewBluetoothConnections(true);
    }
}

static void taskBluetooth(void *pvParameters) {
    Serial.printf("[BT] Versão Bluepad32: %s\n", BP32.firmwareVersion());
    const uint8_t* addr = BP32.localBdAddress();
    Serial.printf("[BT] Endereço MAC Bluetooth do ESP32: %02X:%02X:%02X:%02X:%02X:%02X\n",
                  addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);

    BP32.setup(&onConnectedController, &onDisconnectedController);
    BP32.enableVirtualDevice(false);
    BP32.enableBLEService(false);
    BP32.enableNewBluetoothConnections(true);
    Serial.println("[BT] Pilha Bluetooth iniciada. Modo de conexão/descoberta ativo!");

    uint32_t lastPrint = millis();

    while (true) {
        BP32.update();

        // Log a cada 5 segundos se ainda não conectou
        if (myGamepad == nullptr && (millis() - lastPrint > 5000)) {
            lastPrint = millis();
            Serial.println("[BT] Escaneando... Coloque o controle Ace Gamer em modo de pareamento.");
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

extern "C" void start_bluetooth_task() {
    xTaskCreatePinnedToCore(
        taskBluetooth,
        "TaskBT",
        6144,
        NULL,
        2, // Prioridade 2 para processamento ágil de pacotes BT
        NULL,
        0 // Núcleo 0 dedicado ao Bluetooth stack
    );
}

extern "C" void controller_init() {
    pinMode(0, INPUT_PULLUP); // Botão BOOT da placa como fallback
}

static bool g_game_running = false;

extern "C" void set_game_running(bool running) {
    g_game_running = running;
}

extern "C" bool controller_is_connected() {
    return (myGamepad != nullptr && myGamepad->isConnected());
}

extern "C" uint32_t controller_read_input() {
    uint32_t val = 0xFFFFFFFF; // 1 = solto, 0 = pressionado

    if (myGamepad && myGamepad->isConnected()) {
        // Direcionais (D-Pad e Analógico Esquerdo)
        if ((myGamepad->dpad() & DPAD_UP) || (myGamepad->axisY() < -200)) val &= ~(1 << 0);
        if ((myGamepad->dpad() & DPAD_DOWN) || (myGamepad->axisY() > 200)) val &= ~(1 << 1);
        if ((myGamepad->dpad() & DPAD_LEFT) || (myGamepad->axisX() < -200)) val &= ~(1 << 2);
        if ((myGamepad->dpad() & DPAD_RIGHT) || (myGamepad->axisX() > 200)) val &= ~(1 << 3);

        // Select: Select / Back / L2 / L3
        if (myGamepad->miscSelect() || 
            (myGamepad->miscButtons() & MISC_BUTTON_BACK) || 
            (myGamepad->miscButtons() & MISC_BUTTON_SELECT) ||
            myGamepad->l2() || myGamepad->thumbL()) {
            val &= ~(1 << 4);
        }

        // Start: Start / Home / System / R2 / R3
        if (myGamepad->miscStart() || 
            myGamepad->miscSystem() ||
            (myGamepad->miscButtons() & MISC_BUTTON_START) || 
            (myGamepad->miscButtons() & MISC_BUTTON_HOME) || 
            (myGamepad->miscButtons() & MISC_BUTTON_SYSTEM) ||
            myGamepad->r2() || myGamepad->thumbR()) {
            val &= ~(1 << 5);
        }

        // Botões de Ação NES:
        // NES A (Pulo no Mario / Donkey Kong): Botão A ou Y do controle, ou R1
        if (myGamepad->a() || myGamepad->y() || myGamepad->r1()) val &= ~(1 << 6);
        // NES B (Ação / Marreta): Botão B ou X do controle, ou L1
        if (myGamepad->b() || myGamepad->x() || myGamepad->l1()) val &= ~(1 << 7);
    }

    // Botão BOOT da placa (GPIO 0) simula START somente após 3s do boot
    if (millis() > 3000 && digitalRead(0) == LOW) {
        val &= ~(1 << 5);
    }

    // Durante o jogo: Se segurar SELECT + START no controle por 1.2s ou segurar o botão BOOT por 2.5s, reinicia para o menu!
    if (g_game_running) {
        static uint32_t reset_start = 0;
        bool select_down = !(val & (1 << 4));
        bool start_down = !(val & (1 << 5));
        bool combo_controller = (select_down && start_down);
        bool combo_boot = (millis() > 3000 && digitalRead(0) == LOW);

        if (combo_controller || combo_boot) {
            if (reset_start == 0) {
                reset_start = millis();
            } else {
                uint32_t threshold = combo_boot ? 2000 : 1000;
                if (millis() - reset_start > threshold) {
                    Serial.println("[SYSTEM] Retornando ao Menu de Jogos (Sem reiniciar placa / Sem desconectar Bluetooth)...");
                    main_quit();
                    reset_start = 0;
                }
            }
        } else {
            reset_start = 0;
        }
    }

    return val;
}
