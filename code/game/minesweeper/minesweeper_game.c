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
            cell->cellGroupIndices = i32_listInit(memory, 8);
        }
    }

    ms->pointerX = 200.0f;
    ms->pointerY = 200.0f;

    ms->aiState = AI_STATE_THINKING;
    ms->moveDuration = 1.0f;
    ms->state = MS_GAME_STATE_TITLE;
}

void initGrid () {
    for (int i = 0; i < ms->gridHeight; ++i) {
        for (int j = 0; j < ms->gridWidth; ++j) {
            mine_cell *cell = &ms->cells[i * ms->gridWidth + j];
            cell->row = i;
            cell->col = j;
            cell->hasBomb = false;
            cell->flagged = false;
            cell->solved = false;
            cell->cantHaveBomb = false;
            cell->open = false;
            cell->numAdjBombs = 0;
        }
    }
    ms->firstClick = false;
    ms->aiState = AI_STATE_THINKING;
    ms->aiTimer = 0.0f;
    ms->revealedBomb = false;
    ms->moveDuration = 1.0f;
    ms->gameTimer = 0;
}

void generateBombs (int initialRow, int initialCol) {
    mine_cell *firstCell = &ms->cells[initialRow * ms->gridWidth + initialCol];
    firstCell->cantHaveBomb = true;
    for (int i = -1; i <= 1; ++i) {
        for (int j = -1; j <= 1; ++j) {
            int row = initialRow + i;
            int col = initialCol + j;

            if ((row == initialRow && col == initialCol )|| 
                row < 0 || row >= ms->gridHeight ||
                col < 0 || col >= ms->gridWidth) 
            {
                continue;
            }

            mine_cell *neighbor = &ms->cells[row * ms->gridWidth + col];
            neighbor->cantHaveBomb = true;
        }
    }

    int numBombs = ms->totalNumBombs;
    int numPlacedBombs = 0;
    for (int i = 0; i < ms->gridHeight; ++i) {
        for (int j = 0; j < ms->gridWidth; ++j) {
            mine_cell *cell = &ms->cells[i * ms->gridWidth + j];

            if (!cell->cantHaveBomb && numPlacedBombs < numBombs) {
                cell->hasBomb = true;
                numPlacedBombs++;
            }
        }
    }

    int numCells = ms->gridWidth * ms->gridHeight;
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

    for (int cellIndex = 0; cellIndex < ms->gridWidth * ms->gridHeight; ++cellIndex) {
        mine_cell *cell = &ms->cells[cellIndex];

        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                int row = cell->row + i;
                int col = cell->col + j;

                if ((row == cell->row && col == cell->col )|| 
                    row < 0 || row >= ms->gridHeight ||
                    col < 0 || col >= ms->gridWidth) 
                {
                    continue;
                }

                mine_cell *neighbor = &ms->cells[row * ms->gridWidth + col];
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

    mine_cell_ptr_list cellsToOpen = mine_cell_ptr_listInit(msScratchMemory, ms->gridWidth * ms->gridHeight);
    mine_cell_ptr_listPush(&cellsToOpen, startCell);

    for (int cellIndex = 0; cellIndex < ms->gridWidth * ms->gridHeight; ++cellIndex) {
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
                        row < 0 || row >= ms->gridHeight ||
                        col < 0 || col >= ms->gridWidth) 
                    {
                        continue;
                    }

                    mine_cell *neighbor = &ms->cells[row * ms->gridWidth + col];
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
        int randomRow = (int)(randomU32() % ms->gridHeight);
        int randomCol = (int)(randomU32() % ms->gridWidth);
        mine_cell *cell = &ms->cells[randomRow * ms->gridWidth + randomCol];

        ms->aiAction = AI_ACTION_REVEAL_CELL;
        ms->targetCellRow = cell->row;
        ms->targetCellCol = cell->col;
        //revealCell(cell);
    }
    else {
        scratch_mem_save aiStartMemorySave;
        saveScratchMemPointer(msScratchMemory, &aiStartMemorySave);

        b32 makeMistake = randomF32() < 0.2f;
        b32 fixWrongFlag = randomF32() < 0.3f;

        mine_cell_ptr_list wrongFlagCells = mine_cell_ptr_listInit(msScratchMemory, ms->gridWidth * ms->gridHeight);

        for (int i = 0; i < ms->gridHeight; ++i) {
            for (int j = 0; j < ms->gridWidth; ++j) {
                mine_cell *cell = &ms->cells[i * ms->gridWidth + j];
                cell->cellGroupIndices.numValues = 0;
                if (cell->flagged && !cell->hasBomb) {
                    mine_cell_ptr_listPush(&wrongFlagCells, cell);
                }
            }
        }

        // Get list of (unsolved) open cells with adj bombs
        mine_cell_ptr_list unsolvedCells = mine_cell_ptr_listInit(msScratchMemory, ms->gridWidth * ms->gridHeight);
        for (int i = 0; i < ms->gridHeight; ++i) {
            for (int j = 0; j < ms->gridWidth; ++j) {
                mine_cell *cell = &ms->cells[i * ms->gridWidth + j];
                if (!cell->solved && cell->open && cell->numAdjBombs > 0) {
                    mine_cell_ptr_listPush(&unsolvedCells, cell);
                }
            }
        }
        cell_group_list cellGroups = cell_group_listInit(msScratchMemory, 600);

        // Sort closest to cursor? random shuffle for now
        for (int i = 0; i < unsolvedCells.numValues; ++i) {
            for (int j = i; j > 0; --j) {
                mine_cell *first = unsolvedCells.values[j-1];
                mine_cell *second = unsolvedCells.values[j];

                vec2 cursorPos = (vec2){ .x = ms->pointerX, .y = ms->pointerY };
                vec2 firstPos = (vec2){ .x = first->col * CELL_DIM + 8.0f, .y = first->row * CELL_DIM  + 8.0f };
                vec2 secondPos = (vec2){ .x = second->col * CELL_DIM + 8.0f, .y = second->row * CELL_DIM  + 8.0f };

                vec2 firstToCursor = vec2Subtract(cursorPos, firstPos);
                f32 firstDistToCursor = vec2Length(firstToCursor);
                vec2 secondToCursor = vec2Subtract(cursorPos, secondPos);
                f32 secondDistToCursor = vec2Length(secondToCursor);

                if (secondDistToCursor < firstDistToCursor) {
                    unsolvedCells.values[j-1] = second;
                    unsolvedCells.values[j] = first;
                }
            }
        }

        if (wrongFlagCells.numValues > 0 && fixWrongFlag) {
            for (int cellIndex = wrongFlagCells.numValues - 1; cellIndex >= 0; cellIndex--) {
                mine_cell *cell = wrongFlagCells.values[cellIndex];

                int randomIndex = randomU32() % wrongFlagCells.numValues;
                mine_cell *randomCell = wrongFlagCells.values[randomIndex];
            
                wrongFlagCells.values[cellIndex] = randomCell;
                wrongFlagCells.values[randomIndex] = cell;
            }

            ms->aiAction = AI_ACTION_FLAG_CELL;
            ms->targetCellRow = wrongFlagCells.values[0]->row;
            ms->targetCellCol = wrongFlagCells.values[0]->col;
        }
        else {
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
                            row < 0 || row >= ms->gridHeight ||
                            col < 0 || col >= ms->gridWidth) 
                        {
                            continue;
                        }

                        mine_cell *neighbor = &ms->cells[row * ms->gridWidth + col];
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
                        didAction = true;

                        if (makeMistake) {
                            ms->aiAction = AI_ACTION_FLAG_CELL;
                        }
                        else {
                            ms->aiAction = AI_ACTION_REVEAL_CELL;
                        }

                        // open adj cells
                        for (int adjIndex = adjCells->numValues - 1; adjIndex >= 0; adjIndex--) {
                            mine_cell *adjCell = adjCells->values[adjIndex];

                            int randomIndex = randomU32() % adjCells->numValues;
                            mine_cell *randomCell = adjCells->values[randomIndex];
                        
                            adjCells->values[adjIndex] = randomCell;
                            adjCells->values[randomIndex] = adjCell;
                        }
                        for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                            mine_cell *adjCell = adjCells->values[adjIndex];
                            if (!adjCell->flagged && !adjCell->open) {

                                //revealCell(adjCell);
                                ms->targetCellRow = adjCell->row;
                                ms->targetCellCol = adjCell->col;
                                break;
                            }
                        }
                    }
                }
                // - if num adj unflagged = num adj bombs
                //   - flag all adj cells
                else if (cell->numUnflaggedCells == cell->numRemainingBombs) {
                    if (makeMistake) {
                        ms->aiAction = AI_ACTION_REVEAL_CELL;
                    }
                    else {
                        ms->aiAction = AI_ACTION_FLAG_CELL;
                    }

                    didAction = true;

                    for (int adjIndex = adjCells->numValues - 1; adjIndex >= 0; adjIndex--) {
                        mine_cell *adjCell = adjCells->values[adjIndex];

                        int randomIndex = randomU32() % adjCells->numValues;
                        mine_cell *randomCell = adjCells->values[randomIndex];
                    
                        adjCells->values[adjIndex] = randomCell;
                        adjCells->values[randomIndex] = adjCell;
                    }
                    for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                        mine_cell *adjCell = adjCells->values[adjIndex];
                        if (!adjCell->flagged && !adjCell->open) {
                            //adjCell->flagged = true;
                            ms->targetCellRow = adjCell->row;
                            ms->targetCellCol = adjCell->col;
                            break;
                        }
                    }
                }

                if (didAction) {
                    break;
                }
            }

            if (!didAction) {
                for (int cellIndex = 0; cellIndex < unsolvedCells.numValues; cellIndex++) {
                    mine_cell *cell = unsolvedCells.values[cellIndex];
                    mine_cell_ptr_list *adjCells = &cell->adjCells;

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
                                        ms->aiAction = AI_ACTION_REVEAL_CELL;


                                        for (int remainingCellIndex = remainingCells.numValues - 1; remainingCellIndex >= 0; remainingCellIndex--) {
                                            mine_cell *remainingCell = remainingCells.values[remainingCellIndex];

                                            int randomIndex = randomU32() % remainingCells.numValues;
                                            mine_cell *randomCell = remainingCells.values[randomIndex];
                                        
                                            remainingCells.values[remainingCellIndex] = randomCell;
                                            remainingCells.values[randomIndex] = remainingCell;
                                        }
                                        for (i32 remainingCellIndex = 0; remainingCellIndex < remainingCells.numValues; remainingCellIndex++) {
                                            mine_cell *remainingCell = remainingCells.values[remainingCellIndex];
                                            ms->targetCellRow = remainingCell->row;
                                            ms->targetCellCol = remainingCell->col;
                                            //revealCell(remainingCell);
                                        }
                                    }
                                }

                                // deduce flag cell: check if a (partial) group of considered cells is know to contain X flags: then decremtn num remaining flags
                                // if num remaining flags = num remaining considered cells: flag those cells
                                if (!didAction) {
                                    if (remainingCells.numValues > 0 && remainingCells.numValues == (cell->numRemainingBombs - cellGroup.numBombs)) {
                                        didAction = true;
                                        ms->aiAction = AI_ACTION_FLAG_CELL;
                                        for (int remainingCellIndex = remainingCells.numValues - 1; remainingCellIndex >= 0; remainingCellIndex--) {
                                            mine_cell *remainingCell = remainingCells.values[remainingCellIndex];

                                            int randomIndex = randomU32() % remainingCells.numValues;
                                            mine_cell *randomCell = remainingCells.values[randomIndex];
                                        
                                            remainingCells.values[remainingCellIndex] = randomCell;
                                            remainingCells.values[randomIndex] = remainingCell;
                                        }
                                        for (i32 remainingCellIndex = 0; remainingCellIndex < remainingCells.numValues; remainingCellIndex++) {
                                            mine_cell *remainingCell = remainingCells.values[remainingCellIndex];
                                            ms->targetCellRow = remainingCell->row;
                                            ms->targetCellCol = remainingCell->col;
                                            //remainingCell->flagged = true;
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

                    if (didAction) {
                        break;
                    } 
                }
            }

            if (!didAction) {
                // if all open cells with adj bombs considered and no options: pick an adj unflagged cell
                // randomly and open it (guess)
                if (unsolvedCells.numValues > 0) {
                    mine_cell *cell = unsolvedCells.values[0];
                    if (cell->adjCells.numValues > 0) {

                        mine_cell_ptr_list *adjCells = &cell->adjCells;
                        for (int adjIndex = adjCells->numValues - 1; adjIndex >= 0; adjIndex--) {
                            mine_cell *adjCell = adjCells->values[adjIndex];

                            int randomIndex = randomU32() % adjCells->numValues;
                            mine_cell *randomCell = adjCells->values[randomIndex];
                        
                            adjCells->values[adjIndex] = randomCell;
                            adjCells->values[randomIndex] = adjCell;
                        }
                        for (int adjIndex = 0; adjIndex < adjCells->numValues; adjIndex++) {
                            mine_cell *adjCell = adjCells->values[adjIndex];
                            if (!adjCell->flagged) {
                                ms->aiAction = AI_ACTION_REVEAL_CELL;
                                ms->targetCellRow = adjCell->row;
                                ms->targetCellCol = adjCell->col;
                                break;
                            }
                        }

                        //revealCell(cell->adjCells.values[0]);
                    }
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

    switch (ms->state) {
        case MS_GAME_STATE_TITLE: {
            if (input->pointerJustDown) {

                mat3x3 gameTransform = spriteManPeekMatrix();
                vec3 localPointerPos = (vec3){ .x = (f32)input->pointerX, 
                                               .y = (f32)input->pointerY, 
                                               .z = 1.0f };
                localPointerPos = vec3MatrixMul(mat3x3Inv(gameTransform), localPointerPos);

                if (localPointerPos.x >= 267.0f && localPointerPos.x < 356.0f && 
                    localPointerPos.y >= 180.0f && localPointerPos.y < 206.0f) 
                {
                    ms->gridWidth = 9;
                    ms->gridHeight = 9;
                    ms->totalNumBombs = 10;
                    ms->gridStartX = 246.0f;
                    ms->gridStartY = 104.0f;
                    initGrid();
                    ms->state = MS_GAME_STATE_GAMEPLAY;
                }
                else if (localPointerPos.x >= 267.0f && localPointerPos.x < 356.0f && 
                         localPointerPos.y >= 216.0f && localPointerPos.y < 242.0f) 
                {
                    ms->gridWidth = 16;
                    ms->gridHeight = 16;
                    ms->totalNumBombs = 40;
                    ms->gridStartX = 192.0f;
                    ms->gridStartY = 72.0f;
                    initGrid();
                    ms->state = MS_GAME_STATE_GAMEPLAY;
                }
                else if (localPointerPos.x >= 267.0f && localPointerPos.x < 356.0f && 
                         localPointerPos.y >= 252.0f && localPointerPos.y < 278.0f) 
                {
                    ms->gridWidth = GRID_WIDTH;
                    ms->gridHeight = GRID_HEIGHT;
                    ms->totalNumBombs = 99;
                    ms->gridStartX = 80.0f;
                    ms->gridStartY = 56.0f;
                    initGrid();
                    ms->state = MS_GAME_STATE_GAMEPLAY;
                }
            }
        } break;
        case MS_GAME_STATE_GAMEPLAY: {
            if (!ms->revealedBomb) {
                ms->gameTimer += dt;
                ms->aiTimer += dt;
                switch (ms->aiState) {
                    case AI_STATE_THINKING: {
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiTimer = 0.0f;
                            doAIAction();

                            ms->aiState = AI_STATE_THOUGHT_BUBBLE;
                            ms->aiTimer = 0.0f;
                            //ms->moveDuration = 0.01f;
                            ms->moveDuration = 1.0f;
                            if (ms->gameTimer > 30.0f) {
                                ms->moveDuration = 0.75f;
                            }
                            if (ms->gameTimer > 100.0f) {
                                ms->moveDuration = 0.65f;
                            }
                        }
                    } break;
                    case AI_STATE_THOUGHT_BUBBLE: {
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiState = AI_STATE_MOVING;
                            ms->aiTimer = 0.0f;
                            //ms->moveDuration = 0.01f;
                            ms->moveDuration = 0.2f;
                            ms->moveStartPos = (vec2){ .x = ms->pointerX, .y = ms->pointerY };
                            ms->moveEndPos = (vec2){
                                .x = ms->targetCellCol * 16.0f + 8.0f,
                                .y = ms->targetCellRow * 16.0f + 8.0f,
                            };
                        }
                    } break;
                    case AI_STATE_MOVING: {
                        b32 moveDone = false;
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiTimer = ms->moveDuration;
                            moveDone = true;
                        }
                        f32 t = ms->aiTimer / ms->moveDuration;

                        if (t < 0.5f) {
                            t = 4.0f * t * t * t;
                        }
                        else {
                            t = (t - 1.0f) * (2.0f * t - 2.0f) * (2.0f * t - 2.0f) + 1.0f; 
                        }

                        ms->pointerX = (1.0f - t) * ms->moveStartPos.x + t * ms->moveEndPos.x;
                        ms->pointerY = (1.0f - t) * ms->moveStartPos.y + t * ms->moveEndPos.y;

                        if (moveDone) {
                            ms->aiState = AI_STATE_CLICK_DELAY;
                            ms->aiTimer = 0.0f;
                            //ms->moveDuration = 0.01f;
                            ms->moveDuration = 1.25f + randomF32() * 1.5f;
                            if (ms->gameTimer > 60.0f) {
                                ms->moveDuration = 1.25f + randomF32() * 1.0f;
                            }
                            if (ms->gameTimer > 100.0f) {
                                ms->moveDuration = 1.25f + randomF32() * 0.5f;
                            }
                        }
                    } break;
                    case AI_STATE_CLICK_DELAY: {
                        b32 waitDone = false;
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiTimer = ms->moveDuration;
                            waitDone = true;
                        }
                        if (waitDone) {
                            ms->aiState = AI_STATE_CLICK;
                            ms->aiTimer = 0.0f;
                        }
                    } break;
                    case AI_STATE_CLICK: {
                        int mouseRow = (int)(ms->pointerY / CELL_DIM);
                        int mouseCol = (int)(ms->pointerX / CELL_DIM);

                        if (mouseRow >= 0 && mouseRow < ms->gridHeight && mouseCol >= 0 && mouseCol < ms->gridWidth) {
                            mine_cell *cell = &ms->cells[mouseRow * ms->gridWidth + mouseCol];
                            if (ms->aiAction == AI_ACTION_REVEAL_CELL) {
                                revealCell(cell);
                            }
                            else if (ms->aiAction == AI_ACTION_FLAG_CELL) {
                                if (!cell->open) {
                                    cell->flagged = !cell->flagged;
                                }
                            }
                        }
                        soundManPlaySound("click");

                        ms->aiState = AI_STATE_JUST_CLICKED;
                        ms->aiTimer = 0.0f;
                        //ms->moveDuration = 0.01f;
                        ms->moveDuration = 0.125;
                     } break;
                    case AI_STATE_JUST_CLICKED: {
                        b32 waitDone = false;
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiTimer = ms->moveDuration;
                            waitDone = true;
                        }

                        if (waitDone) {
                            ms->aiState = AI_STATE_THINK_DELAY;
                            ms->aiTimer = 0.0f;
                            //ms->moveDuration = 0.01;
                            ms->moveDuration = 0.125f;
                        }
                    } break;
                    case AI_STATE_THINK_DELAY: {
                        b32 waitDone = false;
                        if (ms->aiTimer >= ms->moveDuration) {
                            ms->aiTimer = ms->moveDuration;
                            waitDone = true;
                        }

                        if (waitDone) {
                            ms->aiState = AI_STATE_THINKING;
                            ms->aiTimer = 0.0f;
                            //ms->moveDuration = 0.01f;
                            ms->moveDuration = 0.75f * randomF32() * 1.25f;
                            if (ms->gameTimer > 30.0f) {
                                ms->moveDuration = 0.5f + randomF32() * 1.25f;
                            }
                            if (ms->gameTimer > 60.0f) {
                                ms->moveDuration = 0.5f + randomF32() * 1.0f;
                            }
                            if (ms->gameTimer > 90.0f) {
                                ms->moveDuration = 0.25f + randomF32() * 0.75f;
                            }
                            if (ms->gameTimer > 120.0f) {
                                ms->moveDuration = 0.25f + randomF32() * 0.5f;
                            }
                            if (ms->gameTimer > 150.0f) {
                                ms->moveDuration = 0.25f + randomF32() * 0.25f;
                            }
                            if (ms->gameTimer > 180.0f) {
                                ms->moveDuration = 0.125f + randomF32() * 0.125f;
                            }
                        }
                    } break;
                }
            }

            ms->bumpingLeft = false;
            ms->bumpingRight = false;
            ms->bumpingUp = false;
            ms->bumpingDown = false;

            if (msInput.left.down) {
                ms->bumpingLeft = true;
            }
            if (msInput.right.down) {
                ms->bumpingRight = true;
            }
            if (msInput.up.down) {
                ms->bumpingUp = true;
            }
            if (msInput.down.down) {
                ms->bumpingDown = true;
            }

            if (msInput.left.justPressed) {
                ms->pointerX -= 2.0f;
                ms->bumpAmtX -= 5.0f;
                soundManPlaySound("impact");
            }
            if (msInput.right.justPressed) {
                ms->pointerX += 2.0f;
                ms->bumpAmtX += 5.0f;
                soundManPlaySound("impact");
            }
            if (msInput.up.justPressed) {
                ms->pointerY -= 2.0f;
                ms->bumpAmtY -= 5.0f;
                soundManPlaySound("impact");
            }
            if (msInput.down.justPressed) {
                ms->pointerY += 2.0f;
                ms->bumpAmtY += 5.0f;
                soundManPlaySound("impact");
            }

            if (ms->bumpAmtX < 0.0f) {
                ms->bumpAmtX += 50.0f * dt;
                if (ms->bumpAmtX >= 0.0f) {
                    ms->bumpAmtX = 0.0f;
                }
            }
            if (ms->bumpAmtX > 0.0f) {
                ms->bumpAmtX -= 50.0f * dt;
                if (ms->bumpAmtX <= 0.0f) {
                    ms->bumpAmtX = 0.0f;
                }
            }
            if (ms->bumpAmtY < 0.0f) {
                ms->bumpAmtY += 50.0f * dt;
                if (ms->bumpAmtY >= 0.0f) {
                    ms->bumpAmtY = 0.0f;
                }
            }
            if (ms->bumpAmtY > 0.0f) {
                ms->bumpAmtY -= 50.0f * dt;
                if (ms->bumpAmtY <= 0.0f) {
                    ms->bumpAmtY = 0.0f;
                }
            }



            i32 numClosedCells = 0;
            for (int i = 0; i < ms->gridHeight; ++i) {
                for (int j = 0; j < ms->gridWidth; ++j) {
                    mine_cell *cell = &ms->cells[i * ms->gridWidth + j];
                    if (!cell->open) {
                        numClosedCells++;
                    }
                }
            }
            if (numClosedCells == ms->totalNumBombs) {
                ms->state = MS_GAME_STATE_WIN;
            }

            if (ms->revealedBomb) {
                ms->state = MS_GAME_STATE_LOSE;
            }
        } break;
        case MS_GAME_STATE_WIN: 
        case MS_GAME_STATE_LOSE: {
            if (input->pointerJustDown) {
                mat3x3 gameTransform = spriteManPeekMatrix();
                vec3 localPointerPos = (vec3){ .x = (f32)input->pointerX, 
                                               .y = (f32)input->pointerY, 
                                               .z = 1.0f };
                localPointerPos = vec3MatrixMul(mat3x3Inv(gameTransform), localPointerPos);

                if (localPointerPos.x >= 267.0f && localPointerPos.x < 356.0f && 
                    localPointerPos.y >= 180.0f && localPointerPos.y < 206.0f) 
                {
                    ms->state = MS_GAME_STATE_TITLE;
                }
            }

        } break;
    }
}

