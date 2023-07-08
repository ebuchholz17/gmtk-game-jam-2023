#include "minesweeper_game.h"

mine_state *ms;
mem_arena *msScratchMemory;

void saveScratchMemPointer (mem_arena *scratchMemory, scratch_mem_save *scratchMemSave) {
    scratchMemSave->current = scratchMemory->current;
}

void restoreScratchMemPointer (mem_arena *scratchMemory, scratch_mem_save *scratchMemSave) {
    scratchMemory->current = scratchMemSave->current;
}

void initMinesweeper (mine_state* mineState, mem_arena *memory) {
    ms = mineState;
    zeroMemory((u8 *)ms, sizeof(mine_state));

    ms->initialized = true;

    for (int i = 0; i < GRID_HEIGHT; ++i) {
        for (int j = 0; j < GRID_WIDTH; ++j) {
            mine_cell *cell = &ms->cells[i * GRID_WIDTH + j];
            cell->row = i;
            cell->col = j;
        }
    }
}

void generateBombs (int initialRow, int initialCol) {
    mine_cell *firstCell = &ms->cells[initialRow * GRID_WIDTH + initialCol];
    firstCell->cantHaveBomb = true;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int row = initialRow + i;
            int col = initialCol + j;

            if ((row == initialRow && col == initialCol )|| 
                row < 0 || row >= GRID_HEIGHT ||
                col < 0 || col >= GRID_WIDTH) 
            {
                continue;
            }

            mine_cell *neighbor = &ms->cells[row * GRID_WIDTH + col];
            neighbor->cantHaveBomb = true;
        }
    }

    int numBombs = 99;
    int numPlacedBombs = 0;
    for (int i = 0; i < GRID_HEIGHT; ++i) {
        for (int j = 0; j < GRID_WIDTH; ++j) {
            mine_cell *cell = &ms->cells[i * GRID_WIDTH + j];

            if (!cell->cantHaveBomb && numPlacedBombs < numBombs) {
                cell->hasBomb = true;
                numPlacedBombs++;
            }
        }
    }

    int numCells = GRID_WIDTH * GRID_HEIGHT;
    for (int i = numCells - 1; i >= 0; --i) {
        mine_cell *cell = &ms->cells[i];

        int randomIndex = randomU32() % numCells;
        mine_cell *randomCell = &ms->cells[randomIndex];

        if (cell->cantHaveBomb || randomCell->cantHaveBomb) {
            continue;
        }
    
        b32 temp = cell->hasBomb;
        cell->hasBomb = randomCell->hasBomb;
        randomCell->hasBomb = temp;
    }

    for (int cellIndex = 0; cellIndex < GRID_WIDTH * GRID_HEIGHT; ++cellIndex) {
        mine_cell *cell = &ms->cells[cellIndex];

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                int row = cell->row + i;
                int col = cell->col + j;

                if ((row == cell->row && col == cell->col )|| 
                    row < 0 || row >= GRID_HEIGHT ||
                    col < 0 || col >= GRID_WIDTH) 
                {
                    continue;
                }

                mine_cell *neighbor = &ms->cells[row * GRID_WIDTH + col];
                if (neighbor->hasBomb) {
                    cell->numAdjBombs++;
                }
            }
        }
    }
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

            result.leftClick = input->zKey;
            result.rightClick = input->xKey;
            //result.playCard = input->aKey;
            //result.pause = input->sKey;
        } break;
        case INPUT_SOURCE_VIRTUAL: {
            result.up = vInput->dPadUp.button;;
            result.down = vInput->dPadDown.button;
            result.left = vInput->dPadLeft.button;
            result.right = vInput->dPadRight.button;

            result.leftClick = vInput->bottomButton.button;
            result.rightClick = vInput->rightButton.button;
        } break;
        case INPUT_SOURCE_GAMEPAD: {
            for (u32 controllerIndex = 0; controllerIndex < MAX_NUM_CONTROLLERS; controllerIndex++) {
                game_controller_input *cont = &input->controllers[controllerIndex];

                if (cont->connected) {
                    result.left = cont->dPadLeft;
                    result.right = cont->dPadRight;
                    result.up = cont->dPadUp;
                    result.down = cont->dPadDown;

                    result.leftClick = cont->aButton;
                    result.rightClick = cont->bButton;
                    break;
                }
            }
        } break;
    }

    return result;
}

