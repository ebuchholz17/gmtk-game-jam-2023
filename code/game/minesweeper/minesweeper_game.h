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
} minesweeper_input;

typedef struct scratch_mem_save{
    void *current;
} scratch_mem_save;

typedef struct mine_state {
    b32 initialized;
    b32 gameStarted;

    input_source inputSource;
    scratch_mem_save scratchMemSave;
} mine_state;

#endif