void drawMinesweeper (plat_api platAPI, f32 gameScale, mem_arena *scratchMemory) {
    switch (ms->state) {
        case MS_GAME_STATE_TITLE: {
            spriteManAddText((sprite_text){
                .x = 270.0f,
                .y = 140.0f,
                .text = "mind the mines",
                .fontKey = "font"
            });
            //spriteManAddText((sprite_text){
            //    .x = 258.0f,
            //    .y = 190.0f,
            //    .text = "click to continue",
            //    .fontKey = "font"
            //});

            sprite button = defaultSprite();
            button.pos.x = 267.0f;
            button.pos.y = 180.0f;
            button.atlasKey = "atlas";
            button.frameKey = "button";
            spriteManAddSprite(button);

            spriteManAddText((sprite_text){
                .x = 270.0f,
                .y = button.pos.y + 10,
                .text = "beginner",
                .fontKey = "font"
            });

            button.pos.y = 216.0f;
            spriteManAddSprite(button);

            spriteManAddText((sprite_text){
                .x = 270.0f,
                .y = button.pos.y + 10,
                .text = "intermediate",
                .fontKey = "font"
            });

            button.pos.y = 252.0f;
            spriteManAddSprite(button);

            spriteManAddText((sprite_text){
                .x = 270.0f,
                .y = button.pos.y + 10,
                .text = "expert",
                .fontKey = "font"
            });
        } break;
        case MS_GAME_STATE_WIN:
        case MS_GAME_STATE_LOSE:
        case MS_GAME_STATE_GAMEPLAY: {
            if (ms->state == MS_GAME_STATE_GAMEPLAY) {
                spriteManPushTransform((sprite_transform){
                    .pos = (vec2) {.x = 0, .y = ms->bumpAmtY },
                    .scale = 1.0f
                });
                spriteManPushTransform((sprite_transform){
                    .pos = (vec2) {.x = ms->bumpAmtX, .y = 0 },
                    .scale = 1.0f
                });

                spriteManPushTransform((sprite_transform){
                    .pos = (vec2) {.x = 320.0f, .y = 180.0f },
                    .scale = 1.0f
                });
                spriteManPushTransform((sprite_transform){
                    .rotation = ms->bumpAmtX * 0.00125f,
                    .scale = 1.0f
                });
                spriteManPushTransform((sprite_transform){
                    .pos = (vec2) {.x = -320.0f, .y = -180.0f },
                    .scale = 1.0f
                });
            }

            for (int i = 0; i < ms->gridHeight; ++i) {
                for (int j = 0; j < ms->gridWidth; ++j) {
                    mine_cell *cell = &ms->cells[i * ms->gridWidth + j];

                    sprite cellSprite = defaultSprite();
                    cellSprite.pos.x = ms->gridStartX + (float)(cell->col * CELL_DIM);
                    cellSprite.pos.y = ms->gridStartY + (float)(cell->row * CELL_DIM);
                    cellSprite.atlasKey = "atlas";

                    if (!cell->open) {
                        cellSprite.frameKey = "closed_cell";
                        spriteManAddSprite(cellSprite);

                        if (ms->state == MS_GAME_STATE_LOSE) {
                            if (cell->flagged) {
                                if (!cell->hasBomb) {
                                    cellSprite.frameKey = "bomb";
                                    spriteManAddSprite(cellSprite);
                                    cellSprite.frameKey = "x";
                                    spriteManAddSprite(cellSprite);
                                }
                                else {
                                    cellSprite.frameKey = "flag";
                                    spriteManAddSprite(cellSprite);
                                }
                            }
                            else {
                                if (cell->hasBomb) {
                                    cellSprite.frameKey = "bomb";
                                    spriteManAddSprite(cellSprite);
                                }
                            }
                        }
                        else {
                            if (cell->flagged) {
                                cellSprite.frameKey = "flag";
                                spriteManAddSprite(cellSprite);
                            }
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

            sprite smileyFrame = defaultSprite();
            smileyFrame.pos.x = 300.0f;
            smileyFrame.pos.y = 8.0f;
            smileyFrame.atlasKey = "atlas";
            smileyFrame.frameKey = "smiley_frame";
            spriteManAddSprite(smileyFrame);

            sprite smiley = defaultSprite();

            smiley.pos.x = 300.0f;
            if (ms->bumpingLeft) {
                smiley.pos.x -= 3.0f;
            }
            if (ms->bumpingRight) {
                smiley.pos.x += 3.0f;
            }

            smiley.pos.y = 8.0f;
            if (ms->bumpingUp) {
                smiley.pos.y -= 3.0f;
            }
            if (ms->bumpingDown) {
                smiley.pos.y += 3.0f;
            }

            smiley.atlasKey = "atlas";
            if (ms->state == MS_GAME_STATE_WIN) {
                smiley.frameKey = "smiley_sunglasses";
            }
            else {
                if (ms->revealedBomb) {
                    smiley.frameKey = "smiley_dead";
                }
                else {
                    smiley.frameKey = "smiley";
                }
            }
            spriteManAddSprite(smiley);

            sprite bombsFrame = defaultSprite();
            bombsFrame.pos.x = 300.0f - 62.0f - 50.0f;
            bombsFrame.pos.y = 8.0f;
            bombsFrame.atlasKey = "atlas";
            bombsFrame.frameKey = "number_backing";
            spriteManAddSprite(bombsFrame);

            i32 numFlaggedCells = 0;
            for (int i = 0; i < ms->gridHeight; ++i) {
                for (int j = 0; j < ms->gridWidth; ++j) {
                    mine_cell *cell = &ms->cells[i * ms->gridWidth + j];
                    if (cell->flagged) {
                        numFlaggedCells++;
                    }
                }
            }
            i32 numBombsRemaining = ms->totalNumBombs - numFlaggedCells;

            sprite digit = defaultSprite();
            digit.pos.x = bombsFrame.pos.x + 41.0f;
            digit.pos.y = 12.0f;
            digit.atlasKey = "atlas";
            digit.frameKey = tempStringAppend("led_", tempStringFromI32(numBombsRemaining % 10));
            spriteManAddSprite(digit);

            if (numBombsRemaining >= 10) {
                digit.pos.x = bombsFrame.pos.x + 23.0f;
                digit.frameKey = tempStringAppend("led_", tempStringFromI32(numBombsRemaining / 10));
                spriteManAddSprite(digit);
            }

            sprite timerFrame = defaultSprite();
            timerFrame.pos.x = 300.0f + 38.0f + 50.0f;
            timerFrame.pos.y = 8.0f;
            timerFrame.atlasKey = "atlas";
            timerFrame.frameKey = "number_backing";
            spriteManAddSprite(timerFrame);

            i32 seconds = (i32)ms->gameTimer;
            sprite secondSprite = defaultSprite();
            secondSprite.pos.x = timerFrame.pos.x + 41.0f;
            secondSprite.pos.y = 12.0f;
            secondSprite.atlasKey = "atlas";
            secondSprite.frameKey = tempStringAppend("led_", tempStringFromI32(seconds % 10));
            spriteManAddSprite(secondSprite);

            if (seconds >= 10) {
                secondSprite.pos.x = timerFrame.pos.x + 23.0f;
                secondSprite.frameKey = tempStringAppend("led_", tempStringFromI32((seconds / 10) % 10));
                spriteManAddSprite(secondSprite);
            }

            if (seconds >= 100) {
                secondSprite.pos.x = timerFrame.pos.x + 5.0f;
                secondSprite.frameKey = tempStringAppend("led_", tempStringFromI32(seconds / 100));
                spriteManAddSprite(secondSprite);
            }

            sprite pointerSprite = defaultSprite();
            pointerSprite.pos.x = ms->gridStartX + ms->pointerX;
            pointerSprite.pos.y = ms->gridStartY + ms->pointerY;
            pointerSprite.atlasKey = "atlas";
            pointerSprite.frameKey = "pointer";
            spriteManAddSprite(pointerSprite);

            if (ms->aiState == AI_STATE_THOUGHT_BUBBLE) {
                sprite bubbleSprite = defaultSprite();
                bubbleSprite.pos.x = pointerSprite.pos.x + 10.0f;
                bubbleSprite.pos.y = pointerSprite.pos.y - 25.0f;
                bubbleSprite.atlasKey = "atlas";

                if (ms->aiAction == AI_ACTION_FLAG_CELL) {
                    bubbleSprite.frameKey = "bubble_flag";
                }
                else {
                    bubbleSprite.frameKey = "bubble_click";
                }
                spriteManAddSprite(bubbleSprite);
            }
            else if (ms->aiState == AI_STATE_THINKING) {
                sprite bubbleSprite = defaultSprite();
                bubbleSprite.pos.x = pointerSprite.pos.x + 10.0f;
                bubbleSprite.pos.y = pointerSprite.pos.y - 25.0f;
                bubbleSprite.atlasKey = "atlas";
                bubbleSprite.frameKey = "bubble_dots";
                spriteManAddSprite(bubbleSprite);
            }
            else if (ms->aiState == AI_STATE_JUST_CLICKED) {
                sprite clickSprite = defaultSprite();
                clickSprite.pos.x = pointerSprite.pos.x - 15.0f;
                clickSprite.pos.y = pointerSprite.pos.y - 20.0f;
                clickSprite.atlasKey = "atlas";
                clickSprite.frameKey = "click";
                spriteManAddSprite(clickSprite);
            }


            if (ms->state == MS_GAME_STATE_GAMEPLAY) {
                spriteManPopMatrix();
                spriteManPopMatrix();
                spriteManPopMatrix();
                spriteManPopMatrix();
                spriteManPopMatrix();
            }

            if (ms->state == MS_GAME_STATE_WIN || ms->state== MS_GAME_STATE_LOSE) {
                sprite button = defaultSprite();
                button.pos.x = 267.0f;
                button.pos.y = 180.0f;
                button.atlasKey = "atlas";
                button.frameKey = "button";
                spriteManAddSprite(button);

                spriteManAddText((sprite_text){
                    .x = 270.0f,
                    .y = button.pos.y + 10,
                    .text = "play again",
                    .fontKey = "font"
                });
            }
        } break;
    }

}