void openAllNearbyZeroValueCells (mine_cell *startCell) {
    scratch_mem_save openZeroCellsMemSave;
    saveScratchMemPointer(msScratchMemory, &openZeroCellsMemSave);

    mine_cell_ptr_list cellsToOpen = mine_cell_ptr_listInit(msScratchMemory, GRID_WIDTH * GRID_HEIGHT);
    mine_cell_ptr_listPush(&cellsToOpen, startCell);

    for (int cellIndex = 0; cellIndex < GRID_WIDTH * GRID_HEIGHT; ++cellIndex) {
        mine_cell *cell = &ms->cells[cellIndex];
        cell->seen = false;
    }

    while (cellsToOpen.numValues > 0) {
        mine_cell *currentCell = cellsToOpen.values[0];
        currentCell->open = true;
        mine_cell_ptr_listSplice(&cellsToOpen, 0);

        if (currentCell->numAdjBombs == 0) {
            // TODO(ebuchholz): condense all these -1 to 1 searches somehow
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    int row = currentCell->row + i;
                    int col = currentCell->col + j;

                    if ((row == currentCell->row && col == currentCell->col )|| 
                        row < 0 || row >= GRID_HEIGHT ||
                        col < 0 || col >= GRID_WIDTH) 
                    {
                        continue;
                    }

                    mine_cell *neighbor = &ms->cells[row * GRID_WIDTH + col];
                    if (!neighbor->seen) {
                        neighbor->seen = true;
                        mine_cell_ptr_listPush(&cellsToOpen, neighbor);
                    }
                }
            }
        }
    }

    restoreScratchMemPointer(msScratchMemory, &openZeroCellsMemSave);
}

void revealCell (mine_cell *cell) {
    if (!cell->open) {
        if (!cell->flagged) {
            if (!ms->firstClick) {
                generateBombs(cell->row, cell->col);
                ms->firstClick = true;
            }

            cell->open = true;

            if (cell->hasBomb) {
                //playSound("wormhole_revealed", gameSounds, assets);
            }
            else {
                //playSound("click", gameSounds, assets);
                //ms->score += 10;
                if (cell->numAdjBombs == 0) {
                    openAllNearbyZeroValueCells(cell);
                }
            }
        }
    }
}

void doAIAction (void) {
    if (!ms->firstClick) {
        int randomRow = (int)(randomU32() % GRID_WIDTH);
        int randomCol = (int)(randomU32() % GRID_HEIGHT);
        mine_cell *cell = &ms->cells[randomRow * GRID_WIDTH + randomCol];
        revealCell(cell);
    }
    else {
        scratch_mem_save aiStartMemorySave;
        saveScratchMemPointer(msScratchMemory, &aiStartMemorySave);

        // Get list of (unsolved) open cells with adj bombs
        mine_cell_ptr_list unsolvedCells = mine_cell_ptr_listInit(msScratchMemory, GRID_WIDTH * GRID_HEIGHT);
        for (int i = 0; i < GRID_HEIGHT; ++i) {
            for (int j = 0; j < GRID_WIDTH; ++j) {
                mine_cell *cell = &ms->cells[i * GRID_WIDTH + j];
                if (!cell->solved && cell->open && cell->numAdjBombs > 0) {
                    mine_cell_ptr_listPush(&unsolvedCells, cell);
                }
            }
        }

        // Sort closest to cursor? random shuffle for now
        for (int i = unsolvedCells.numValues - 1; i >= 0; --i) {
            mine_cell *cell = unsolvedCells.values[i];

            int randomIndex = randomU32() % unsolvedCells.numValues;
            mine_cell *randomCell = unsolvedCells.values[randomIndex];

            unsolvedCells.values[i] = randomCell;
            unsolvedCells.values[randomIndex] = cell;
        }

        // For each cell
        b32 didAction = false;
        for (int cellIndex = 0; cellIndex < unsolvedCells.numValues; cellIndex++) {
            mine_cell *cell = unsolvedCells.values[cellIndex];

            //scratch_mem_save cellMemorySave;
            //saveScratchMemPointer(msScratchMemory, &cellMemorySave);

            // - get num adj flagged and num adj unflagged cells
            mine_cell_ptr_list adjCells = mine_cell_ptr_listInit(msScratchMemory, 8);
            for (int i = -1; i <= 1; ++i) {
                for (int j = -1; j <= 1; ++j) {
                    int row = cell->row + i;
                    int col = cell->col + j;

                    if ((row == cell->row && col == cell->col )|| 
                        row < 0 || row >= GRID_HEIGHT ||
                        col < 0 || col >= GRID_WIDTH) 
                    {
                        continue;
                    }

                    mine_cell *neighbor = &ms->cells[row * GRID_WIDTH + col];
                    if (!neighbor->open) {
                        mine_cell_ptr_listPush(&adjCells, neighbor);
                    }
                }
            }

            int numFlaggedCells = 0;
            int numUnflaggedCells = 0;

            for (int adjIndex = 0; adjIndex < adjCells.numValues; adjIndex++) {
                mine_cell *adjCell = adjCells.values[adjIndex];
                if (adjCell->flagged) {
                    numFlaggedCells++;
                }
                else {
                    numUnflaggedCells++;
                }
            }

            int numRemainingBombs = cell->numAdjBombs - numFlaggedCells;
            // - if num adj flagged = num adj bombs
            if (numFlaggedCells == cell->numAdjBombs) {
                //   - if num ajd unflagged = 0, marked solved
                if (numUnflaggedCells == 0) {
                    cell->solved = true;
                }
                else {
                    // open adj cells
                    // TODO: animate cursor to one of the cells and click it
                    for (int adjIndex = 0; adjIndex < adjCells.numValues; adjIndex++) {
                        mine_cell *adjCell = adjCells.values[adjIndex];
                        if (!adjCell->flagged && !adjCell->open) {
                            revealCell(adjCell);
                        }
                    }
                    didAction = true;
                }
            }
            // - if num adj unflagged = num adj bombs
            //   - flag all adj cells
            else if (numUnflaggedCells == numRemainingBombs) {
                for (int adjIndex = 0; adjIndex < adjCells.numValues; adjIndex++) {
                    mine_cell *adjCell = adjCells.values[adjIndex];
                    if (!adjCell->flagged && !adjCell->open) {
                        adjCell->flagged = true;
                    }
                }
                didAction = true;
            }
            else {
                // If can't do that: form groups
                // e.g. two adj unflagged cells, 1 remaining flag: form a group (cells: [4, 5], [4, 6]; bombs: 1)
                
                // deduce open cell: check if a (whole) group of considered cells is known to contain X bombs: then decrement num remaining flags
                // if num remaining flags= 0: open remaining considered cells

                // deduce flag cell: check if a (partial) group of considered cells is know to contain X flags: then decremtn num remaining flags
                // if num remaining flags = num remaining considered cells: flag those cells
            }
            //restoreScratchMemPointer(msScratchMemory, &cellMemorySave);

            if (didAction) {
                break;
            }
        }

        // if all open cells with adj bombs considered and no options: pick an adj unflagged cell
        // randomly and open it (guess)

        // keep cells solved across frames: reset groups, cell list, etc. between frames

        restoreScratchMemPointer(msScratchMemory, &aiStartMemorySave);
    }
}

