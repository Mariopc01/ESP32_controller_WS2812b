// Inclui as bibliotecas necessárias
// Instale 'RTClib' e 'FastLED' através do Gerenciador de Bibliotecas da IDE Arduino.
// Para Wi-Fi e NTP, certifique-se de ter o suporte para ESP32 instalado (Gerenciador de Placas).
#include <Wire.h>     // Para comunicação I2C com o módulo RTC
#include <RTClib.h>   // Para o módulo Real Time Clock (RTC)
#include <FastLED.h>  // Para controlar as fitas de LED WS2812B/NeoPixel
#include <WiFi.h>     // Para funcionalidades Wi-Fi do ESP32
#include <NTPClient.h> // Para sincronizar a hora via NTP (Network Time Protocol)
#include <WiFiUdp.h>  // Necessário para o NTPClient

// --- Configurações da Fita de LED ---
#define LED_PIN     2    // Pino GPIO do ESP32 onde o DATA IN da fita LED está conectado
#define NUM_LEDS    60   // Número de LEDs na sua fita (ajuste conforme o seu comprimento)
#define LED_TYPE    WS2812B // Tipo de LED. Se for NeoPixel, pode ser WS2812.
#define COLOR_ORDER GRB  // Ordem das cores. Para WS2812B é GRB. Se as cores estiverem erradas, tente RGB ou BGR.

// Cria um array para armazenar os objetos de LED
CRGB leds[NUM_LEDS];

// --- Configurações do Módulo RTC ---
RTC_DS3231 rtc; // Cria um objeto para o módulo RTC DS3231

// --- Configurações Wi-Fi e NTP ---
// Defina suas duas redes Wi-Fi aqui.
// O ESP32 tentara se conectar a rede 0, depois a rede 1.
const char* ssid[] = {"CM122", "NET_2G9AAF23"}; // Substitua pelos nomes das suas redes Wi-Fi
const char* password[] = {"Alclfbf48592@", "689AAF23"}; // Substitua pelas senhas das suas redes Wi-Fi
const int NUM_WIFI_NETWORKS = 2; // Numero de redes Wi-Fi configuradas

WiFiUDP ntpUDP;
// NTPClient(ntpUDP, "servidor_ntp", offset_horas_segundos, intervalo_atualizacao_segundos)
// "pool.ntp.org" eh um servidor NTP publico confiavel.
// -3 * 3600 para fuso horario de Brasilia (GMT-3). Ajuste se estiver em outro fuso horario.
// 60000 significa que ele atualizara a cada 60 segundos (nao eh usado para o ajuste inicial do RTC)
NTPClient timeClient(ntpUDP, "pool.ntp.org", -3 * 3600, 60000);

// --- Definição das Cores para Cada Dia da Semana ---
// As cores são definidas em formato CRGB
// O array é indexado de Domingo (0) a Sábado (6)
CRGB colorsPerDay[7] = {
    CRGB::Blue,       // Domingo (0)
    CRGB::Yellow,     // Segunda-feira (1)
    CRGB::DeepPink,   // Terça-feira (2) - 'Rosa' mais vibrante
    CRGB::White,      // Quarta-feira (3)
    CRGB::Green,      // Quinta-feira (4)
    CRGB(139, 0, 0),  // Sexta-feira (5) - 'Rubi' (usando valor RGB direto para um tom de vermelho profundo)
    CRGB::Violet      // Sábado (6) - 'Violeta'
};

// Variável para armazenar o dia da semana atual
int currentDay = -1; // Inicializa com um valor que não é um dia da semana válido

// Variável para armazenar a última hora em que a animação horária foi executada
int lastAnimatedHour = -1;

// --- Configurações do Botão de Modo ---
#define MODE_BUTTON_PIN  23   // Pino GPIO do ESP32 onde o botão de MODO está conectado (Ex: GPIO 23)
#define DEBOUNCE_DELAY 50 // Tempo de debounce em milissegundos

int modeButtonState;        // Estado atual do botão de modo
int lastModeButtonState = HIGH; // Último estado lido do botão de modo (HIGH porque usaremos INPUT_PULLUP)
unsigned long lastModeDebounceTime = 0; // Último tempo em que o pino de modo foi alternado

