#include "ui.h"

TFT_eSPI tft = TFT_eSPI(SCREEN_WIDTH, SCREEN_HEIGHT);

InfoBlock::InfoBlock(int x, int y, int w, int h, int font_size) {
    this->x = x;
    this->y = y;
    this->w = w;
    this->h = h;
    this->font_size = font_size;
};

void InfoBlock::draw(char* print_str) {
    tft.fillRect(this->x, this->y, this->w, this->h, TFT_RED);
    tft.setFreeFont(&Font5x7FixedMono);
    tft.setTextSize(this->font_size);
    tft.setTextDatum(TC_DATUM);
    tft.drawString(String(print_str), this->x, this->y);
};


void UI::init() {
    tft.init();
    tft.setRotation(2);
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&Font5x7FixedMono);
    tft.setTextSize(2);
}
void UI::draw_rpm(int rpm) {
    char print_str[256];
    sprintf(print_str, "RPM: %d", rpm);
    rpm_block.draw(print_str);
}
void UI::draw() {
    tft.fillScreen(TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextDatum(MC_DATUM);
    tft.setFreeFont(&FreeMonoBold9pt7b);
    tft.setTextSize(1);
    tft.drawString("JCNC Lathe", SCREEN_MIDX, 4);
    tft.fillRect(0, 17, SCREEN_WIDTH, 2, TFT_GREEN);
    draw_rpm(0);
};


    /*void draw_estop() {
        uint flash_interval = 1000;
        bool flash_phase = (millis() % flash_interval) > (flash_interval / 2);
        if(flash_phase != last_flash_phase) {
            if(flash_phase) {
                tft.fillScreen(TFT_RED);
                tft.setTextColor(TFT_BLACK);    
            } else {
                tft.fillScreen(TFT_BLACK);
                tft.setTextColor(TFT_RED);
            }
            tft.setFreeFont(&Font5x7FixedMono);
            tft.setTextSize(3);
            tft.drawString("JCNC", SCREEN_MIDX, SCREEN_MIDY - 25);
            tft.drawString("LOCKED", SCREEN_MIDX, SCREEN_MIDY + 20);
            last_flash_phase = flash_phase;    
        }
        
    }*/