void updateMinesweeper (game_input *input, virtual_input *vInput, f32 dt, 
                        mem_arena *memory, mem_arena *scratchMemory, plat_api platAPI) 
{
    msScratchMemory = scratchMemory;
    minesweeper_input msInput = parseGameInput(input, vInput);

    if (!ms->gameStarted) {
        //startGame(memory);
    }

    ms->aiTimer += dt;
    f32 aiStepTime = 0.1f;
    while (ms->aiTimer >= aiStepTime) {
        ms->aiTimer -= aiStepTime;
        doAIAction();
    }

    if (msInput.right.down) {
        ms->pointerX += 64 * dt;
    }
    if (msInput.left.down) {
        ms->pointerX -= 64 * dt;
    }
    if (msInput.up.down) {
        ms->pointerY -= 64 * dt;
    }
    if (msInput.down.down) {
        ms->pointerY += 64 * dt;
    }

    int mouseRow = (int)(ms->pointerY / CELL_DIM);
    int mouseCol = (int)(ms->pointerX / CELL_DIM);

    if (mouseRow >= 0 && mouseRow < GRID_HEIGHT && mouseCol >= 0 && mouseCol < GRID_WIDTH) {
        if (msInput.leftClick.justPressed) {

            mine_cell *cell = &ms->cells[mouseRow * GRID_WIDTH + mouseCol];
            if (!cell->open) {
                revealCell(cell);
            }
        }
        else if (msInput.rightClick.justPressed) {
            mine_cell *cell = &ms->cells[mouseRow * GRID_WIDTH + mouseCol];
            if (!cell->open) {
                cell->flagged = !cell->flagged;
                //playSound("click", gameSounds, assets);
            }
        }
    }
}

void drawMinesweeper (plat_api platAPI, f32 gameScale, mem_arena *scratchMemory) {
    vec2 gridStart = { .x = 80.0f, .y = 56.0f };

    for (int i = 0; i < GRID_HEIGHT; ++i) {
        for (int j = 0; j < GRID_WIDTH; ++j) {
            mine_cell *cell = &ms->cells[i * GRID_WIDTH + j];
            cell->row = i;
            cell->col = j;

            sprite cellSprite = defaultSprite();
            cellSprite.pos.x = gridStart.x + (float)(cell->col * CELL_DIM);
            cellSprite.pos.y = gridStart.y + (float)(cell->row * CELL_DIM);
            cellSprite.atlasKey = "atlas";

            if (!cell->open) {
                cellSprite.frameKey = "closed_cell";
                spriteManAddSprite(cellSprite);

                if (cell->flagged) {
                    cellSprite.frameKey = "flag";
                    spriteManAddSprite(cellSprite);
                }
            }
            else {
                cellSprite.frameKey = "open_cell";
                spriteManAddSprite(cellSprite);

                if (cell->hasBomb) {
                    cellSprite.frameKey = "bomb";
                    spriteManAddSprite(cellSprite);
                }
                else {
                    if (cell->numAdjBombs > 0) {
                        cellSprite.frameKey = tempStringFromI32(cell->numAdjBombs);
                        spriteManAddSprite(cellSprite);
                    }
                }
            }
        }
    }

    sprite pointerSprite = defaultSprite();
    pointerSprite.pos.x = gridStart.x + ms->pointerX;
    pointerSprite.pos.y = gridStart.y + ms->pointerY;
    pointerSprite.atlasKey = "atlas";
    pointerSprite.frameKey = "pointer";
    spriteManAddSprite(pointerSprite);
}