// --- Variáveis de Controle de Modo ---
enum LightMode { NORMAL_MODE, PRESENTATION_MODE };
LightMode currentMode = NORMAL_MODE; // Inicia no modo normal

// --- Variáveis para o Modo Apresentação ---
int presentationFormIndex = 0; // Índice da forma de apresentação atual (0 a 6)
unsigned long formStartTime = 0; // Tempo de início da forma de apresentação atual
const unsigned long FORM_DURATION_MS = 30000; // Duração de cada forma em milissegundos (30 segundos)

// --- Configurações e Variáveis de Brilho ---
#define BRIGHTNESS_UP_BUTTON_PIN   22 // Pino GPIO para aumentar brilho (Ex: GPIO 22)
#define BRIGHTNESS_DOWN_BUTTON_PIN 21 // Pino GPIO para diminuir brilho (Ex: GPIO 21)

// Converte porcentagens para valores de 0-255
const uint8_t MIN_BRIGHTNESS_VALUE = (uint8_t)(0.10 * 255); // 10% de 255
const uint8_t MAX_BRIGHTNESS_VALUE = (uint8_t)(0.50 * 255); // 50% de 255
const uint8_t BRIGHTNESS_STEP_VALUE = (uint8_t)(0.05 * 255); // 5% de 255

uint8_t currentBrightness; // Variável para armazenar o brilho atual (0-255)

int brightnessUpButtonState;
int lastBrightnessUpButtonState = HIGH;
unsigned long lastBrightnessUpDebounceTime = 0;

int brightnessDownButtonState;
int lastBrightnessDownButtonState = HIGH;
unsigned long lastBrightnessDownDebounceTime = 0;

// --- Variáveis para controle da animação de piscar suave (NÃO-BLOQUEANTE) ---
bool softBlinkActive = false;
unsigned long softBlinkLastChangeMillis = 0;
int softBlinkCurrentPhase = 0; // 0: fade out, 1: off, 2: fade in
int softBlinkCount = 0;
CRGB softBlinkOriginalColor;
uint8_t softBlinkOriginalBrightness;
const int SOFT_BLINK_PHASE_DURATION = 100; // ms para cada fase (off, meio do fade)
const int SOFT_BLINK_FADE_STEPS = 10; // Passos para o fade

// --- Variáveis para controle da animação de Ano Novo (NÃO-BLOQUEANTE) ---
bool newYearAnimationActive = false;
unsigned long newYearAnimationStartTime = 0;
const unsigned long NEW_YEAR_ANIMATION_DURATION_MS = 60000; // 1 minuto

// Protótipos das funções de animação (algumas foram modificadas para não-bloqueantes)
void startSoftBlink();
void updateSoftBlink();
void stopSoftBlink();

void startNewYearAnimation();
void updateNewYearAnimation();
void stopNewYearAnimation();

void runPresentationForm(int index); // Função para rodar as formas de apresentação

// Função para tentar conectar ao Wi-Fi
bool connectToWiFi() {
    Serial.println("\nTentando conectar ao Wi-Fi...");
    for (int i = 0; i < NUM_WIFI_NETWORKS; i++) {
        Serial.print("Conectando a rede: ");
        Serial.println(ssid[i]);
        WiFi.begin(ssid[i], password[i]);

        long startTime = millis();
        while (WiFi.status() != WL_CONNECTED && millis() - startTime < 15000) { // Tenta por 15 segundos
            delay(500);
            Serial.print(".");
        }
        Serial.println();

        if (WiFi.status() == WL_CONNECTED) {
            Serial.print("Conectado a rede Wi-Fi! IP: ");
            Serial.println(WiFi.localIP());
            return true;
        } else {
            Serial.println("Falha na conexao com esta rede.");
        }
    }
    Serial.println("Falha na conexao com todas as redes Wi-Fi configuradas.");
    return false;
}

