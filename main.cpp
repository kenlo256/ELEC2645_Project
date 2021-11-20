#include <array>
#include <cstdint>
#include <string>
#include <utility>

#include "CollisionEngine.h"
#include "Joystick.h"
#include "N5110.h"
#include "mbed.h"

#define MAXNUMPARTICLE 100
#define PI 3.14159265
// VCC,SCE,RST,D/C,MOSI,SCLK,LED
N5110 lcd(p14, p8, p9, p10, p11, p13, p21);

Joystick joystick(p20, p19);
BusOut rgb_led(p24, p23, p22);
InterruptIn button_D(p26);
InterruptIn button_B(p28);
Ticker timer;
Ticker gameTimer;
Ticker randTimer;
Ticker panicTimer;

void init_display();
void timer_isr();
void game_diff_timer_isr();
void rand_timer_isr();
void panic_timer_isr();
void printStartScreenInfo();
void mainCharController(uint8_t x0, uint8_t y0, float rotation);
void GameOverDisplay();
void refresh_screen();
void showParticles();
void checkMainCharDesPar();
void GameStartISR(uint8_t numParIdx);
void reset_led();
void initParticlesInfo();
void button_B_isr();

enum class Act {
    PANICMODE,
    CLICK_B,
};

typedef struct LEDValAction {
    int ledVal;
    Act action;
} LEDVal;

static constexpr LEDVal LEDAction[] = {
    {5, Act::PANICMODE},  // Green
    {6, Act::CLICK_B},    // Red
};

Particle* init_particle();

float fps = 18;
float dt = 1 / fps;
volatile int g_button_D_flag = 0;
volatile int g_button_B_flag = 0;
volatile int timer_flag = 0;
volatile int game_diff_timer_flag = 0;
volatile int rand_timer_flag = 0;
volatile int panic_timer_flag = 0;
uint64_t score;

Particle mainChar;
Box* b;

void button_D_isr();

void init_main_char() {
    // max vlaue of the uint8_t reserved for main char
    mainChar.id = 225;
    mainChar.r = 3;
    mainChar.x = 47;
    mainChar.y = 23;
}

void mainCharBoundary() {
    if (mainChar.x + mainChar.r > 83) {
        mainChar.x = 83 - mainChar.r - 1;
    }

    if (mainChar.r > mainChar.x) {
        mainChar.x = mainChar.r + 1;
    }

    if (mainChar.r > mainChar.y) {
        mainChar.y = mainChar.r + 1;
    }

    if (mainChar.y + mainChar.r > 47) {
        mainChar.y = 47 - mainChar.r - 1;
    }
}

void mainCharController(uint8_t numParIdx) {
    float mag = joystick.get_mag();
    float rotation = joystick.get_angle() * (PI / 180);
    const static uint8_t radius = 3;

    mainChar.x += sin(rotation) * (mag * numParIdx * 0.3);
    mainChar.y -= cos(rotation) * (mag * numParIdx * 0.3);
    mainCharBoundary();
    
    lcd.drawMainCharCircle(mainChar.x, mainChar.y, radius, rotation);
}

int main() {
    l(2.0);
    
    init_display();
    printStartScreenInfo();
    b = getBox();
    initParticlesInfo();
    init_main_char();
    joystick.init();

    button_D.rise(&button_D_isr);
    button_D.mode(PullNone);

    button_B.rise(&button_B_isr);
    button_B.mode(PullNone);

    srand(customRand(5));

    timer.attach(&timer_isr, dt);
    gameTimer.attach(&game_diff_timer_isr, 30);
    randTimer.attach(&rand_timer_isr, (int)(rand() % 20 + 1));

    uint8_t numParIdx = b->n;
    uint8_t incPar = 2;
    bool ggFlag = false;

    while (!ggFlag) {
        if (timer_flag && g_button_D_flag) {
            timer_flag = 0;

            if (rand_timer_flag) {
                rand_timer_flag = 0;
                Act action = static_cast<Act>(customRand(1) % 2);
                switch (action) {
                    case Act::PANICMODE: {
                        rgb_led.write(LEDAction[0].ledVal);
                        panic_timer_flag = 1;
                        panicTimer.attach(&panic_timer_isr, 8);
                        lcd.inverseMode();
                        break;
                    }
                    case Act::CLICK_B: {
                        rgb_led.write(LEDAction[1].ledVal);
                        Timer t;
                        t.start();

                        bool hasClicked = false;

                        while (t.read() <= 500e-03) {
                            if (g_button_B_flag) {
                                g_button_B_flag = 0;
                                mainCharDesPar(mainChar);
                                score++;
                                hasClicked = true;
                                reset_led();
                                break;
                            }
                        }

                        t.stop();

                        if (!hasClicked) {
                            reset_led();
                            ggFlag = true;
                        }
                        break;
                    }
                }
                randTimer.attach(&rand_timer_isr, (int)(rand() % 20 + 1));
            }

            if (panic_timer_flag && mainCharCollidePar(mainChar)) ggFlag = true;

            if (!panic_timer_flag) {
                mainCharDesPar(mainChar);
                score++;
                panicTimer.detach();
                lcd.normalMode();
                reset_led();
            }

            refresh_param();
            showParticles();
            mainCharController(numParIdx);
            lcd.refresh();

            if (game_diff_timer_flag && b->n != 46) {
                game_diff_timer_flag = 0;
                numParIdx += incPar;
                incPar++;
                GameStartISR(numParIdx);
            }
        }
        sleep();
    }
    GameOverDisplay();
}

void printStartScreenInfo() {
    char buffer1[14];
    char buffer2[14];
    sprintf(buffer1, "DesPar.io");
    sprintf(buffer2, "Start:Press D");
    lcd.printString(buffer1, 15, 0);
    lcd.printString(buffer2, 0, 4);
    lcd.refresh();
}

void init_display() {
    lcd.init();
    lcd.setContrast(0.5);
}

void init_score() { score = 0; }

void button_B_isr() { g_button_B_flag = 1; }
void button_D_isr() { g_button_D_flag = 1; }
void timer_isr() { timer_flag = 1; }
void rand_timer_isr() { rand_timer_flag = 1; }

void game_diff_timer_isr() { game_diff_timer_flag = 1; }

void panic_timer_isr() { panic_timer_flag = 0; }

void showParticles() {
    lcd.clear();

    for (Particle p : b->pl) lcd.drawCircle(p.x, p.y, p.r, FILL_BLACK);

    lcd.refresh();
}

void reset_led() { rgb_led.write(15); }
void GameStartISR(uint8_t numParIdx) {
    for (uint8_t i = b->n; i < numParIdx; i++) {
        spawnParticle(i, mainChar);
        if (b->n < MAXNUMPARTICLE) b->n++;
    }
}

void GameOverDisplay() {
    lcd.clear();
    char buffer1[14];
    char buffer2[14];
    sprintf(buffer1, "Game Over");
    sprintf(buffer2, "Score: %lli", score);
    lcd.printString(buffer1, 15, 0);
    lcd.printString(buffer2, 0, 4);
    lcd.refresh();
}

void initParticlesInfo() {
    b->n = 3;

    for (uint8_t i = 0; i < b->n; i++) spawnParticle(i, mainChar);
}