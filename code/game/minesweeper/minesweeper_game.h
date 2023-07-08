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

typedef struct cell_coord {
    i32 row;
    i32 col;
} cell_coord;

#define LIST_TYPE cell_coord
#include "../list.h"

typedef struct cell_group {
    cell_coord_list coords;
    i32 numBombs;
    i32 index;
} cell_group;

#define LIST_TYPE cell_group
#include "../list.h"

#define LIST_TYPE i32
#include "../list.h"

typedef struct mine_cell mine_cell;

typedef mine_cell *mine_cell_ptr;
#define LIST_TYPE mine_cell_ptr
#include "../list.h"

struct mine_cell {
    int row;
    int col;
    int open;
    b32 hasBomb;
    b32 flagged;
    int numAdjBombs;

    b32 cantHaveBomb;
    b32 seen;

    b32 solved;
    i32_list cellGroupIndices;

    // per-frame stuff for solver
    int numFlaggedCells;
    int numUnflaggedCells;
    int numRemainingBombs;
    mine_cell_ptr_list adjCells;
};

typedef enum {
    AI_STATE_THINKING,
    AI_STATE_MOVING,
    AI_STATE_CLICK_DELAY,
    AI_STATE_CLICK
} ai_state;

typedef enum {
    AI_ACTION_REVEAL_CELL,
    AI_ACTION_FLAG_CELL
} ai_action;

typedef struct mine_state {
    b32 initialized;
    b32 gameStarted;

    b32 firstClick;

    input_source inputSource;

    mine_cell cells[GRID_WIDTH * GRID_HEIGHT];

    f32 pointerX;
    f32 pointerY;

    f32 aiTimer;
    ai_state aiState;
    f32 moveDuration;
    vec2 moveStartPos;
    vec2 moveEndPos;
    
    ai_action aiAction;
    i32 targetCellRow;
    i32 targetCellCol;

    b32 revealedBomb;
} mine_state;

#endif