void setup() {
    Serial.begin(115200); // Para ESP32, a taxa de baud recomendada para serial é 115200
    while (!Serial); // Aguarda a porta serial conectar (útil para depuração)

    // Configura os pinos dos botões como entrada com resistor pull-up interno
    pinMode(MODE_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BRIGHTNESS_UP_BUTTON_PIN, INPUT_PULLUP);
    pinMode(BRIGHTNESS_DOWN_BUTTON_PIN, INPUT_PULLUP);

    // Inicializa o FastLED
    FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
    
    // Define o brilho inicial (no máximo permitido, 50%)
    currentBrightness = MAX_BRIGHTNESS_VALUE;
    FastLED.setBrightness(currentBrightness); 

    // Inicializa o módulo I2C para o RTC (usando pinos 16 e 17)
    // Conecte SDA do RTC ao GPIO 16 e SCL do RTC ao GPIO 17.
    Wire.begin(16, 17); // Inicializa I2C nos pinos SDA=GPIO16, SCL=GPIO17

    // Verifica se o módulo RTC está conectado e funcionando
    if (!rtc.begin()) {
        Serial.println("ERRO: Nao foi possivel encontrar o modulo RTC!");
        Serial.println("Verifique as conexoes (SDA em GPIO 16, SCL em GPIO 17) ou se a biblioteca esta instalada.");
        // Não para aqui, tenta sincronizar via Wi-Fi como alternativa
    }

    // --- Sincronizacao de Hora: Preferencia Wi-Fi/NTP, depois RTC ou Compilacao ---
    bool timeSynced = false;
    if (connectToWiFi()) {
        timeClient.begin();
        // A primeira atualizacao pode levar alguns segundos
        Serial.println("Sincronizando hora via NTP...");
        if (timeClient.forceUpdate()) { // ForceUpdate garante que a hora seja obtida imediatamente
            unsigned long epochTime = timeClient.getEpochTime();
            rtc.adjust(DateTime(epochTime)); // Ajusta o RTC com a hora do NTP
            Serial.println("RTC sincronizado com sucesso via NTP!");
            timeSynced = true;
        } else {
            Serial.println("Falha ao sincronizar hora via NTP.");
        }
    }

    if (!timeSynced) {
        if (rtc.lostPower()) {
            Serial.println("AVISO: RTC perdeu energia ou nao foi sincronizado via NTP. Definindo a hora para o tempo de compilacao!");
            rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
        } else {
            // Se o RTC nao perdeu energia e nao foi sincronizado via NTP, ele esta mantendo a hora.
            Serial.println("RTC esta mantendo a hora. Sem necessidade de ajuste inicial.");
        }
    } else {
        // Se a hora foi sincronizada via NTP, garantimos que o RTC nao 'lostPower' aqui.
        // O RTC.adjust() ja foi chamado com a hora do NTP.
    }
    
    // Define a cor inicial baseada no dia da semana atual
    DateTime now = rtc.now(); // Obtem a data e hora atuais do RTC
    currentDay = now.dayOfTheWeek(); // Pega o dia da semana (0=domingo, 1=segunda...)
    Serial.print("Dia da semana inicial: ");
    String days[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"}; // Array para imprimir o dia da semana
    Serial.println(days[currentDay]);
    
    // Define todos os LEDs para a cor do dia atual
    fill_solid(leds, NUM_LEDS, colorsPerDay[currentDay]);
    FastLED.show(); // Envia a cor para a fita de LED

    lastAnimatedHour = now.hour(); // Inicializa a ultima hora animada para evitar animacao no boot.
}

void loop() {
    // --- Leitura e Debounce do Botão de MODO ---
    int modeReading = digitalRead(MODE_BUTTON_PIN);
    if (modeReading != lastModeButtonState) {
        lastModeDebounceTime = millis();
    }
    if ((millis() - lastModeDebounceTime) > DEBOUNCE_DELAY) {
        if (modeReading != modeButtonState) {
            modeButtonState = modeReading;
            if (modeButtonState == LOW) { // Botão de modo foi pressionado
                Serial.println("Botao de MODO Pressionado!");
                if (currentMode == NORMAL_MODE) {
                    currentMode = PRESENTATION_MODE;
                    Serial.println("Modo: APRESENTACAO");
                    // Para garantir que nao haja animacao horaria ativa ao entrar no modo apresentacao
                    stopSoftBlink();
                    stopNewYearAnimation();
                    presentationFormIndex = 0; // Começa da primeira forma
                    formStartTime = millis(); // Inicia o timer da primeira forma
                } else { // currentMode == PRESENTATION_MODE
                    currentMode = NORMAL_MODE;
                    Serial.println("Modo: NORMAL");
                    // Ao voltar para o modo normal, restaura a cor do dia imediatamente
                    fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]);
                    FastLED.setBrightness(currentBrightness); // Garante brilho normal
                    FastLED.show();
                }
            }
        }
    }
    lastModeButtonState = modeReading;

    // --- Leitura e Debounce do Botão de AUMENTAR Brilho ---
    int brightnessUpReading = digitalRead(BRIGHTNESS_UP_BUTTON_PIN);
    if (brightnessUpReading != lastBrightnessUpButtonState) {
        lastBrightnessUpDebounceTime = millis();
    }
    if ((millis() - lastBrightnessUpDebounceTime) > DEBOUNCE_DELAY) {
        if (brightnessUpReading != brightnessUpButtonState) {
            brightnessUpButtonState = brightnessUpReading;
            if (brightnessUpButtonState == LOW) { // Botão de aumentar brilho foi pressionado
                Serial.println("Botao AUMENTAR Brilho Pressionado!");
                if (currentBrightness + BRIGHTNESS_STEP_VALUE <= MAX_BRIGHTNESS_VALUE) {
                    currentBrightness += BRIGHTNESS_STEP_VALUE;
                } else {
                    currentBrightness = MAX_BRIGHTNESS_VALUE; // Garante que nao exceda o maximo
                }
                FastLED.setBrightness(currentBrightness);
                // Força a atualização dos LEDs para refletir o novo brilho imediatamente
                fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]);
                FastLED.show();
                Serial.print("Novo Brilho: ");
                Serial.println(currentBrightness);
            }
        }
    }
    lastBrightnessUpButtonState = brightnessUpReading;

    // --- Leitura e Debounce do Botão de DIMINUIR Brilho ---
    int brightnessDownReading = digitalRead(BRIGHTNESS_DOWN_BUTTON_PIN);
    if (brightnessDownReading != lastBrightnessDownButtonState) {
        lastBrightnessDownDebounceTime = millis();
    }
    if ((millis() - lastBrightnessDownDebounceTime) > DEBOUNCE_DELAY) {
        if (brightnessDownReading != brightnessDownButtonState) {
            brightnessDownButtonState = brightnessDownReading;
            if (brightnessDownButtonState == LOW) { // Botão de diminuir brilho foi pressionado
                Serial.println("Botao DIMINUIR Brilho Pressionado!");
                if (currentBrightness - BRIGHTNESS_STEP_VALUE >= MIN_BRIGHTNESS_VALUE) {
                    currentBrightness -= BRIGHTNESS_STEP_VALUE;
                } else {
                    currentBrightness = MIN_BRIGHTNESS_VALUE; // Garante que nao va abaixo do minimo
                }
                FastLED.setBrightness(currentBrightness);
                // Força a atualização dos LEDs para refletir o novo brilho imediatamente
                fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]);
                FastLED.show();
                Serial.print("Novo Brilho: ");
                Serial.println(currentBrightness);
            }
        }
    }
    lastBrightnessDownButtonState = brightnessDownReading;

    // --- Lógica Principal Baseada no Modo Atual ---
    if (currentMode == NORMAL_MODE) {
        DateTime now = rtc.now(); // Obtem a data e hora atuais do RTC
        int dayOfTheWeek = now.dayOfTheWeek();
        int currentHour = now.hour();
        int currentMinute = now.minute();
        int currentSecond = now.second();

        // Imprime a data e hora no Serial Monitor para depuracao
        Serial.print(now.year(), DEC);
        Serial.print('/');
        Serial.print(now.month(), DEC);
        Serial.print('/');
        Serial.print(now.day(), DEC);
        Serial.print(" (");
        String days[] = {"Dom", "Seg", "Ter", "Qua", "Qui", "Sex", "Sab"};
        Serial.print(days[dayOfTheWeek]);
        Serial.print(") ");
        Serial.print(currentHour, DEC);
        Serial.print(':');
        Serial.print(currentMinute, DEC);
        Serial.print(':');
        Serial.print(currentSecond, DEC);
        Serial.print(" (Modo NORMAL) Brilho: ");
        Serial.println(currentBrightness);

        // --- Lógica de Inicio da Animação Horária ---
        // Verifica se é o início de uma nova hora E se nenhuma animação horária está ativa
        if (currentHour != lastAnimatedHour && currentMinute == 0 && currentSecond == 0 && !softBlinkActive && !newYearAnimationActive) {
            Serial.println("TRIGGER: Nova hora para animacao horaria!");
            if (now.month() == 1 && now.day() == 1) { // É 1º de Janeiro?
                startNewYearAnimation();
            } else {
                startSoftBlink();
            }
            lastAnimatedHour = currentHour; // Atualiza a ultima hora animada
        }

        // --- Atualização Contínua das Animações Horárias Ativas (NÃO-BLOQUEANTE) ---
        if (softBlinkActive) {
            updateSoftBlink();
        } else if (newYearAnimationActive) {
            updateNewYearAnimation();
        } else {
            // Se nenhuma animacao horaria esta ativa, exibe a cor do dia
            // (Esta parte soh atualiza se o dia da semana mudou, ou se for a primeira vez)
            if (dayOfTheWeek != currentDay) {
                currentDay = dayOfTheWeek;
                Serial.print("O dia mudou! Nova cor para ");
                Serial.println(days[currentDay]);
                fill_solid(leds, NUM_LEDS, colorsPerDay[currentDay]);
                FastLED.setBrightness(currentBrightness);
                FastLED.show();
            } else {
                 // Certifica-se de que a cor do dia e o brilho estejam corretos se nao houver animacao e o dia nao mudou
                 // Isso eh importante para restaurar a cor do dia e o brilho apos sair do modo apresentacao ou após
                 // uma animacao horaria finalizar e o dia não ter mudado.
                fill_solid(leds, NUM_LEDS, colorsPerDay[currentDay]);
                FastLED.setBrightness(currentBrightness);
                FastLED.show();
            }
        }
    } else { // currentMode == PRESENTATION_MODE
        Serial.print("Modo: APRESENTACAO. Forma ");
        Serial.print(presentationFormIndex + 1);
        Serial.print(" de 7. Tempo restante: ");
        Serial.print((FORM_DURATION_MS - (millis() - formStartTime)) / 1000);
        Serial.print("s. Brilho: ");
        Serial.println(currentBrightness);

        // Avanca para a proxima forma se a duracao atual expirou
        if (millis() - formStartTime >= FORM_DURATION_MS) {
            presentationFormIndex++;
            if (presentationFormIndex >= 7) {
                presentationFormIndex = 0; // Volta para a primeira forma
            }
            formStartTime = millis(); // Reseta o timer para a nova forma
            Serial.print("Mudando para a forma de apresentacao ");
            Serial.println(presentationFormIndex + 1);
        }
        
        // Executa a forma de apresentacao atual (estas ja sao nao-bloqueantes)
        runPresentationForm(presentationFormIndex);
    }
    
    // Nao ha mais 'delay(10)' aqui para garantir que o loop execute o mais rapido possivel
    // e responda aos botoes e temporizacoes das animacoes.
}

