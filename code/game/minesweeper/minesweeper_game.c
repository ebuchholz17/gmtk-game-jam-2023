#include "minesweeper_game.h"

mine_state *ms;

void initMinesweeper (mine_state* mineState, mem_arena *memory) {
    ms = mineState;
    zeroMemory((u8 *)ms, sizeof(mine_state));

    ms->initialized = true;
}

minesweeper_input parseGameInput (game_input *input, virtual_input *vInput) {
    minesweeper_input result = {0};
    if (input->leftArrow.down || 
        input->rightArrow.down || 
        input->upArrow.down || 
        input->downArrow.down ||
        input->aKey.down ||
        input->zKey.down ||
        input->sKey.down ||
        input->xKey.down) 
    {
        ms->inputSource = INPUT_SOURCE_KEYBOARD;
    }
    else if (vInput->dPadUp.button.down || 
             vInput->dPadDown.button.down || 
             vInput->dPadLeft.button.down || 
             vInput->dPadRight.button.down ||
             vInput->topButton.button.down || 
             vInput->bottomButton.button.down || 
             vInput->leftButton.button.down || 
             vInput->rightButton.button.down)
    {
        ms->inputSource = INPUT_SOURCE_VIRTUAL;
    }
    else {
        for (u32 controllerIndex = 0; controllerIndex < MAX_NUM_CONTROLLERS; controllerIndex++) {
            game_controller_input *cont = &input->controllers[controllerIndex];

            if (cont->connected) {
                b32 useController;
                if (cont->dPadUp.down || cont->dPadLeft.down || cont->dPadDown.down || cont->dPadRight.down ||
                    cont->aButton.down || cont->bButton.down || cont->xButton.down || cont->yButton.down) 
                {
                    ms->inputSource = INPUT_SOURCE_GAMEPAD;
                    break;
                }
            }
        }
    }

    switch (ms->inputSource) {
        case INPUT_SOURCE_KEYBOARD: {
            result.left = input->leftArrow;
            result.right = input->rightArrow;
            result.up = input->upArrow;
            result.down = input->downArrow;
            //result.attack = input->zKey;
            //result.dash = input->xKey;
            //result.playCard = input->aKey;
            //result.pause = input->sKey;
        } break;
        case INPUT_SOURCE_VIRTUAL: {
            result.up = vInput->dPadUp.button;;
            result.down = vInput->dPadDown.button;
            result.left = vInput->dPadLeft.button;
            result.right = vInput->dPadRight.button;
            //result.attack = vInput->bottomButton.button;
            //result.dash = vInput->leftButton.button;
            //result.playCard = vInput->rightButton.button;
            //result.pause = vInput->topButton.button;
        } break;
        case INPUT_SOURCE_GAMEPAD: {
            for (u32 controllerIndex = 0; controllerIndex < MAX_NUM_CONTROLLERS; controllerIndex++) {
                game_controller_input *cont = &input->controllers[controllerIndex];

                if (cont->connected) {
                    result.left = cont->dPadLeft;
                    result.right = cont->dPadRight;
                    result.up = cont->dPadUp;
                    result.down = cont->dPadDown;
                    //result.attack = cont->aButton;
                    //result.dash = cont->xButton;
                    //result.playCard = cont->bButton;
                    //result.pause = cont->yButton;
                    break;
                }
            }
        } break;
    }

    return result;
}

void updateMinesweeper (game_input *input, virtual_input *vInput, f32 dt, 
                        mem_arena *memory, mem_arena *scratchMemory, plat_api platAPI) 
{
    minesweeper_input msInput = parseGameInput(input, vInput);

    if (!ms->gameStarted) {
        //startGame(memory);
    }
}

void drawMinesweeper (plat_api platAPI, f32 gameScale, mem_arena *scratchMemory) {
    f32 grassWidth = 108.0f;
    f32 grassHeight = 108.0f;

    vec2 windowOffset = (vec2){ 
        .x = ((f32)platAPI.windowWidth / 2.0f) / gameScale,
        .y = ((f32)platAPI.windowHeight / 2.0f) / gameScale
    };

    vec2 upperCorner = (vec2){ .x = 0.0f, .y = 0.0f };
    f32 grassStartX = (-2 + (i32)(upperCorner.x / grassWidth)) * grassWidth;
    f32 grassStartY = (-2 + (i32)(upperCorner.y / grassHeight)) * grassHeight;

    i32 numCols = (2.0f * windowOffset.x) / grassWidth + 4;
    i32 numRows = (2.0f * windowOffset.y) / grassHeight + 4;
    if (numCols < 0) { numCols = -numCols; }
    if (numRows < 0) { numRows = -numRows; }
    for (u32 grassRow = 0; grassRow < numRows; grassRow++) {
        for (u32 grassCol = 0; grassCol < numCols; grassCol++) {
            sprite grass = defaultSprite();
            grass.pos.x = grassStartX + grassWidth * (f32)grassCol;
            grass.pos.y = grassStartY + grassHeight * (f32)grassRow;
            grass.atlasKey = "atlas";
            grass.frameKey = "grassy_checkers";
            spriteManAddSprite(grass);
        }
    }
}
