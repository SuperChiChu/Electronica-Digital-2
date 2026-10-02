/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "ili9341.h"
#include <math.h>
#include "car_sprites.h"
#include "track_tiles.h"
#include "villa_nueva_map.h"
#include "caes_map.h"


/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

typedef enum
{
    STATE_MENU,
    STATE_MAP_SELECT,
    STATE_CAR_SELECT_P1,
    STATE_CAR_SELECT_P2,
    STATE_GAME
} GameState;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define MAX_SPEED 340.0f
#define GRASS_MAX_SPEED 80.0f
#define GRASS_DRAG 600.0f
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
SPI_HandleTypeDef hspi1;
DMA_HandleTypeDef hdma_spi1_tx;

/* USER CODE BEGIN PV */

float car1_x = 0.0f;
float car1_y = 0.0f;
float car1_angle = 90.0f;
float car1_speed = 0.0f;

float car2_x = 0.0f;
float car2_y = 0.0f;
float car2_angle = 90.0f;
float car2_speed = 0.0f;

uint8_t active_player = 1;

GPIO_PinState last_brake;

GPIO_PinState idle_k1;
GPIO_PinState idle_k2;
GPIO_PinState idle_k3;

GPIO_PinState last_k1;
GPIO_PinState last_k2;
GPIO_PinState last_k3;

uint32_t last_tick = 0;
uint32_t last_frame_tick = 0;

uint8_t line_buffer[2][ILI9341_WIDTH * 2];

uint8_t *draw_buffer;

GameState game_state = STATE_MENU;

uint8_t menu_selection = 0;
uint8_t player_count = 1;

uint8_t selected_map = 0;

uint8_t car_selection = 0;
uint8_t selected_car_p1 = 0;
uint8_t selected_car_p2 = 1;


GPIO_PinState idle_brake;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_SPI1_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

static void ResetRacePosition(void)
{
    car1_speed = 0.0f;
    car2_speed = 0.0f;

    active_player = 1;


    if (selected_map == 0)
    {
        float start_world_x =
            VILLA_NUEVA_START_TILE_X *
            TRACK_TILE_SIZE +
            TRACK_TILE_SIZE / 2.0f;

        float start_world_y =
            VILLA_NUEVA_START_TILE_Y *
            TRACK_TILE_SIZE +
            TRACK_TILE_SIZE / 2.0f;


        if (player_count == 1)
        {
            car1_x =
                start_world_x;

            car1_y =
                start_world_y;
        }
        else
        {
            /*
             * La meta va horizontalmente
             * y los carros miran hacia +X.
             *
             * Por eso los separamos en Y.
             */

            car1_x =
                start_world_x;

            car1_y =
                start_world_y - 18.0f;


            car2_x =
                start_world_x;

            car2_y =
                start_world_y + 18.0f;
        }


        car1_angle = 90.0f;
        car2_angle = 90.0f;
    }
    else if (selected_map == 1)
    {
        float start_world_x =
            CAES_START_TILE_X *
            TRACK_TILE_SIZE +
            TRACK_TILE_SIZE / 2.0f;

        float start_world_y =
            CAES_START_TILE_Y *
            TRACK_TILE_SIZE +
            TRACK_TILE_SIZE / 2.0f;


        if (player_count == 1)
        {
            car1_x =
                start_world_x;

            car1_y =
                start_world_y;
        }
        else
        {
            car1_x =
                start_world_x;

            car1_y =
                start_world_y -
                18.0f;


            car2_x =
                start_world_x;

            car2_y =
                start_world_y +
                18.0f;
        }


        car1_angle = 90.0f;
        car2_angle = 90.0f;
    }
}