// --- Funções para a Animação de Piscar Suave (NÃO-BLOQUEANTE) ---

// Inicia a animação de piscar suave
void startSoftBlink() {
    softBlinkActive = true;
    softBlinkLastChangeMillis = millis();
    softBlinkCurrentPhase = 0; // Começa com fade out
    softBlinkCount = 0;
    softBlinkOriginalColor = colorsPerDay[rtc.now().dayOfTheWeek()]; // Pega a cor atual do dia
    softBlinkOriginalBrightness = currentBrightness; // Armazena o brilho atual para a animação
    Serial.println("Soft Blink: Iniciado.");
}

// Atualiza a animação de piscar suave (chamada repetidamente no loop)
void updateSoftBlink() {
    unsigned long currentTime = millis();
    uint8_t targetBrightness;
    
    // Calcula o progresso do fade e define o brilho alvo
    if (softBlinkCurrentPhase == 0) { // Fase 0: Fade out
        int fadeProgress = map(currentTime - softBlinkLastChangeMillis, 0, SOFT_BLINK_PHASE_DURATION, 0, 100);
        targetBrightness = map(fadeProgress, 0, 100, softBlinkOriginalBrightness, 0);
    } else if (softBlinkCurrentPhase == 1) { // Fase 1: LEDs desligados
        targetBrightness = 0;
    } else { // Fase 2: Fade in
        int fadeProgress = map(currentTime - softBlinkLastChangeMillis, 0, SOFT_BLINK_PHASE_DURATION, 0, 100);
        targetBrightness = map(fadeProgress, 0, 100, 0, softBlinkOriginalBrightness);
    }

    FastLED.setBrightness(targetBrightness);
    fill_solid(leds, NUM_LEDS, softBlinkOriginalColor);
    FastLED.show();

    // Transição de fases da animação
    if (currentTime - softBlinkLastChangeMillis >= SOFT_BLINK_PHASE_DURATION) {
        softBlinkLastChangeMillis = currentTime;
        softBlinkCurrentPhase++;

        if (softBlinkCurrentPhase > 2) { // Completou as 3 fases de um piscar (FadeOut -> Off -> FadeIn)
            softBlinkCurrentPhase = 0; // Reinicia para o proximo piscar
            softBlinkCount++;
            Serial.print("Soft Blink: Piscar "); Serial.print(softBlinkCount); Serial.println(" de 7.");
        }
    }

    // Verifica se todos os piscares foram completados
    if (softBlinkCount >= 7) { // 7 vezes
        stopSoftBlink();
    }
}

