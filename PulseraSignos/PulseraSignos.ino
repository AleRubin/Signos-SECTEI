#include <lvgl.h>
#include <TFT_eSPI.h>
#include <WiFi.h>
#include <time.h>
#include "lv_conf.h"
#include <CST816S.h>
#include <Wire.h> 
#include "Protocentral_MAX30205.h"

// Para el sensor de ritmo cardíaco MAX30105
#include "MAX30105.h"
#include "heartRate.h"

// Para el sensor de SpO2 (sensor MAX30105/MAX30102)
#include "spo2_algorithm.h" 

CST816S touch(6, 7, 13, 5); // SDA, SCL, RST, IRQ

// --- Configuración WiFi ---
const char* ssid = "RED";
const char* password = "PASS";

// --- Configuración de Pantalla ---
#define SCREEN_WIDTH 240
#define SCREEN_HEIGHT 240

// LVGL Draw Buffer
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[SCREEN_WIDTH * SCREEN_HEIGHT / 10]; 

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

// --- Sensores ---
MAX30205 tempSensor;
MAX30105 particleSensor; // Sensor de ritmo cardíaco
MAX30105 spo2Sensor; // Sensor de SpO2 

// --- Variables para el algoritmo de ritmo cardíaco ---
const byte RATE_SIZE = 4; 
byte rates[RATE_SIZE]; 
byte rateSpot = 0;
long lastBeat = 0; 
float beatsPerMinute;
int beatAvg;

// --- Variables para el algoritmo de SpO2 ---
uint32_t irBuffer[100];
uint32_t redBuffer[100]; 
int32_t bufferLength = 100;
int32_t spo2;
int8_t validSPO2;
int32_t heartRate_spo2;
int8_t validHeartRate_spo2;
byte spo2_sample_count = 0;

// LVGL Labels para mostrar el tiempo
static lv_obj_t *time_label;
static lv_obj_t *date_label;
static lv_obj_t *status_label;
static lv_obj_t *clock_screen;

// --- Pantalla de Temperatura ---
static lv_obj_t *temperature_screen;
static lv_obj_t *temp_label;
static lv_obj_t *measure_btn;

// --- Pantalla de Ritmo Cardíaco ---
static lv_obj_t *heart_rate_screen;
static lv_obj_t *hr_label;
static lv_obj_t *hr_status_label; // Nuevo label para mensajes de estado
static lv_obj_t *hr_measure_btn;

// --- Pantalla de Oximetría (SpO2) ---
static lv_obj_t *spo2_screen;
static lv_obj_t *spo2_label;
static lv_obj_t *spo2_status_label; // Nuevo label para mensajes de estado
static lv_obj_t *spo2_measure_btn;

// Variables para pantalla de carga
static lv_obj_t *loading_screen;
static lv_obj_t *loading_label;
static lv_obj_t *progress_label;
static lv_obj_t *loading_bar;

// --- Estados del programa ---
enum AppState {
    LOADING_WIFI,
    LOADING_NTP,
    CLOCK_SCREEN,
    TEMPERATURE_SCREEN,
    MEASURING_TEMP,
    HEART_RATE_SCREEN,
    MEASURING_HR,
    SPO2_SCREEN,
    MEASURING_SPO2 
};
AppState currentState = LOADING_WIFI;

// --- Variables para el Debounce del Gesto ---
static unsigned long lastSwipeTime = 0;
const unsigned long SWIPE_DEBOUNCE_TIME = 600; // Milisegundos de espera entre gestos

// --- Variables para la medición de temperatura ---
static unsigned long tempMeasurementStartTime = 0;
const unsigned long TEMP_MEASUREMENT_DURATION = 10000; // 10 segundos
static float currentTempReading = 0.0;

// --- Variables para la medición de ritmo cardíaco ---
static unsigned long hrMeasurementStartTime = 0;
const unsigned long HR_MEASUREMENT_DURATION = 10000; // 10 segundos

// --- Variables para la medición de SpO2 ---
static unsigned long spo2MeasurementStartTime = 0;
const unsigned long SPO2_MEASUREMENT_DURATION = 15000; // 15 segundos 