static void UpdateCar(
    float *x,
    float *y,
    float *angle,
    float *speed,
    uint8_t left,
    uint8_t right,
    uint8_t accel,
    uint8_t brake,
    float dt)
{
    const float TURN_SPEED =
        180.0f;

    const float ACCELERATION =
        120.0f;

    const float BRAKE_FORCE =
        450.0f;

    const float COAST_DRAG =
        40.0f;


    if (left && !right)
    {
        *angle -=
            TURN_SPEED *
            dt;

        if (*angle < 0.0f)
        {
            *angle += 360.0f;
        }
    }


    if (right && !left)
    {
        *angle +=
            TURN_SPEED *
            dt;

        if (*angle >= 360.0f)
        {
            *angle -= 360.0f;
        }
    }


    if (brake)
    {
        *speed -=
            BRAKE_FORCE *
            dt;
    }
    else if (accel)
    {
        *speed +=
            ACCELERATION *
            dt;
    }
    else
    {
        *speed -=
            COAST_DRAG *
            dt;
    }


    if (*speed > MAX_SPEED)
    {
        *speed =
            MAX_SPEED;
    }

    if (*speed < 0.0f)
    {
        *speed =
            0.0f;
    }


    if (*speed > 0.0f)
    {
        float radians =
            (*angle) *
            3.14159265f /
            180.0f;


        *x +=
            sinf(radians) *
            (*speed) *
            dt;

        *y -=
            cosf(radians) *
            (*speed) *
            dt;
    }
}

static int PositiveMod(int value, int modulo)
{
    int result = value % modulo;

    if (result < 0)
    {
        result += modulo;
    }

    return result;
}

static int FloorDiv32(int value)
{
    if (value >= 0)
    {
        return value / TRACK_TILE_SIZE;
    }

    return -(
        (-value + TRACK_TILE_SIZE - 1) /
        TRACK_TILE_SIZE
    );
}

static uint8_t IsCarOnGrass(float x,
                            float y)
{
    int tile_x =
        FloorDiv32((int)x);

    int tile_y =
        FloorDiv32((int)y);


    if (selected_map == 0)
    {
        if (tile_x < 0 ||
            tile_y < 0 ||
            tile_x >= VILLA_NUEVA_MAP_WIDTH ||
            tile_y >= VILLA_NUEVA_MAP_HEIGHT)
        {
            return 1;
        }

        return
            villa_nueva_map[tile_y][tile_x]
            == TRACK_TILE_GRASS;
    }


    if (selected_map == 1)
    {
        if (tile_x < 0 ||
            tile_y < 0 ||
            tile_x >= CAES_MAP_WIDTH ||
            tile_y >= CAES_MAP_HEIGHT)
        {
            return 1;
        }

        return
            caes_map[tile_y][tile_x]
            == TRACK_TILE_GRASS;
    }


    return 0;
}

static void RenderVillaNuevaLineFast(int world_start_x,
                                     int world_y)
{
    int pixel_y =
        PositiveMod(
            world_y,
            TRACK_TILE_SIZE
        );

    int tile_y =
        FloorDiv32(world_y);

    int screen_x = 0;
    int world_x = world_start_x;


    while (screen_x < ILI9341_WIDTH)
    {
        int pixel_x =
            PositiveMod(
                world_x,
                TRACK_TILE_SIZE
            );

        int tile_x =
            FloorDiv32(world_x);


        int run =
            TRACK_TILE_SIZE -
            pixel_x;

        if (run >
            ILI9341_WIDTH -
            screen_x)
        {
            run =
                ILI9341_WIDTH -
                screen_x;
        }


        uint8_t tile =
            TRACK_TILE_GRASS;


        if (tile_x >= 0 &&
            tile_y >= 0 &&
            tile_x <
                VILLA_NUEVA_MAP_WIDTH &&
            tile_y <
                VILLA_NUEVA_MAP_HEIGHT)
        {
            tile =
                villa_nueva_map[
                    tile_y
                ][
                    tile_x
                ];
        }


        const uint16_t *src =
            &track_tiles[
                tile
            ][
                pixel_y *
                TRACK_TILE_SIZE +
                pixel_x
            ];


        for (int i = 0;
             i < run;
             i++)
        {
            uint16_t color =
                src[i];

            int x =
                screen_x + i;

            draw_buffer[
                x * 2
            ] =
                (color >> 8) &
                0xFF;

            draw_buffer[
                x * 2 + 1
            ] =
                color &
                0xFF;
        }


        screen_x += run;
        world_x += run;
    }
}