// Para a animação de piscar suave e restaura o estado normal
void stopSoftBlink() {
    softBlinkActive = false;
    softBlinkCount = 0; // Reseta o contador para a próxima vez
    // Garante que o brilho e a cor sejam restaurados para o estado normal do dia
    fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]);
    FastLED.setBrightness(currentBrightness);
    FastLED.show();
    Serial.println("Soft Blink: Finalizado.");
}

// --- Funções para a Animação de Ano Novo (NÃO-BLOQUEANTE) ---

// Inicia a animação de Ano Novo
void startNewYearAnimation() {
    newYearAnimationActive = true;
    newYearAnimationStartTime = millis();
    Serial.println("Ano Novo: Iniciado.");
}

// Atualiza a animação de Ano Novo (chamada repetidamente no loop)
void updateNewYearAnimation() {
    unsigned long currentTime = millis();
    
    if (currentTime - newYearAnimationStartTime < NEW_YEAR_ANIMATION_DURATION_MS) {
        fill_rainbow(leds, NUM_LEDS, beatsin8(20, 0, 255)); // Animação de arco-íris
        FastLED.setBrightness(currentBrightness); // Garante que o brilho é o atual
        FastLED.show();
    } else {
        stopNewYearAnimation();
    }
}

// Para a animação de Ano Novo e restaura o estado normal
void stopNewYearAnimation() {
    newYearAnimationActive = false;
    // Garante que o brilho e a cor sejam restaurados para o estado normal do dia
    fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]);
    FastLED.setBrightness(currentBrightness);
    FastLED.show();
    Serial.println("Ano Novo: Finalizado.");
}

