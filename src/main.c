#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

#ifdef __WIN64
#define SDL_MAIN_HANDLED
#endif
#include <SDL2/SDL.h>

typedef struct vec3_s { double x, y, z; } vec3;
typedef struct vert_s { vec3 a, b, c; } vert;

#define SCREEN_WIDTH  320 // 1440// 1600// 800
#define SCREEN_HEIGHT 240 // 1080// 900// 450

#define WINDOW_WIDTH  640 // 1440// 1600// 800 // 1920 // 640
#define WINDOW_HEIGHT 480 // 1080// 900// 450 // 1080 // 480

vert test_vt = {{-1, 0.5, 1},
                {-1, 0.5, -1},
                {1, 0.5, 0}};

vert test_vt2 = {{-0.784, 0.25, 0.453},
                {-0.656, 0.25, -0.733},
                {0.826, 0.25, 0.234}};

//vert test_vt = {{0, 0.5, 0},
//                {1, 0.5, 0.5},
//                {0, 0.5, 1}};

uint32_t voidColor = 0x000000FF;
uint32_t color0 = 0x222222FF;
//uint32_t skyColor = 0x110033FF;
uint32_t floorColor = 0x222222FF;
//uint32_t color1 = 0x990044FF;
//uint32_t color2 = 0x004499FF;
//uint32_t color3 = 0x994400FF;

uint32_t pixels[SCREEN_WIDTH * SCREEN_HEIGHT];

SDL_Window *window;
SDL_Texture *texture;
SDL_Renderer *renderer;

bool quit = false;

struct {
    clock_t time;
    double delta;
    double fpsCap;
} state;

struct {
    vec3 pos;
    vec3 rot;

    double rSpeed;
    double pSin;
    double pCos;
    double nSin;
    double nCos;
} player;

struct {
    double fov; //in radians rn
    double plaDist; //plane dist from player
    double plaW; //plane width
} camera;


uint32_t setColor(uint8_t r, uint8_t g, uint8_t b, uint8_t a) {
    uint32_t col = r;
    col = (col << 8) + g;
    col = (col << 8) + b;
    col = (col << 8) + a;
    return col;
}

double getDist(double x1, double y1, double x2, double y2) {
    return sqrtf(( powf(x2 - x1, 2) ) + ( powf(y2 - y1, 2) ));
}

//double getDist(double x1, double y1, double x2, double y2) {
//    return sqrtf(( (x2 - x1) * (x2 - x1) ) + ( (y2 - y1) * y2 - y1 ));
//}

int getSign(double num) {
    return (num > 0 ? 1 : (num < 0 ? -1 : 0));
}

void zeroOut(double* pVal, double step) {
    if (*pVal == 0) {return;}
    double oldVal = *pVal;
    step *= getSign(*pVal) * -1;
    *pVal += step;
    if (getSign(*pVal) != getSign(oldVal)) {*pVal = 0;}
}


void setPos(double x, double y, double z) {
    player.pos.x = x;
    player.pos.y = y;
    player.pos.z = z;
}


void drawCrosshair() {
    pixels[(SCREEN_WIDTH / 2) + (SCREEN_WIDTH * (SCREEN_HEIGHT / 2))] = 0xFFFFFFFF;
}

void fillScreen(uint32_t col) {
    for (int i = 0; i < (SCREEN_HEIGHT * SCREEN_WIDTH); i += 1) {
        pixels[i] = col;
    }
}

void drawPoint2D(double x, double z) {
    /*
    Take point from (-1,-1) to (1,1) and draw to pixel buffer
    */
    pixels[ 
        (int)((x + 1) * 0.5 * (SCREEN_WIDTH - 1)) + 
        (int)((z + 1) * 0.5 * (SCREEN_HEIGHT - 1)) * 
        SCREEN_WIDTH] = 0xFFFFFFFF;
}