static void RenderCAESLineFast(
    int world_start_x,
    int world_y)
{
    int pixel_y =
        PositiveMod(
            world_y,
            TRACK_TILE_SIZE
        );

    int tile_y =
        FloorDiv32(
            world_y
        );

    int screen_x = 0;
    int world_x =
        world_start_x;


    while (screen_x <
           ILI9341_WIDTH)
    {
        int pixel_x =
            PositiveMod(
                world_x,
                TRACK_TILE_SIZE
            );

        int tile_x =
            FloorDiv32(
                world_x
            );


        int run =
            TRACK_TILE_SIZE -
            pixel_x;


        if (run >
            ILI9341_WIDTH -
            screen_x)
        {
            run =
                ILI9341_WIDTH -
                screen_x;
        }


        uint8_t tile =
            TRACK_TILE_GRASS;


        if (tile_x >= 0 &&
            tile_y >= 0 &&
            tile_x <
                CAES_MAP_WIDTH &&
            tile_y <
                CAES_MAP_HEIGHT)
        {
            tile =
                caes_map[
                    tile_y
                ][
                    tile_x
                ];
        }


        const uint16_t *src =
            &track_tiles[
                tile
            ][
                pixel_y *
                TRACK_TILE_SIZE +
                pixel_x
            ];


        for (int i = 0;
             i < run;
             i++)
        {
            uint16_t color =
                src[i];

            int x =
                screen_x +
                i;


            draw_buffer[
                x * 2
            ] =
                (color >> 8) &
                0xFF;

            draw_buffer[
                x * 2 + 1
            ] =
                color &
                0xFF;
        }


        screen_x += run;
        world_x += run;
    }
}

static void SetPixelInLine(int x, uint16_t color)
{
    if (x < 0 ||
        x >= ILI9341_WIDTH)
    {
        return;
    }

    draw_buffer[x * 2] =
        (color >> 8) & 0xFF;

    draw_buffer[x * 2 + 1] =
        color & 0xFF;
}


static uint16_t GetCarPixelForPlayer(
    uint8_t car_id,
    float local_x,
    float local_y)
{
    const uint16_t *sprite =
        CarSprites_Get(car_id);

    int sprite_x =
        (int)roundf(local_x) +
        (CAR_SPRITE_WIDTH / 2);

    int sprite_y =
        (int)roundf(local_y) +
        (CAR_SPRITE_HEIGHT / 2);


    if (sprite_x < 0 ||
        sprite_x >= CAR_SPRITE_WIDTH ||
        sprite_y < 0 ||
        sprite_y >= CAR_SPRITE_HEIGHT)
    {
        return CAR_TRANSPARENT;
    }


    return sprite[
        sprite_y *
        CAR_SPRITE_WIDTH +
        sprite_x
    ];
}