// --- Funções para as 7 Formas de Apresentação (JÁ SÃO NÃO-BLOQUEANTES) ---
// Elas executam apenas um "passo" da animação e são chamadas repetidamente no loop

void runPresentationForm(int index) {
    FastLED.setBrightness(currentBrightness); // Garante que o brilho está definido no modo apresentação

    switch (index) {
        case 0: rainbowCycle(); break;
        case 1: twinkleEffect(); break;
        case 2: cylonScanner(); break;
        case 3: fireEffect(); break;
        case 4: confettiEffect(); break;
        case 5: colorWipe(); break;
        case 6: pulseEffect(); break;
    }
}

// Forma 0: Ciclo de Arco-Íris
void rainbowCycle() {
    fill_rainbow(leds, NUM_LEDS, beatsin8(20, 0, 255)); // beatsin8 para movimento suave
    FastLED.show();
}

// Forma 1: Twinkle (cintilante)
void twinkleEffect() {
    fadeToBlackBy(leds, NUM_LEDS, 20); // Faz os LEDs existentes desbotarem
    int pixel = random(NUM_LEDS);
    leds[pixel] = CHSV(random8(), 255, 255); // Acende um pixel aleatorio com cor aleatoria
    FastLED.show();
}

// Forma 2: Cylon/Scanner (luz que vai e volta)
void cylonScanner() {
    static int head = 0;
    static int direction = 1;
    fadeToBlackBy(leds, NUM_LEDS, 60); // Desbota o rastro

    leds[head] = CRGB::Red; // Define a cor da "cabeça"

    head += direction;
    if (head == NUM_LEDS - 1) direction = -1;
    if (head == 0) direction = 1;
    
    FastLED.show();
}

