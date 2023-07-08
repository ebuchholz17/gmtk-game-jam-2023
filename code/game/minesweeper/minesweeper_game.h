#ifndef MINESWEEPER_GAME_H
#define MINESWEEPER_GAME_H

typedef enum {
    INPUT_SOURCE_KEYBOARD,
    INPUT_SOURCE_GAMEPAD,
    INPUT_SOURCE_VIRTUAL
} input_source;

typedef struct minesweeper_input {
    input_key up;
    input_key down;
    input_key left;
    input_key right;

    input_key leftClick;
    input_key rightClick;
} minesweeper_input;

typedef struct scratch_mem_save{
    void *current;
} scratch_mem_save;

#define GRID_WIDTH 30
#define GRID_HEIGHT 18
#define CELL_DIM 16

typedef struct mine_cell {
    int row;
    int col;
    int open;
    b32 hasBomb;
    b32 flagged;
    int numAdjBombs;

    b32 cantHaveBomb;
    b32 seen;

    b32 solved;
} mine_cell;

typedef mine_cell *mine_cell_ptr;
#define LIST_TYPE mine_cell_ptr
#include "../list.h"

typedef struct mine_state {
    b32 initialized;
    b32 gameStarted;

    b32 firstClick;

    input_source inputSource;

    mine_cell cells[GRID_WIDTH * GRID_HEIGHT];

    f32 pointerX;
    f32 pointerY;

    f32 aiTimer;
} mine_state;

#endif