void drawLine2DW(double x1, double z1, double x2, double z2) {
    
    int screenX1 = (int)((x1 + 1) * 0.5 * (SCREEN_WIDTH - 1));
    int screenZ1 = (int)((z1 + 1) * 0.5 * (SCREEN_HEIGHT - 1));
    int screenX2 = (int)((x2 + 1) * 0.5 * (SCREEN_WIDTH - 1));
    int screenZ2 = (int)((z2 + 1) * 0.5 * (SCREEN_HEIGHT - 1));
    
    int x = screenX2 - screenX1;
    int z = screenZ2 - screenZ1;

    double step;
    if (fabs(x) >= fabs(z)) {
        step = (double)z / x;

        if (getSign(x) == 1) {
            for (int i = 0; i < x; i += 1) {
                pixels[ (screenX1 + i) + (int)(screenZ1 + (step * i)) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
        else {
            for (int i = 0; i > x; i -= 1) {
                pixels[ (screenX1 + i) + (int)(screenZ1 + (step * i)) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }  
    }
    else {
        step = (double)x / z;

        if (getSign(z) == 1) {
            for (int i = 0; i < z; i += 1) {
                pixels[ (int)(screenX1 + (step * i)) + (screenZ1 + i) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
        else {
            for (int i = 0; i > z; i -= 1) {
                pixels[ (int)(screenX1 + (step * i)) + (screenZ1 + i) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
    }
}

void drawLine2D(double x1, double z1, double x2, double z2) {
    
    double screenX1 = ((x1 + 1) * 0.5 * (SCREEN_WIDTH - 1));
    double screenZ1 = ((z1 + 1) * 0.5 * (SCREEN_HEIGHT - 1));
    double screenX2 = ((x2 + 1) * 0.5 * (SCREEN_WIDTH - 1));
    double screenZ2 = ((z2 + 1) * 0.5 * (SCREEN_HEIGHT - 1));
    
    double x = screenX2 - screenX1;
    double z = screenZ2 - screenZ1;

    double step;
    if (fabs(x) >= fabs(z)) {
        step = z / x;

        if (getSign(x) == 1) {
            for (int i = 0; i < x; i += 1) {
                pixels[ (int)(screenX1 + i) + (int)(screenZ1 + (step * i)) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
        else {
            for (int i = 0; i > x; i -= 1) {
                pixels[ (int)(screenX1 + i) + (int)(screenZ1 + (step * i)) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }  
    }
    else {
        step = x / z;

        if (getSign(z) == 1) {
            for (int i = 0; i < z; i += 1) {
                pixels[ (int)(screenX1 + (step * i)) + (int)(screenZ1 + i) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
        else {
            for (int i = 0; i > z; i -= 1) {
                pixels[ (int)(screenX1 + (step * i)) + (int)(screenZ1 + i) * SCREEN_WIDTH] = 0xFFFFFFFF;
            }
        }
    }
}


int main(int argc, char const *argv[]) {
    printf("%s\n", ":)");

    if(SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "SDL failed to initialize: %s\n", SDL_GetError());
    }

    window = SDL_CreateWindow(
        "raster", 
        SDL_WINDOWPOS_CENTERED_DISPLAY(0), 
        SDL_WINDOWPOS_CENTERED_DISPLAY(0), 
        WINDOW_WIDTH, 
        WINDOW_HEIGHT,
        SDL_WINDOW_ALLOW_HIGHDPI); // | SDL_WINDOW_FULLSCREEN
    if(!window) {
        fprintf(stderr, "failed to create SDL window: %s\n", SDL_GetError());
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_PRESENTVSYNC); //SDL_RENDERER_PRESENTVSYNC
    if(!renderer) {
        fprintf(stderr, "failed to create SDL renderer: %s\n", SDL_GetError());
    }

    texture = SDL_CreateTexture(
            renderer,
            SDL_PIXELFORMAT_RGBA8888,
            SDL_TEXTUREACCESS_STREAMING,
            SCREEN_WIDTH,
            SCREEN_HEIGHT);
    if(!texture) {
        fprintf(stderr, "failed to create SDL texture: %s\n", SDL_GetError());
    }
    

    //SDL_SetRelativeMouseMode(true);
    //event.motion.xrel
    fillScreen(0x222222FF);
    drawCrosshair();

    while (!quit){
        
        //while (clock() - state.time < state.fpsCap) {}
        //printf("%ld\n", clock() - state.time);
        state.delta = (double)(clock() - state.time) / CLOCKS_PER_SEC; //1000000
        state.time = clock();
        
        printf("%f\n", 1/state.delta);
        //printf("%ld\n", state.time - state.oldTime);

        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                quit = true;
            }
        }

        const uint8_t *keystate = SDL_GetKeyboardState(NULL);

        if (keystate[SDL_SCANCODE_P]) {
            quit = true;
        }

        //memset(pixels, 0, sizeof(pixels));    
        fillScreen(0x222222FF);

        //pixels[ (1 - 1) + (1 - 1) * SCREEN_WIDTH] = 0xFFFFFFFF;
        //pixels[ (320 - 1) + (240 - 1) * SCREEN_WIDTH] = 0xFFFFFFFF;
        //pixels[ (int)(1 * (SCREEN_WIDTH - 1)) + (int)(1 * (SCREEN_HEIGHT - 1)) * SCREEN_WIDTH] = 0xFFFFFFFF;
        //drawPoint2D(test_vt.a.x, test_vt.a.z);
        //drawPoint2D(test_vt.b.x, test_vt.b.z);
        //drawPoint2D(test_vt.c.x, test_vt.c.z);
        //drawLine2DW(test_vt.a.x, test_vt.a.z, test_vt.b.x, test_vt.b.z);
        //drawLine2DW(test_vt.b.x, test_vt.b.z, test_vt.c.x, test_vt.c.z);
        //drawLine2DW(test_vt.c.x, test_vt.c.z, test_vt.a.x, test_vt.a.z);
        drawLine2D(test_vt.a.x, test_vt.a.z, test_vt.b.x, test_vt.b.z);
        drawLine2D(test_vt.b.x, test_vt.b.z, test_vt.c.x, test_vt.c.z);
        drawLine2D(test_vt.c.x, test_vt.c.z, test_vt.a.x, test_vt.a.z);

        //drawPoint2D(test_vt2.a.x, test_vt2.a.z);
        //drawPoint2D(test_vt2.b.x, test_vt2.b.z);
        //drawPoint2D(test_vt2.c.x, test_vt2.c.z);
        drawLine2DW(test_vt2.a.x, test_vt2.a.z, test_vt2.b.x, test_vt2.b.z);
        drawLine2DW(test_vt2.b.x, test_vt2.b.z, test_vt2.c.x, test_vt2.c.z);
        drawLine2DW(test_vt2.c.x, test_vt2.c.z, test_vt2.a.x, test_vt2.a.z);
        drawLine2D(test_vt2.a.x, test_vt2.a.z, test_vt2.b.x, test_vt2.b.z);
        drawLine2D(test_vt2.b.x, test_vt2.b.z, test_vt2.c.x, test_vt2.c.z);
        drawLine2D(test_vt2.c.x, test_vt2.c.z, test_vt2.a.x, test_vt2.a.z);

        test_vt2.c.z += 0.001;
        test_vt2.c.x -= 0.001;

        SDL_UpdateTexture(texture, NULL, pixels, SCREEN_WIDTH * 4);
        SDL_RenderCopyEx(
            renderer,
            texture,
            NULL,
            NULL,
            0.0,
            NULL,
            0); //SDL_FLIP_VERTICAL
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);

    exit(EXIT_SUCCESS);
}
