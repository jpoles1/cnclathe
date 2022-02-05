#include <TFT_eSPI.h>
#include <list>
#include <Font5x7FixedMono.h>

#define SCREEN_WIDTH 135
#define SCREEN_HEIGHT 240
#define SCREEN_MIDX 120
#define SCREEN_MIDY 67

class InfoBlock {
        public:
            int x, y, w, h, font_size;
            GFXfont font;
            InfoBlock(int x, int y, int w=SCREEN_WIDTH, int h=SCREEN_HEIGHT,int font_size=2);
            void draw(char* print_str);
};



class UI {
    public:
        InfoBlock rpm_block  = InfoBlock(0, 30, SCREEN_WIDTH, 30);
        void init();
        void draw_rpm(int rpm);
        void draw();
};