// --- Detección de toque ---
void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data) {
    if (currentState == MEASURING_TEMP || currentState == MEASURING_HR || currentState == MEASURING_SPO2) {
        data->state = LV_INDEV_STATE_REL;
        return;
    }
    
    if (touch.available()) {
        data->state = LV_INDEV_STATE_PR;
        data->point.x = touch.data.x;
        data->point.y = touch.data.y;
        
        unsigned long currentTime = millis();
        String gesture = touch.gesture();

        if (currentTime - lastSwipeTime < SWIPE_DEBOUNCE_TIME) {
            return;
        }

        if (gesture == "SWIPE RIGHT") {
            Serial.println("Gesto: SWIPE_RIGHT detectado");
            lastSwipeTime = currentTime;
            if (currentState == CLOCK_SCREEN) {
                lv_scr_load_anim(temperature_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
                currentState = TEMPERATURE_SCREEN;
            } else if (currentState == TEMPERATURE_SCREEN) {
                lv_scr_load_anim(heart_rate_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
                currentState = HEART_RATE_SCREEN;
            } else if (currentState == HEART_RATE_SCREEN) {
                lv_scr_load_anim(spo2_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
                currentState = SPO2_SCREEN;
            } else if (currentState == SPO2_SCREEN) {
                 lv_scr_load_anim(clock_screen, LV_SCR_LOAD_ANIM_MOVE_LEFT, 500, 0, false);
                currentState = CLOCK_SCREEN;
            }
        } else if (gesture == "SWIPE LEFT") {
            Serial.println("Gesto: SWIPE_LEFT detectado");
            lastSwipeTime = currentTime;
             if (currentState == CLOCK_SCREEN) {
                lv_scr_load_anim(spo2_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
                currentState = SPO2_SCREEN;
            } else if (currentState == SPO2_SCREEN) {
                lv_scr_load_anim(heart_rate_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
                currentState = HEART_RATE_SCREEN;
            } else if (currentState == HEART_RATE_SCREEN) {
                lv_scr_load_anim(temperature_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
                currentState = TEMPERATURE_SCREEN;
            } else if (currentState == TEMPERATURE_SCREEN) {
                lv_scr_load_anim(clock_screen, LV_SCR_LOAD_ANIM_MOVE_RIGHT, 500, 0, false);
                currentState = CLOCK_SCREEN;
            }
        }
    } else {
        data->state = LV_INDEV_STATE_REL;
    }
}

// --- LVGL Display Flush Callback ---
void my_disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p) {
    uint32_t w = (area->x2 - area->x1 + 1);
    uint32_t h = (area->y2 - area->y1 + 1);
    tft.startWrite();
    tft.setAddrWindow(area->x1, area->y1, w, h);
    tft.pushColors((uint16_t *)&color_p->full, w * h, true);
    tft.endWrite();
    lv_disp_flush_ready(disp_drv);
}

// --- LVGL Tick Increment ---
static void lv_tick_task(void *arg) {
    (void) arg;
    lv_tick_inc(1);
}

// --- Evento del botón de temperatura ---
static void measure_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        Serial.println("Botón 'MEASURE' presionado en pantalla de temperatura. Iniciando medición...");
        
        Wire.end(); 
        Wire.begin(21, 18);
        if (!tempSensor.scanAvailableSensors()) {
             Serial.println("ERROR: No se encontró el sensor MAX30205.");
             currentState = TEMPERATURE_SCREEN;
             touch.begin();
             return;
        }
        tempSensor.begin();

        lv_obj_set_style_bg_color(measure_btn, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);

        currentState = MEASURING_TEMP;
        tempMeasurementStartTime = millis();
        lv_label_set_text(temp_label, "Midiento...");
    }
}

// --- Evento del botón de ritmo cardíaco ---
static void hr_measure_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        Serial.println("Botón 'MEASURE' presionado en pantalla de ritmo cardíaco. Iniciando medición...");
        
        Wire.end();
        Wire.begin(21, 18);
        if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) {
            Serial.println("ERROR: No se encontró el sensor MAX30105.");
            currentState = HEART_RATE_SCREEN;
            touch.begin();
            return;
        }

        particleSensor.setup(); 
        particleSensor.setPulseAmplitudeRed(0x0A);
        particleSensor.setPulseAmplitudeGreen(0);

        lv_obj_set_style_bg_color(hr_measure_btn, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(hr_label, "---"); // Borra el valor anterior
        lv_label_set_text(hr_status_label, "Coloca tu dedo");

        currentState = MEASURING_HR;
        hrMeasurementStartTime = millis();
    }
}

// --- Evento del botón de SpO2 ---
static void spo2_measure_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    if(code == LV_EVENT_CLICKED) {
        Serial.println("Botón 'MEASURE' presionado en pantalla de SpO2. Iniciando medición...");
        
        Wire.end();
        Wire.begin(21, 18);
        if (!spo2Sensor.begin(Wire, I2C_SPEED_FAST)) {
            Serial.println("ERROR: No se encontró el sensor MAX30105 para SpO2.");
            currentState = SPO2_SCREEN;
            touch.begin();
            return;
        }

        // Configuración para SpO2
        byte ledBrightness = 60;
        byte sampleAverage = 4;
        byte ledMode = 2; 
        byte sampleRate = 100;
        int pulseWidth = 411;
        int adcRange = 4096;

        spo2Sensor.setup(ledBrightness, sampleAverage, ledMode, sampleRate, pulseWidth, adcRange);

        lv_obj_set_style_bg_color(spo2_measure_btn, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_label_set_text(spo2_label, "---"); 
        lv_label_set_text(spo2_status_label, "Coloca tu dedo");

        // Inicializar variables para SpO2
        spo2_sample_count = 0;
        spo2 = 0;
        validSPO2 = 0;

        currentState = MEASURING_SPO2;
        spo2MeasurementStartTime = millis();

        Serial.println("Recolectando datos iniciales para SpO2...");
    }
}

// --- Crear pantalla de carga ---
void createLoadingScreen() {
    loading_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(loading_screen, lv_color_hex(0x001122), LV_PART_MAIN);

    lv_obj_t *app_title = lv_label_create(loading_screen);
    lv_label_set_text(app_title, "DIGITAL CLOCK");
    lv_obj_set_style_text_font(app_title, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(app_title, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(app_title, LV_ALIGN_CENTER, 0, -40);

    loading_bar = lv_bar_create(loading_screen);
    lv_obj_set_size(loading_bar, 180, 8);
    lv_obj_set_style_bg_color(loading_bar, lv_color_make(40, 40, 40), LV_PART_MAIN);
    lv_obj_set_style_bg_color(loading_bar, lv_color_make(0, 150, 255), LV_PART_INDICATOR);
    lv_obj_align(loading_bar, LV_ALIGN_CENTER, 0, 0);
    lv_bar_set_value(loading_bar, 0, LV_ANIM_OFF);

    progress_label = lv_label_create(loading_screen);
    lv_label_set_text(progress_label, "0%");
    lv_obj_set_style_text_font(progress_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(progress_label, lv_color_make(0, 150, 255), LV_PART_MAIN);
    lv_obj_align(progress_label, LV_ALIGN_CENTER, 0, 20);

    loading_label = lv_label_create(loading_screen);
    lv_label_set_text(loading_label, "Iniciando sistema...");
    lv_obj_set_style_text_font(loading_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(loading_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(loading_label, LV_ALIGN_CENTER, 0, 45);

    lv_scr_load(loading_screen);
}

// --- Crear pantalla principal del reloj ---
void createClockScreen() {
    clock_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(clock_screen, lv_color_hex(0x000000), LV_PART_MAIN);

    time_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_font(time_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(time_label, lv_color_white(), LV_PART_MAIN);
    lv_label_set_text(time_label, "--:--:--");
    lv_obj_align(time_label, LV_ALIGN_CENTER, 0, -10); 

    date_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_font(date_label, &lv_font_montserrat_24, LV_PART_MAIN);
    lv_obj_set_style_text_color(date_label, lv_color_make(150, 150, 150), LV_PART_MAIN);
    lv_label_set_text(date_label, "--/--/----");
    lv_obj_align(date_label, LV_ALIGN_CENTER, 0, 30);

    status_label = lv_label_create(clock_screen);
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(status_label, lv_color_make(100, 200, 255), LV_PART_MAIN);
    lv_label_set_text(status_label, "Sincronizado");
    lv_obj_align(status_label, LV_ALIGN_BOTTOM_MID, 0, -10);
}

// --- Crear la pantalla de temperatura ---
void createTemperatureScreen() {
    temperature_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(temperature_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t *title_label = lv_label_create(temperature_screen);
    lv_label_set_text(title_label, "Wrist Skin Temperature");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

    temp_label = lv_label_create(temperature_screen);
    lv_label_set_text(temp_label, "---"); // Valor inicial
    lv_obj_set_style_text_font(temp_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(temp_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(temp_label, LV_ALIGN_CENTER, 0, -10);

    measure_btn = lv_btn_create(temperature_screen);
    lv_obj_set_size(measure_btn, 120, 35);
    lv_obj_align(measure_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(measure_btn, lv_color_hex(0x0066FF), LV_PART_MAIN | LV_STATE_DEFAULT); 
    lv_obj_add_event_cb(measure_btn, measure_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *btn_label = lv_label_create(measure_btn);
    lv_label_set_text(btn_label, "MEASURE");
    lv_obj_set_style_text_color(btn_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(btn_label);
}

// --- Crear la pantalla de ritmo cardíaco ---
void createHeartRateScreen() {
    heart_rate_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(heart_rate_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t *title_label = lv_label_create(heart_rate_screen);
    lv_label_set_text(title_label, "Heart Rate");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

    hr_label = lv_label_create(heart_rate_screen);
    lv_label_set_text(hr_label, "---");
    lv_obj_set_style_text_font(hr_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(hr_label, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Color naranja
    lv_obj_align(hr_label, LV_ALIGN_CENTER, 0, -10);

    hr_status_label = lv_label_create(heart_rate_screen);
    lv_label_set_text(hr_status_label, "");
    lv_obj_set_style_text_font(hr_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(hr_status_label, lv_color_make(150, 150, 150), LV_PART_MAIN);
    lv_obj_align_to(hr_status_label, hr_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    hr_measure_btn = lv_btn_create(heart_rate_screen);
    lv_obj_set_size(hr_measure_btn, 120, 35);
    lv_obj_align(hr_measure_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(hr_measure_btn, lv_color_hex(0xFF8C00), LV_PART_MAIN); // Color naranja
    lv_obj_add_event_cb(hr_measure_btn, hr_measure_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *hr_btn_label = lv_label_create(hr_measure_btn);
    lv_label_set_text(hr_btn_label, "MEASURE");
    lv_obj_set_style_text_color(hr_btn_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(hr_btn_label);
}

// --- Crear la pantalla de SpO2 (Oximetría) ---
void createSpO2Screen() {
    spo2_screen = lv_obj_create(NULL);
    lv_obj_set_style_bg_color(spo2_screen, lv_color_hex(0x000000), LV_PART_MAIN);
    
    lv_obj_t *title_label = lv_label_create(spo2_screen);
    lv_label_set_text(title_label, "Blood Oxygen (SpO2)");
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(title_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_align(title_label, LV_ALIGN_TOP_MID, 0, 40);

    spo2_label = lv_label_create(spo2_screen);
    lv_label_set_text(spo2_label, "---");
    lv_obj_set_style_text_font(spo2_label, &lv_font_montserrat_48, LV_PART_MAIN);
    lv_obj_set_style_text_color(spo2_label, lv_color_hex(0x1a9e52), LV_PART_MAIN); // Color verde
    lv_obj_align(spo2_label, LV_ALIGN_CENTER, 0, -10);

    spo2_status_label = lv_label_create(spo2_screen);
    lv_label_set_text(spo2_status_label, "");
    lv_obj_set_style_text_font(spo2_status_label, &lv_font_montserrat_14, LV_PART_MAIN);
    lv_obj_set_style_text_color(spo2_status_label, lv_color_make(150, 150, 150), LV_PART_MAIN);
    lv_obj_align_to(spo2_status_label, spo2_label, LV_ALIGN_OUT_BOTTOM_MID, 0, 0);

    spo2_measure_btn = lv_btn_create(spo2_screen);
    lv_obj_set_size(spo2_measure_btn, 120, 35);
    lv_obj_align(spo2_measure_btn, LV_ALIGN_CENTER, 0, 60);
    lv_obj_set_style_bg_color(spo2_measure_btn, lv_color_hex(0x1a9e52), LV_PART_MAIN); // Color verde para el botón
    lv_obj_add_event_cb(spo2_measure_btn, spo2_measure_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t *spo2_btn_label = lv_label_create(spo2_measure_btn);
    lv_label_set_text(spo2_btn_label, "MEASURE");
    lv_obj_set_style_text_color(spo2_btn_label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(spo2_btn_label);
}

// --- Actualizar tiempo desde sistema ---
void updateTimeFromSystem() {
    static unsigned long last_update = 0;
    unsigned long now_ms = millis();

    if (now_ms - last_update >= 1000) {
        last_update = now_ms;
        
        time_t rawtime;
        struct tm * timeinfo;
        
        time(&rawtime);
        timeinfo = localtime(&rawtime);
        
        char time_str[9];
        sprintf(time_str, "%02d:%02d:%02d", 
                timeinfo->tm_hour, 
                timeinfo->tm_min, 
                timeinfo->tm_sec);
        lv_label_set_text(time_label, time_str);

        char date_str[11];
        sprintf(date_str, "%02d/%02d/%04d", 
                timeinfo->tm_mday, 
                timeinfo->tm_mon + 1, 
                timeinfo->tm_year + 1900);
        lv_label_set_text(date_label, date_str);
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando reloj con NTP...");

    tft.init();
    tft.setRotation(0);
    tft.fillScreen(TFT_BLACK);

    lv_init();
    lv_disp_draw_buf_init(&draw_buf, buf, NULL, SCREEN_WIDTH * SCREEN_HEIGHT / 10);
    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = SCREEN_WIDTH;
    disp_drv.ver_res = SCREEN_HEIGHT;
    disp_drv.flush_cb = my_disp_flush;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    touch.begin();
    
    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    lv_indev_drv_register(&indev_drv);

    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &lv_tick_task,
        .name = "lv_tick_timer"
    };
    esp_timer_handle_t periodic_timer;
    esp_timer_create(&periodic_timer_args, &periodic_timer);
    esp_timer_start_periodic(periodic_timer, 1000);

    createLoadingScreen();
    createClockScreen();
    createTemperatureScreen();
    createHeartRateScreen();
    createSpO2Screen();
    
    WiFi.begin(ssid, password);
}

void loop() {
    lv_timer_handler();
    
    static unsigned long last_wifi_check = 0;
    unsigned long now_ms = millis();

    switch (currentState) {
        case LOADING_WIFI:
            if (now_ms - last_wifi_check > 500) {
                last_wifi_check = now_ms;
                
                int progress = map(WiFi.status(), WL_IDLE_STATUS, WL_CONNECTED, 0, 50);
                if (WiFi.status() == WL_CONNECTED) {
                    lv_bar_set_value(loading_bar, 50, LV_ANIM_ON);
                    lv_label_set_text(progress_label, "50%");
                    lv_label_set_text(loading_label, "WiFi Conectado! \nSincronizando NTP...");
                    
                    configTime(-6 * 3600, 0, "pool.ntp.org", "time.nist.gov");
                    currentState = LOADING_NTP;
                } else {
                    lv_bar_set_value(loading_bar, progress, LV_ANIM_OFF);
                    char progress_str[8];
                    sprintf(progress_str, "%d%%", progress);
                    lv_label_set_text(progress_label, progress_str);
                }
            }
            break;
            
        case LOADING_NTP:
            if (now_ms - last_wifi_check > 1000) {
                last_wifi_check = now_ms;
                
                time_t now_ntp;
                time(&now_ntp);
                struct tm timeinfo;
                localtime_r(&now_ntp, &timeinfo);
                
                if (timeinfo.tm_year > (2016 - 1900)) {
                    lv_bar_set_value(loading_bar, 100, LV_ANIM_ON);
                    lv_label_set_text(progress_label, "100%");
                    lv_label_set_text(loading_label, "Sincronización completa");
                    
                    lv_scr_load_anim(clock_screen, LV_SCR_LOAD_ANIM_FADE_IN, 500, 0, false);
                    currentState = CLOCK_SCREEN;
                }
            }
            break;
            
        case CLOCK_SCREEN:
            updateTimeFromSystem();
            break;
        
        case TEMPERATURE_SCREEN:
            break;
        
        case MEASURING_TEMP:
            if (now_ms - tempMeasurementStartTime < TEMP_MEASUREMENT_DURATION) {
                currentTempReading = tempSensor.getTemperature();
                char temp_str[10];
                sprintf(temp_str, "%.2f'C", currentTempReading);
                lv_label_set_text(temp_label, temp_str);
                Serial.print("Temperatura midiendo: ");
                Serial.println(currentTempReading);
            } else {
                Serial.println("Medición de temperatura completada.");
                char temp_str[10];
                sprintf(temp_str, "%.2f'C", currentTempReading);
                lv_label_set_text(temp_label, temp_str);
                
                Wire.end();
                touch.begin();
                
                lv_obj_set_style_bg_color(measure_btn, lv_color_hex(0x0066FF), LV_PART_MAIN | LV_STATE_DEFAULT);
                
                currentState = TEMPERATURE_SCREEN;
            }
            break;

        case HEART_RATE_SCREEN:
            break;

        case MEASURING_HR:
            if (now_ms - hrMeasurementStartTime < HR_MEASUREMENT_DURATION) {
                long irValue = particleSensor.getIR();

                if (checkForBeat(irValue) == true) {
                    long delta = millis() - lastBeat;
                    lastBeat = millis();

                    beatsPerMinute = 60 / (delta / 1000.0);

                    if (beatsPerMinute < 255 && beatsPerMinute > 20) {
                        rates[rateSpot++] = (byte)beatsPerMinute; 
                        rateSpot %= RATE_SIZE;

                        beatAvg = 0;
                        for (byte x = 0; x < RATE_SIZE; x++)
                            beatAvg += rates[x];
                        beatAvg /= RATE_SIZE;
                    }
                }
                
                if (irValue < 50000) {
                    lv_label_set_text(hr_status_label, "Coloca tu dedo");
                } else {
                    lv_label_set_text(hr_status_label, "Midiento...");
                    char hr_str[10];
                    sprintf(hr_str, "%d", beatAvg);
                    lv_label_set_text(hr_label, hr_str);
                }
                
            } else {
                Serial.println("Medición de ritmo cardíaco completada.");
                
                Wire.end();
                touch.begin();
                
                lv_obj_set_style_bg_color(hr_measure_btn, lv_color_hex(0xFF8C00), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(hr_status_label, "Finalizado");
                
                currentState = HEART_RATE_SCREEN;
            }
            break;

        case SPO2_SCREEN:
            break;

        case MEASURING_SPO2:
            if (now_ms - spo2MeasurementStartTime < SPO2_MEASUREMENT_DURATION) {
                while (spo2Sensor.available() == false) {
                    spo2Sensor.check();
                }
                
                // Recolectar las primeras 100 muestras
                if (spo2_sample_count < 100) {
                    redBuffer[spo2_sample_count] = spo2Sensor.getRed();
                    irBuffer[spo2_sample_count] = spo2Sensor.getIR();
                    spo2Sensor.nextSample();
                    spo2_sample_count++;

                    char progress_str[20];
                    sprintf(progress_str, "Recolectando %d%%", (spo2_sample_count * 100) / 100);
                    lv_label_set_text(spo2_status_label, progress_str);
                    
                    // Completas las 100 muestras iniciales, calcular SpO2
                    if (spo2_sample_count == 100) {
                        maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate_spo2, &validHeartRate_spo2);
                        Serial.println("Cálculo inicial de SpO2 completado.");
                    }
                } else {
                    for (byte i = 25; i < 100; i++) {
                        redBuffer[i - 25] = redBuffer[i];
                        irBuffer[i - 25] = irBuffer[i];
                    }
                    for (byte i = 75; i < 100; i++) {
                        while (spo2Sensor.available() == false) {
                            spo2Sensor.check();
                        }
                        redBuffer[i] = spo2Sensor.getRed();
                        irBuffer[i] = spo2Sensor.getIR();
                        spo2Sensor.nextSample();
                    }

                    // Recalcular HR y SpO2 con el nuevo buffer
                    maxim_heart_rate_and_oxygen_saturation(irBuffer, bufferLength, redBuffer, &spo2, &validSPO2, &heartRate_spo2, &validHeartRate_spo2);
                    
                    // Mostrar resultados si son válidos
                    if (validSPO2 == 1 && spo2 > 0) {
                        char spo2_str[10];
                        sprintf(spo2_str, "%d%%", spo2);
                        lv_label_set_text(spo2_label, spo2_str);
                        lv_label_set_text(spo2_status_label, "Midiendo...");
                    } else {
                        lv_label_set_text(spo2_status_label, "Ajusta posicion");
                    }
                    
                    Serial.print("SpO2=");
                    Serial.print(spo2, DEC);
                    Serial.print(", SpO2Valid=");
                    Serial.println(validSPO2, DEC);
                }
                
            } else {
                Serial.println("Medición de SpO2 completada.");
                
                // Mostrar resultado final
                if (validSPO2 == 1 && spo2 > 0) {
                    char spo2_str[10];
                    sprintf(spo2_str, "%d%%", spo2);
                    lv_label_set_text(spo2_label, spo2_str);
                } else {
                    lv_label_set_text(spo2_label, "Error");
                }
                
                Wire.end();
                touch.begin();
                
                lv_obj_set_style_bg_color(spo2_measure_btn, lv_color_hex(0x1a9e52), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_label_set_text(spo2_status_label, "Finalizado");
                
                currentState = SPO2_SCREEN;
            }
            break;
    }
    
    delay(5);
}