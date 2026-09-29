#include <Arduino.h>
#include <SPIFFS.h>
#include <TFT_eSPI.h>
#include <esp_task_wdt.h>

#include "display_tft.h"
#include "controller_bp32.h"
#include "games_roms.h"

extern "C" {
#include <nofrendo.h>
}

// Configura o loopTask do Arduino com 16KB de stack para a emulação NES
SET_LOOP_TASK_STACK_SIZE(16384);

TFT_eSPI tft = TFT_eSPI();

void draw_menu_ui(int selected, bool bt_connected) {
    // Top Bar (Y: 0..20)
    tft.fillRect(0, 0, 240, 20, TFT_NAVY);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawString("NES RETRO ARCADE", 6, 2, 2);

    if (bt_connected) {
        tft.setTextColor(TFT_GREEN, TFT_NAVY);
        tft.drawString("[BT OK]", 175, 2, 2);
    } else {
        tft.setTextColor(TFT_ORANGE, TFT_NAVY);
        tft.drawString("[BUSCA]", 175, 2, 2);
    }

    // Games List (Y: 21..117, 16px per row)
    for (int i = 0; i < g_games_count; i++) {
        int y = 21 + i * 16;
        if (i == selected) {
            // Linha selecionada (destaque ciano)
            tft.fillRect(0, y, 240, 16, TFT_DARKCYAN);
            tft.setTextColor(TFT_YELLOW, TFT_DARKCYAN);
            tft.drawString(">", 6, y, 2);
            tft.setTextColor(TFT_WHITE, TFT_DARKCYAN);
            tft.drawString(g_games_list[i].title, 20, y, 2);
        } else {
            tft.fillRect(0, y, 240, 16, TFT_BLACK);
            tft.setTextColor(TFT_LIGHTGREY, TFT_BLACK);
            tft.drawString(g_games_list[i].title, 20, y, 2);
        }
    }

    // Bottom Bar (Y: 118..135)
    tft.fillRect(0, 118, 240, 17, TFT_NAVY);
    tft.setTextColor(TFT_CYAN, TFT_NAVY);
    tft.drawString(g_games_list[selected].subtext, 6, 120, 2);
}

int run_game_menu() {
    int selected = 0;
    bool last_bt_state = false;
    draw_menu_ui(selected, controller_is_connected());

    uint32_t last_input_time = 0;
    uint32_t boot_press_time = 0;
    bool boot_was_released = false;

    while (true) {
        bool curr_bt = controller_is_connected();
        if (curr_bt != last_bt_state) {
            last_bt_state = curr_bt;
            draw_menu_ui(selected, curr_bt);
        }

        uint32_t input = controller_read_input();
        bool up = !(input & (1 << 0));
        bool down = !(input & (1 << 1));
        bool start_btn = !(input & (1 << 5));
        bool a_btn = !(input & (1 << 6));

        // Guarda se o botão BOOT já esteve solto após o boot
        if (digitalRead(0) == HIGH) {
            boot_was_released = true;
        }

        // Botão físico BOOT da placa (GPIO 0)
        bool boot_phys = (boot_was_released && (digitalRead(0) == LOW));

        if (boot_phys) {
            if (boot_press_time == 0) {
                boot_press_time = millis();
            } else if (millis() - boot_press_time > 1000) {
                // Segurou BOOT por mais de 1s -> Inicia o jogo!
                break;
            }
        } else if (boot_press_time > 0) {
            // Toque rápido no BOOT -> Próximo jogo!
            boot_press_time = 0;
            selected = (selected + 1) % g_games_count;
            draw_menu_ui(selected, curr_bt);
            delay(150);
        }

        // Navegação pelo D-Pad do controle Ace Gamer
        if (millis() - last_input_time > 180) {
            if (up) {
                selected = (selected - 1 + g_games_count) % g_games_count;
                draw_menu_ui(selected, curr_bt);
                last_input_time = millis();
            } else if (down) {
                selected = (selected + 1) % g_games_count;
                draw_menu_ui(selected, curr_bt);
                last_input_time = millis();
            } else if (a_btn || start_btn) {
                // Botão A ou START pressionado no controle -> Inicia o jogo!
                break;
            }
        }

        delay(20);
    }

    return selected;
}

void setup() {
    Serial.begin(115200);
    delay(300);
    Serial.println("\n============================================");
    Serial.println("  ESP32 RetroGames - NES Multi-Jogos Arcade");
    Serial.println("  IdeaSpark ST7789 240x135 + Bluepad32");
    Serial.println("============================================");

    // Desativa WDT dos núcleos durante a emulação
    TaskHandle_t idle_0 = xTaskGetIdleTaskHandleForCPU(0);
    if (idle_0 != NULL) esp_task_wdt_delete(idle_0);
    TaskHandle_t idle_1 = xTaskGetIdleTaskHandleForCPU(1);
    if (idle_1 != NULL) esp_task_wdt_delete(idle_1);

    // Inicializa display TFT ST7789
    display_begin();

    // Inicializa SPIFFS
    Serial.println("[FS] Montando SPIFFS...");
    if (!SPIFFS.begin(true, "/fs")) {
        Serial.println("[ERRO] Falha ao montar SPIFFS!");
    }

    // Inicia pilha Bluetooth no Core 0 (roda de forma perpétua)
    Serial.println("[BT] Iniciando pilha Bluetooth no Core 0...");
    start_bluetooth_task();
}

void loop() {
    // 1. Marca que não estamos dentro do jogo
    set_game_running(false);

    // 2. Executa o Menu Interativo na tela TFT
    int selected = run_game_menu();

    select_game(selected);
    Serial.printf("[MENU] Jogo selecionado: %s (%s)\n", g_games_list[selected].title, g_games_list[selected].subtext);

    // 3. Tela de Carregamento
    tft.fillScreen(TFT_BLACK);
    tft.fillRect(0, 0, 240, 22, TFT_NAVY);
    tft.setTextColor(TFT_YELLOW, TFT_NAVY);
    tft.drawString("CARREGANDO JOGO", 10, 3, 2);

    tft.setTextColor(TFT_WHITE, TFT_BLACK);
    tft.drawString(g_games_list[selected].title, 15, 38, 4);

    tft.setTextColor(TFT_CYAN, TFT_BLACK);
    tft.drawString(g_games_list[selected].subtext, 15, 75, 2);

    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("SELECT + START: Menu", 15, 105, 2);

    // 4. Grava a ROM selecionada no SPIFFS (/game.nes)
    if (SPIFFS.exists("/game.nes")) {
        SPIFFS.remove("/game.nes");
    }
    File f = SPIFFS.open("/game.nes", "w");
    if (f) {
        f.write(selected_rom_data, selected_rom_size);
        f.close();
        Serial.println("[FS] ROM gravada no SPIFFS com sucesso!");
    } else {
        Serial.println("[ERRO] Falha ao gravar ROM!");
    }

    delay(400);

    // 5. Marca que o jogo está rodando (habilita o combo SELECT + START)
    set_game_running(true);

    // 6. Inicia a emulação diretamente no Core 1
    char *argv[1];
    argv[0] = (char *)"/fs/game.nes";
    Serial.printf("[NES] Executando %s...\n", g_games_list[selected].title);
    nofrendo_main(1, argv);

    // 7. Quando o usuário pressiona SELECT + START, main_quit() faz nofrendo_main retornar aqui!
    Serial.println("[NES] Jogo encerrado. Retornando ao menu principal (Bluetooth ativo!)...");
    set_game_running(false);

    display_begin(); // Limpa e prepara a tela para o menu
}