// Forma 3: Efeito de Fogo
void fireEffect() {
    // Array para armazenar o calor de cada pixel
    static byte heat[NUM_LEDS];

    // Passo 1: Resfriar cada pixel um pouco
    for (int i = 0; i < NUM_LEDS; i++) {
        heat[i] = qsub8(heat[i], random8(0, ((NUM_LEDS - i) * 2 / NUM_LEDS) + 2));
    }

    // Passo 2: Faíscas ascendem da base da tira
    if (random8() < 120) { // Ajuste para mais ou menos faíscas
        heat[random8(NUM_LEDS)] = random8(160, 255); // Acende uma faísca em um ponto aleatorio
    }

    // Passo 3: Propagar o calor para cima um pixel por vez
    for (int j = NUM_LEDS - 1; j >= 2; j--) {
        heat[j] = (heat[j - 1] + heat[j - 2] + heat[j - 2]) / 3;
    }

    // Passo 4: Mapear calor para cores
    for (int i = 0; i < NUM_LEDS; i++) {
        leds[i] = HeatColor(heat[i]);
    }

    FastLED.show();
}

// Forma 4: Confetti
void confettiEffect() {
    fadeToBlackBy(leds, NUM_LEDS, 10);
    int pos = random16(NUM_LEDS);
    leds[pos] += CHSV(random8(), 200, 255); // Adiciona uma nova partícula de confetti
    FastLED.show();
}

// Forma 5: Color Wipe (varredura de cor)
void colorWipe() {
    static int pixelIndex = 0;
    static CRGB wipeColor = CRGB::Red;
    static int colorChangeCounter = 0;

    leds[pixelIndex] = wipeColor;
    FastLED.show();

    pixelIndex++;
    if (pixelIndex >= NUM_LEDS) {
        pixelIndex = 0;
        colorChangeCounter++;
        if (colorChangeCounter == 1) wipeColor = CRGB::Green;
        else if (colorChangeCounter == 2) wipeColor = CRGB::Blue;
        else {
            wipeColor = CRGB::Red;
            colorChangeCounter = 0;
        }
    }
}

// Forma 6: Pulso de Brilho
void pulseEffect() {
    // Usa beatsin8 para criar um brilho que pulsa suavemente de 0 ao currentBrightness
    uint8_t pulseBrightness = beatsin8(15, 0, currentBrightness); // 15: velocidade, 0-currentBrightness: range
    FastLED.setBrightness(pulseBrightness);
    fill_solid(leds, NUM_LEDS, colorsPerDay[rtc.now().dayOfTheWeek()]); // Mantém a cor do dia
    FastLED.show();
    // Restaura o brilho total para o modo normal (se voltar)
    // Nao precisamos mais desta linha aqui, pois o runPresentationForm ja seta o brilho.
    // E no fim da animacao de apresentacao, a FastLED.setBrightness(currentBrightness) no switch de modo ja eh chamada.
}
