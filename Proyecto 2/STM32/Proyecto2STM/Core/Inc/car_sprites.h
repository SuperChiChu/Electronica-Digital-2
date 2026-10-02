#ifndef INC_CAR_SPRITES_H_
#define INC_CAR_SPRITES_H_

#include <stdint.h>

#define CAR_SPRITE_WIDTH 22
#define CAR_SPRITE_HEIGHT 40
#define CAR_TRANSPARENT 0xF81F

extern const uint16_t car_red_1[CAR_SPRITE_WIDTH * CAR_SPRITE_HEIGHT];
extern const uint16_t car_blue_1[CAR_SPRITE_WIDTH * CAR_SPRITE_HEIGHT];
extern const uint16_t car_yellow_1[CAR_SPRITE_WIDTH * CAR_SPRITE_HEIGHT];
extern const uint16_t car_green_1[CAR_SPRITE_WIDTH * CAR_SPRITE_HEIGHT];

const uint16_t *CarSprites_Get(uint8_t index);

#endif
