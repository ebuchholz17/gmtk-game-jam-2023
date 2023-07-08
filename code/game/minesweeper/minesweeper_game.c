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
            cell->cellGroupIndices = i32_listInit(memory, 8);
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
                ms->revealedBomb = true;
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

        for (int i = 0; i < GRID_HEIGHT; ++i) {
            for (int j = 0; j < GRID_WIDTH; ++j) {
                mine_cell *cell = &ms->cells[i * GRID_WIDTH + j];
                cell->cellGroupIndices.numValues = 0;
            }
        }

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
        cell_group_list cellGroups = cell_group_listInit(msScratchMemory, 600);

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
            cell->adjCells = mine_cell_ptr_listInit(msScratchMemory, 8);
            mine_cell_ptr_list *adjCells = &cell->adjCells;
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
                        mine_cell_ptr_listPush(adjCells, neighbor);
                    }
                }
            }

            cell->numFlaggedCells = 0;
            cell->numUnflaggedCells = 0;

            for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                mine_cell *adjCell = adjCells->values[adjIndex];
                if (adjCell->flagged) {
                    cell->numFlaggedCells++;
                }
                else {
                    cell->numUnflaggedCells++;
                }
            }

            cell->numRemainingBombs = cell->numAdjBombs - cell->numFlaggedCells;

            // form groups
            // e.g. two adj unflagged cells, 1 remaining flag: form a group (cells: [4, 5], [4, 6]; bombs: 1)
            cell_group cellGroup = (cell_group){ 
                .coords = cell_coord_listInit(msScratchMemory, 8),
                .numBombs = cell->numRemainingBombs,
                .index = cellGroups.numValues
            };

            for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                mine_cell *adjCell = adjCells->values[adjIndex];
                if (!adjCell->flagged && !adjCell->open) {
                    cell_coord_listPush(&cellGroup.coords, (cell_coord){ .row = adjCell->row, 
                                                                         .col = adjCell->col });
                    i32_listPush(&adjCell->cellGroupIndices, cellGroup.index);
                }
            }

            cell_group_listPush(&cellGroups, cellGroup);
        }

        for (int cellIndex = 0; cellIndex < unsolvedCells.numValues; cellIndex++) {
            mine_cell *cell = unsolvedCells.values[cellIndex];
            mine_cell_ptr_list *adjCells = &cell->adjCells;

            // - if num adj flagged = num adj bombs
            if (cell->numFlaggedCells == cell->numAdjBombs) {
                //   - if num ajd unflagged = 0, marked solved
                if (cell->numUnflaggedCells == 0) {
                    cell->solved = true;
                }
                else {
                    // open adj cells
                    // TODO: animate cursor to one of the cells and click it
                    for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                        mine_cell *adjCell = adjCells->values[adjIndex];
                        if (!adjCell->flagged && !adjCell->open) {
                            revealCell(adjCell);
                        }
                    }
                    didAction = true;
                }
            }
            // - if num adj unflagged = num adj bombs
            //   - flag all adj cells
            else if (cell->numUnflaggedCells == cell->numRemainingBombs) {
                for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                    mine_cell *adjCell = adjCells->values[adjIndex];
                    if (!adjCell->flagged && !adjCell->open) {
                        adjCell->flagged = true;
                    }
                }
                didAction = true;
            }
            else {
                for (i32 adjCellIndex = 0; adjCellIndex < cell->adjCells.numValues; ++adjCellIndex) {
                    mine_cell *adjCell = adjCells->values[adjCellIndex];
                    if (!adjCell->flagged && !adjCell->open && 
                        adjCell->cellGroupIndices.numValues > 0) 
                    {
                        for (i32 cellGroupIndex = 0; 
                             cellGroupIndex < adjCell->cellGroupIndices.numValues; 
                             ++cellGroupIndex) 
                        {
                            scratch_mem_save cellMemorySave;
                            saveScratchMemPointer(msScratchMemory, &cellMemorySave);

                            mine_cell_ptr_list remainingCells = mine_cell_ptr_listInit(msScratchMemory, 8);

                            cell_group cellGroup = cellGroups.values[adjCell->cellGroupIndices.values[cellGroupIndex]];
                            // deduce open cell: check if a (whole) group of considered cells is known to contain X bombs: then decrement num remaining flags
                            // if num remaining flags= 0: open remaining considered cells
                            b32 allCellGroupCoordsAreAdj = true;

                            for (i32 adjCellIndex2 = 0; adjCellIndex2 < cell->adjCells.numValues; ++adjCellIndex2) {
                                mine_cell *adjCell2 = adjCells->values[adjCellIndex2];
                                if (adjCell2->flagged || adjCell2->open) {
                                    continue;
                                } 

                                b32 coordIsAdj = false;
                                for (i32 coordIndex = 0; 
                                     coordIndex < cellGroup.coords.numValues;
                                     ++coordIndex) 
                                {
                                    cell_coord coord = cellGroup.coords.values[coordIndex];

                                    // TODO: better mapping without looping through adj cells again
                                    if (adjCell2->row == coord.row && adjCell2->col == coord.col) {
                                        coordIsAdj = true;
                                        break;
                                    }
                                }

                                if (!coordIsAdj) {
                                    mine_cell_ptr_listPush(&remainingCells, adjCell2);
                                    //allCellGroupCoordsAreAdj = false;
                                }
                            }

                            for (i32 coordIndex = 0; 
                                 coordIndex < cellGroup.coords.numValues;
                                 ++coordIndex) 
                            {
                                cell_coord coord = cellGroup.coords.values[coordIndex];
                                b32 coordIsAdj = false;
                                for (i32 adjCellIndex2 = 0; adjCellIndex2 < cell->adjCells.numValues; ++adjCellIndex2) {
                                    mine_cell *adjCell2 = adjCells->values[adjCellIndex2];

                                    // TODO: better mapping without looping through adj cells again
                                    if (adjCell2->row == coord.row && adjCell2->col == coord.col) {
                                        coordIsAdj = true;
                                        break;
                                    }
                                }

                                if (!coordIsAdj) {
                                    allCellGroupCoordsAreAdj = false;
                                }
                            }

                            if (allCellGroupCoordsAreAdj) {
                                i32 numCellsNotInGroup = cell->numUnflaggedCells - cellGroup.coords.numValues;
                                if (numCellsNotInGroup > 0 && cell->numRemainingBombs - cellGroup.numBombs == 0) {
                                    didAction = true;
                                    for (i32 remainingCellIndex = 0; remainingCellIndex < remainingCells.numValues; remainingCellIndex++) {
                                        mine_cell *remainingCell = remainingCells.values[remainingCellIndex];
                                        revealCell(remainingCell);
                                    }
                                }
                            }

                            // deduce flag cell: check if a (partial) group of considered cells is know to contain X flags: then decremtn num remaining flags
                            // if num remaining flags = num remaining considered cells: flag those cells
                            if (!didAction) {
                                if (remainingCells.numValues > 0 && remainingCells.numValues == (cell->numRemainingBombs - cellGroup.numBombs)) {
                                    didAction = true;
                                    for (i32 remainingCellIndex = 0; remainingCellIndex < remainingCells.numValues; remainingCellIndex++) {
                                        mine_cell *remainingCell = remainingCells.values[remainingCellIndex];
                                        remainingCell->flagged = true;
                                    }
                                }
                            }

                            restoreScratchMemPointer(msScratchMemory, &cellMemorySave);

                            if (didAction) {
                                break;
                            }
                        }
                    }
                    if (didAction) {
                        break;
                    } 
                }
            }

            if (didAction) {
                break;
            }
        }

        if (!didAction) {
            // if all open cells with adj bombs considered and no options: pick an adj unflagged cell
            // randomly and open it (guess)
            //if (unsolvedCells.numValues > 0) {
                mine_cell *cell = unsolvedCells.values[0];
                if (cell->adjCells.numValues > 0) {
                    revealCell(cell->adjCells.values[0]);
                }
            }
        }

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

    if (!ms->revealedBomb) {
        ms->aiTimer += dt;
        f32 aiStepTime = 0.1f;
        while (ms->aiTimer >= aiStepTime) {
            ms->aiTimer -= aiStepTime;
            doAIAction();
        }
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