static void GetGlyph(char c, uint8_t glyph[5])
{
    for (int i = 0; i < 5; i++)
    {
        glyph[i] = 0;
    }

    switch (c)
    {
        case 'A':
        {
            uint8_t g[5] = {0x7E,0x11,0x11,0x11,0x7E};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'B':
        {
            uint8_t g[5] = {0x7F,0x49,0x49,0x49,0x36};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'C':
        {
            uint8_t g[5] = {0x3E,0x41,0x41,0x41,0x22};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'D':
        {
            uint8_t g[5] = {0x7F,0x41,0x41,0x22,0x1C};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'E':
        {
            uint8_t g[5] = {0x7F,0x49,0x49,0x49,0x41};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'G':
        {
            uint8_t g[5] = {0x3E,0x41,0x49,0x49,0x3A};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'I':
        {
            uint8_t g[5] = {0x00,0x41,0x7F,0x41,0x00};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'K':
        {
            uint8_t g[5] = {0x7F,0x08,0x14,0x22,0x41};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'L':
        {
            uint8_t g[5] = {0x7F,0x40,0x40,0x40,0x40};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'N':
        {
            uint8_t g[5] = {0x7F,0x02,0x0C,0x10,0x7F};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'O':
        {
            uint8_t g[5] = {0x3E,0x41,0x41,0x41,0x3E};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'P':
        {
            uint8_t g[5] = {0x7F,0x09,0x09,0x09,0x06};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'Q':
        {
            uint8_t g[5] = {0x3E,0x41,0x51,0x21,0x5E};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'R':
        {
            uint8_t g[5] = {0x7F,0x09,0x19,0x29,0x46};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'S':
        {
            uint8_t g[5] = {0x46,0x49,0x49,0x49,0x31};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'T':
        {
            uint8_t g[5] = {0x01,0x01,0x7F,0x01,0x01};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'U':
        {
            uint8_t g[5] = {0x3F,0x40,0x40,0x40,0x3F};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'V':
        {
            uint8_t g[5] = {0x1F,0x20,0x40,0x20,0x1F};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'W':
        {
            uint8_t g[5] = {0x3F,0x40,0x38,0x40,0x3F};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case 'Y':
        {
            uint8_t g[5] = {0x03,0x04,0x78,0x04,0x03};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case '1':
        {
            uint8_t g[5] = {0x00,0x42,0x7F,0x40,0x00};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case '2':
        {
            uint8_t g[5] = {0x62,0x51,0x49,0x49,0x46};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case '3':
        {
            uint8_t g[5] = {0x22,0x41,0x49,0x49,0x36};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case '>':
        {
            uint8_t g[5] = {0x00,0x22,0x14,0x08,0x00};
            for (int i = 0; i < 5; i++) glyph[i] = g[i];
            break;
        }

        case ' ':
        default:
            break;
    }
}


static void DrawChar5x7(int x,
                        int y,
                        char c,
                        uint16_t color,
                        int scale)
{
    uint8_t glyph[5];

    GetGlyph(c, glyph);

    for (int col = 0; col < 5; col++)
    {
        for (int row = 0; row < 7; row++)
        {
            if (glyph[col] & (1 << row))
            {
                ILI9341_FillRect(
                    x + col * scale,
                    y + row * scale,
                    scale,
                    scale,
                    color
                );
            }
        }
    }
}


static void DrawText5x7(int x,
                        int y,
                        const char *text,
                        uint16_t color,
                        int scale)
{
    while (*text)
    {
        DrawChar5x7(
            x,
            y,
            *text,
            color,
            scale
        );

        x += 6 * scale;

        text++;
    }
}


static void DrawMenu(void)
{
    ILI9341_FillScreen(ILI9341_BLACK);

    DrawText5x7(
        12,
        45,
        "VILLA NUEVA RACING",
        ILI9341_WHITE,
        2
    );

    if (menu_selection == 0)
    {
        DrawText5x7(
            48,
            135,
            "> 1 PLAYER",
            ILI9341_YELLOW,
            2
        );

        DrawText5x7(
            48,
            175,
            "  2 PLAYERS",
            ILI9341_WHITE,
            2
        );
    }
    else
    {
        DrawText5x7(
            48,
            135,
            "  1 PLAYER",
            ILI9341_WHITE,
            2
        );

        DrawText5x7(
            48,
            175,
            "> 2 PLAYERS",
            ILI9341_YELLOW,
            2
        );
    }

    DrawText5x7(
        42,
        255,
        "K1 K2 SELECT",
        ILI9341_GREEN,
        1
    );

    DrawText5x7(
        78,
        280,
        "K3 START",
        ILI9341_GREEN,
        1
    );
}


static void DrawMapSelect(void)
{
    ILI9341_FillScreen(ILI9341_BLACK);


    DrawText5x7(
        48,
        40,
        "SELECT TRACK",
        ILI9341_WHITE,
        2
    );


    DrawText5x7(
        55,
        135,
        "K1 VILLA NUEVA",
        ILI9341_YELLOW,
        1
    );


    DrawText5x7(
        75,
        185,
        "K2 CAES",
        ILI9341_YELLOW,
        1
    );
}


static void DrawCarSelect(uint8_t player)
{
    const char *car_names[4] =
    {
        "RED",
        "BLUE",
        "YELLOW",
        "GREEN"
    };

    uint16_t car_colors[4] =
    {
        ILI9341_RED,
        ILI9341_BLUE,
        ILI9341_YELLOW,
        ILI9341_GREEN
    };

    ILI9341_FillScreen(ILI9341_BLACK);

    DrawText5x7(
        54,
        35,
        "SELECT CAR",
        ILI9341_WHITE,
        2
    );

    if (player == 1)
    {
        DrawText5x7(
            84,
            75,
            "PLAYER 1",
            ILI9341_WHITE,
            1
        );
    }
    else
    {
        DrawText5x7(
            84,
            75,
            "PLAYER 2",
            ILI9341_WHITE,
            1
        );
    }

    DrawText5x7(
        84,
        110,
        car_names[car_selection],
        car_colors[car_selection],
        2
    );

    ILI9341_FillRect(
        95,
        155,
        50,
        75,
        car_colors[car_selection]
    );

    ILI9341_FillRect(
        103,
        165,
        34,
        18,
        ILI9341_WHITE
    );

    ILI9341_FillRect(
        103,
        202,
        34,
        18,
        ILI9341_WHITE
    );

    DrawText5x7(
        30,
        255,
        "K1",
        ILI9341_WHITE,
        1
    );

    DrawText5x7(
        195,
        255,
        "K2",
        ILI9341_WHITE,
        1
    );

    DrawText5x7(
        78,
        285,
        "K3 SELECT",
        ILI9341_GREEN,
        1
    );
}

static void DrawCarOnLine(
    int local_y,
    int center_x,
    int center_y,
    float angle,
    uint8_t car_id)
{
    int dy =
        local_y -
        center_y;


    if (dy < -30 ||
        dy > 30)
    {
        return;
    }


    float radians =
        angle *
        3.14159265f /
        180.0f;

    float c =
        cosf(radians);

    float s =
        sinf(radians);


    for (int x =
             center_x - 30;
         x <=
             center_x + 30;
         x++)
    {
        if (x < 0 ||
            x >= ILI9341_WIDTH)
        {
            continue;
        }


        int dx =
            x -
            center_x;


        float local_x =
            dx * c +
            dy * s;

        float local_car_y =
            -dx * s +
            dy * c;


        uint16_t color =
            GetCarPixelForPlayer(
                car_id,
                local_x,
                local_car_y
            );


        if (color !=
            CAR_TRANSPARENT)
        {
            SetPixelInLine(
                x,
                color
            );
        }
    }
}

static void DrawSpeedBarLine(
    int local_y,
    float speed)
{
    const int speed_x = 20;
    const int speed_y = 6;
    const int speed_width = 200;
    const int speed_height = 8;


    if (local_y < speed_y ||
        local_y >=
            speed_y +
            speed_height)
    {
        return;
    }


    int speed_percent =
        (int)(
            speed /
            MAX_SPEED *
            100.0f
        );


    if (speed_percent < 0)
    {
        speed_percent = 0;
    }

    if (speed_percent > 100)
    {
        speed_percent = 100;
    }


    int fill_width =
        speed_percent *
        (speed_width - 2) /
        100;


    for (int x = speed_x;
         x <
             speed_x +
             speed_width;
         x++)
    {
        uint16_t color =
            ILI9341_BLACK;


        if (local_y == speed_y ||
            local_y ==
                speed_y +
                speed_height - 1 ||
            x == speed_x ||
            x ==
                speed_x +
                speed_width - 1)
        {
            color =
                ILI9341_WHITE;
        }
        else if (
            x <
            speed_x +
            1 +
            fill_width)
        {
            color =
                ILI9341_GREEN;
        }


        SetPixelInLine(
            x,
            color
        );
    }
}

static void RenderFrame(void)
{
    uint8_t buffer_index = 0;


    ILI9341_BeginFrame();


    for (int y = 0;
         y < ILI9341_HEIGHT;
         y++)
    {
        draw_buffer =
            line_buffer[buffer_index];


        /*
         * =========================
         *      UN JUGADOR
         * =========================
         */

        if (player_count == 1)
        {
            const int center_x =
                ILI9341_WIDTH / 2;

            const int center_y =
                ILI9341_HEIGHT / 2;


            int camera_x =
                (int)car1_x -
                center_x;

            int camera_y =
                (int)car1_y -
                center_y;


            int world_y =
                camera_y +
                y;


            if (selected_map == 0)
            {
                RenderVillaNuevaLineFast(
                    camera_x,
                    world_y
                );
            }
            else if (selected_map == 1)
            {
                RenderCAESLineFast(
                    camera_x,
                    world_y
                );
            }


            DrawCarOnLine(
                y,
                center_x,
                center_y,
                car1_angle,
                selected_car_p1
            );


            DrawSpeedBarLine(
                y,
                car1_speed
            );
        }


        /*
         * =========================
         *      DOS JUGADORES
         * =========================
         */

        else
        {
            /*
             * P1:
             * líneas 0 - 158
             */

            if (y < 159)
            {
                const int local_y =
                    y;

                const int center_x =
                    120;

                const int center_y =
                    79;


                int camera_x =
                    (int)car1_x -
                    center_x;

                int camera_y =
                    (int)car1_y -
                    center_y;


                int world_y =
                    camera_y +
                    local_y;


                if (selected_map == 0)
                {
                    RenderVillaNuevaLineFast(
                        camera_x,
                        world_y
                    );
                }
                else if (selected_map == 1)
                {
                    RenderCAESLineFast(
                        camera_x,
                        world_y
                    );
                }


                DrawCarOnLine(
                    local_y,
                    center_x,
                    center_y,
                    car1_angle,
                    selected_car_p1
                );


                DrawSpeedBarLine(
                    local_y,
                    car1_speed
                );


                /*
                 * Indicador amarillo:
                 * K1/K2/K3 controlan P1.
                 */

                if (active_player == 1)
                {
                    SetPixelInLine(
                        0,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        1,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        2,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        3,
                        ILI9341_YELLOW
                    );
                }
            }


            /*
             * Separador central.
             */

            else if (
                y == 159 ||
                y == 160)
            {
                for (int x = 0;
                     x <
                         ILI9341_WIDTH;
                     x++)
                {
                    SetPixelInLine(
                        x,
                        ILI9341_WHITE
                    );
                }
            }


            /*
             * P2:
             * líneas 161 - 319
             */

            else
            {
                const int local_y =
                    y - 161;

                const int center_x =
                    120;

                const int center_y =
                    79;


                int camera_x =
                    (int)car2_x -
                    center_x;

                int camera_y =
                    (int)car2_y -
                    center_y;


                int world_y =
                    camera_y +
                    local_y;


                if (selected_map == 0)
                {
                    RenderVillaNuevaLineFast(
                        camera_x,
                        world_y
                    );
                }
                else if (selected_map == 1)
                {
                    RenderCAESLineFast(
                        camera_x,
                        world_y
                    );
                }


                DrawCarOnLine(
                    local_y,
                    center_x,
                    center_y,
                    car2_angle,
                    selected_car_p2
                );


                DrawSpeedBarLine(
                    local_y,
                    car2_speed
                );


                if (active_player == 2)
                {
                    SetPixelInLine(
                        0,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        1,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        2,
                        ILI9341_YELLOW
                    );

                    SetPixelInLine(
                        3,
                        ILI9341_YELLOW
                    );
                }
            }
        }


        /*
         * DMA
         */

        if (y > 0)
        {
            ILI9341_WaitDMA();
        }


        ILI9341_WriteLineDMA(
            line_buffer[
                buffer_index
            ],
            ILI9341_WIDTH * 2
        );


        buffer_index ^= 1;
    }


    ILI9341_WaitDMA();

    ILI9341_EndFrame();
}

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  /* USER CODE BEGIN 2 */

  HAL_Delay(200);

  ILI9341_Init();

  HAL_Delay(50);

  idle_k1 =
      HAL_GPIO_ReadPin(
          K1_GPIO_Port,
          K1_Pin
      );

  idle_k2 =
      HAL_GPIO_ReadPin(
          K2_GPIO_Port,
          K2_Pin
      );

  idle_k3 =
      HAL_GPIO_ReadPin(
          K3_GPIO_Port,
          K3_Pin
      );

  idle_brake =
      HAL_GPIO_ReadPin(
          B1_GPIO_Port,
          B1_Pin
      );

  last_brake =
      idle_brake;

  last_k1 = idle_k1;
  last_k2 = idle_k2;
  last_k3 = idle_k3;

  last_tick =
      HAL_GetTick();

  last_frame_tick =
      HAL_GetTick();

  DrawMenu();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    GPIO_PinState k1 =
        HAL_GPIO_ReadPin(
            K1_GPIO_Port,
            K1_Pin
        );

    GPIO_PinState k2 =
        HAL_GPIO_ReadPin(
            K2_GPIO_Port,
            K2_Pin
        );

    GPIO_PinState k3 =
        HAL_GPIO_ReadPin(
            K3_GPIO_Port,
            K3_Pin
        );

    GPIO_PinState brake =
        HAL_GPIO_ReadPin(
            B1_GPIO_Port,
            B1_Pin
        );

    uint8_t brake_pressed =
        (brake != idle_brake);

    uint8_t brake_new =
        brake_pressed &&
        (last_brake == idle_brake);

    uint8_t k1_pressed =
        (k1 != idle_k1);

    uint8_t k2_pressed =
        (k2 != idle_k2);

    uint8_t k3_pressed =
        (k3 != idle_k3);

    uint8_t k1_new =
        k1_pressed &&
        (last_k1 == idle_k1);

    uint8_t k2_new =
        k2_pressed &&
        (last_k2 == idle_k2);

    uint8_t k3_new =
        k3_pressed &&
        (last_k3 == idle_k3);


    if (game_state ==
        STATE_MENU)
    {
        if (k1_new)
        {
            menu_selection = 0;

            DrawMenu();
        }

        if (k2_new)
        {
            menu_selection = 1;

            DrawMenu();
        }

        if (k3_new)
        {
            if (menu_selection == 0)
            {
                player_count = 1;
            }
            else
            {
                player_count = 2;
            }

            game_state =
                STATE_MAP_SELECT;

            DrawMapSelect();
        }
    }


    else if (game_state ==
             STATE_MAP_SELECT)
    {
        if (k1_new)
        {
            selected_map = 0;

            car_selection = 0;

            game_state =
                STATE_CAR_SELECT_P1;

            DrawCarSelect(1);
        }

        else if (k2_new)
        {
            selected_map = 1;

            car_selection = 0;

            game_state =
                STATE_CAR_SELECT_P1;

            DrawCarSelect(1);
        }
    }


    else if (game_state ==
             STATE_CAR_SELECT_P1)
    {
        if (k1_new)
        {
            if (car_selection == 0)
            {
                car_selection = 3;
            }
            else
            {
                car_selection--;
            }

            DrawCarSelect(1);
        }

        else if (k2_new)
        {
            car_selection++;

            if (car_selection >= 4)
            {
                car_selection = 0;
            }

            DrawCarSelect(1);
        }

        else if (k3_new)
        {
            selected_car_p1 =
                car_selection;

            if (player_count == 1)
            {
            	ResetRacePosition();

                last_tick =
                    HAL_GetTick();

                last_frame_tick =
                    HAL_GetTick();

                game_state =
                    STATE_GAME;

                RenderFrame();
            }
            else
            {
                car_selection =
                    (selected_car_p1 + 1) % 4;

                game_state =
                    STATE_CAR_SELECT_P2;

                DrawCarSelect(2);
            }
        }
    }


    else if (game_state ==
             STATE_CAR_SELECT_P2)
    {
        if (k1_new)
        {
            do
            {
                if (car_selection == 0)
                {
                    car_selection = 3;
                }
                else
                {
                    car_selection--;
                }

            } while (
                car_selection ==
                selected_car_p1
            );

            DrawCarSelect(2);
        }

        else if (k2_new)
        {
            do
            {
                car_selection++;

                if (car_selection >= 4)
                {
                    car_selection = 0;
                }

            } while (
                car_selection ==
                selected_car_p1
            );

            DrawCarSelect(2);
        }

        else if (k3_new)
        {
            selected_car_p2 =
                car_selection;

            ResetRacePosition();

            last_tick =
                HAL_GetTick();

            last_frame_tick =
                HAL_GetTick();

            game_state =
                STATE_GAME;

            RenderFrame();
        }
    }


    else if (game_state ==
             STATE_GAME)
    {
        uint32_t now =
            HAL_GetTick();


        float dt =
            (now - last_tick) /
            1000.0f;

        last_tick =
            now;


        if (dt > 0.1f)
        {
            dt = 0.1f;
        }


        /*
         * En 2 jugadores:
         * una pulsación nueva de B1
         * cambia quién controla K1/K2/K3.
         */

        if (player_count == 2 &&
            brake_new)
        {
            if (active_player == 1)
            {
                active_player = 2;
            }
            else
            {
                active_player = 1;
            }
        }


        /*
         * PLAYER 1
         */

        uint8_t p1_left = 0;
        uint8_t p1_right = 0;
        uint8_t p1_accel = 0;
        uint8_t p1_brake = 0;


        /*
         * PLAYER 2
         */

        uint8_t p2_left = 0;
        uint8_t p2_right = 0;
        uint8_t p2_accel = 0;
        uint8_t p2_brake = 0;


        /*
         * Un jugador:
         * controles normales.
         */

        if (player_count == 1)
        {
            p1_left =
                k1_pressed;

            p1_right =
                k2_pressed;

            p1_accel =
                k3_pressed;

            p1_brake =
                brake_pressed;
        }


        /*
         * Dos jugadores:
         * K controla solamente
         * el carro activo.
         */

        else
        {
            if (active_player == 1)
            {
                p1_left =
                    k1_pressed;

                p1_right =
                    k2_pressed;

                p1_accel =
                    k3_pressed;

                p1_brake =
                    brake_pressed;
            }
            else
            {
                p2_left =
                    k1_pressed;

                p2_right =
                    k2_pressed;

                p2_accel =
                    k3_pressed;

                p2_brake =
                    brake_pressed;
            }
        }


        UpdateCar(
            &car1_x,
            &car1_y,
            &car1_angle,
            &car1_speed,
            p1_left,
            p1_right,
            p1_accel,
            p1_brake,
            dt
        );

        if (IsCarOnGrass(car1_x, car1_y))
        {
            if (car1_speed > GRASS_MAX_SPEED)
            {
                car1_speed -=
                    GRASS_DRAG * dt;

                if (car1_speed <
                    GRASS_MAX_SPEED)
                {
                    car1_speed =
                        GRASS_MAX_SPEED;
                }
            }
        }

        if (player_count == 2)
        {
            UpdateCar(
                &car2_x,
                &car2_y,
                &car2_angle,
                &car2_speed,
                p2_left,
                p2_right,
                p2_accel,
                p2_brake,
                dt
            );


            if (IsCarOnGrass(
                    car2_x,
                    car2_y))
            {
                if (car2_speed >
                    GRASS_MAX_SPEED)
                {
                    car2_speed -=
                        GRASS_DRAG * dt;

                    if (car2_speed <
                        GRASS_MAX_SPEED)
                    {
                        car2_speed =
                            GRASS_MAX_SPEED;
                    }
                }
            }
        }


        if ((now -
             last_frame_tick)
            >= 33)
        {
            last_frame_tick =
                now;

            RenderFrame();
        }
    }


    last_k1 = k1;
    last_k2 = k2;
    last_k3 = k3;
    last_brake = brake;

    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE3);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 16;
  RCC_OscInitStruct.PLL.PLLN = 336;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV4;
  RCC_OscInitStruct.PLL.PLLQ = 2;
  RCC_OscInitStruct.PLL.PLLR = 2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream3_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream3_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream3_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_DC_GPIO_Port, LCD_DC_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_RST_GPIO_Port, LCD_RST_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LCD_CS_GPIO_Port, LCD_CS_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : B1_Pin */
  GPIO_InitStruct.Pin = B1_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(B1_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : K1_Pin K2_Pin K3_Pin */
  GPIO_InitStruct.Pin = K1_Pin|K2_Pin|K3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : USART_TX_Pin USART_RX_Pin */
  GPIO_InitStruct.Pin = USART_TX_Pin|USART_RX_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF7_USART2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_DC_Pin */
  GPIO_InitStruct.Pin = LCD_DC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_DC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_RST_Pin */
  GPIO_InitStruct.Pin = LCD_RST_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_RST_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pin : LCD_CS_Pin */
  GPIO_InitStruct.Pin = LCD_CS_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LCD_CS_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */

  __disable_irq();

  while (1)
  {
  }

  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */

  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
