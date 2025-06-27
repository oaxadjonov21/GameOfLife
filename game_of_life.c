#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>

// Program code constants
const int ERROR_NOMEMORY = -1;
const int WIN_SIZE_ERROR = -2;
const int LIFE_ERROR = -3;
const int SUCCESS = 0;
const int FAIL = 1;
const int TERMINATE = -4;
const int INPUT_FAIL = -5;
const int SUCCESS_BREAK = 3;
// Constant (NOT variables!) parameters
const int BOARD_COLS = 80;
const int BOARD_ROWS = 25;
const int FSTATE_DEFAULT = -1;
const int DEAD = 0;
const int ALIVE = 1;
const char ALIVE_CELL = '*';
const char DEAD_CELL = ' ';
const char SPEED_UP = 'a';
const char SPEED_DOWN = 'z';
const char TERMINATE_KEY = ' ';
const int DELAY_VAL = 200;
const int TIMEOUT_VAL = 50;
const int TIME_DELTA = 20;
const int INPUT_TERMINATE = -1;

// NCURSES window additional
// functions
int windowSizeCheck(int rows, int cols);

// Game functions
void initNcurses();
void endNcurses();
int initFromFile(int **lifeGrid, int rows, int cols);
int life(int rows, int cols);
int createLife(int ***lifeGrid, int ***futureStates, int rows, int cols);
void prepareNextGen(int **lifeGrid, int **futureStates, int rows, int cols);
void deployNextGen(int **lifeGrid, int **futureStates, int rows, int cols);
void resetFutureStates(int **futureStates, int rows, int cols);
void printLifeGrid(int **lifeGrid, int row, int cols);
int countNeighbors(int **lifeGrid, int rows, int cols, int cellRow, int cellCol);
int handleKey(int *delayValue);
int validateNextChar();

// Core matrix functions
int **createMatrix(int rows, int cols);
void freeMatrix(int **matrix, int rows);
void populateMatrix(int **matrix, int rows, int cols, int value);
void freeMatrices(int **matrix1, int **matrix2, int rows);

void println();

int main(void) {
    int lifeCode = life(BOARD_ROWS, BOARD_COLS);

    return lifeCode;
}

void initNcurses() {
    initscr();
    raw();
    cbreak();
    noecho();
}

void endNcurses() { endwin(); }

int life(int rows, int cols) {
    int **lifeGrid = NULL;
    int **futureStates = NULL;
    int timeoutValue = TIMEOUT_VAL;
    int delayValue = DELAY_VAL;

    int status = createLife(&lifeGrid, &futureStates, rows, cols);

    initNcurses();

    if (status == SUCCESS && windowSizeCheck(rows, cols) == WIN_SIZE_ERROR) {
        freeMatrices(lifeGrid, futureStates, rows);
        status = LIFE_ERROR;
    }

    // Wait time for user input
    timeout(timeoutValue);
    while (status == SUCCESS) {
        clear();
        printLifeGrid(lifeGrid, rows, cols);
        refresh();

        if (handleKey(&delayValue) == TERMINATE) {
            break;
        }

        prepareNextGen(lifeGrid, futureStates, rows, cols);
        deployNextGen(lifeGrid, futureStates, rows, cols);
        resetFutureStates(futureStates, rows, cols);

        napms(delayValue);
    }

    freeMatrices(lifeGrid, futureStates, rows);
    getch();
    endNcurses();

    return status;
}

void freeMatrices(int **matrix1, int **matrix2, int rows) {
    freeMatrix(matrix1, rows);
    freeMatrix(matrix2, rows);
}

int createLife(int ***lifeGrid, int ***futureStates, int rows, int cols) {
    int status = SUCCESS;

    *lifeGrid = createMatrix(rows, cols);
    if (*lifeGrid == NULL) {
        status = ERROR_NOMEMORY;
    }

    *futureStates = createMatrix(rows, cols);
    if (*futureStates == NULL) {
        freeMatrix(*lifeGrid, rows);
        status = ERROR_NOMEMORY;
    }

    if (status == SUCCESS) {
        populateMatrix(*lifeGrid, rows, cols, DEAD);
        populateMatrix(*futureStates, rows, cols, FSTATE_DEFAULT);

        int initStatus = initFromFile(*lifeGrid, rows, cols);

        if (initStatus == INPUT_FAIL) {
            freeMatrix(*lifeGrid, rows);
            freeMatrix(*futureStates, rows);
            status = INPUT_FAIL;
        }
    }

    return status;
}

int initFromFile(int **lifeGrid, int rows, int cols) {
    int status = SUCCESS;
    while (status == SUCCESS) {
        int res;
        int cellRow = 0;
        int cellCol = 0;

        res = scanf("%d %d", &cellRow, &cellCol);
        if (res != 2) {
            status = INPUT_FAIL;
        }

        if (status == SUCCESS && (cellRow == INPUT_TERMINATE || cellCol == INPUT_TERMINATE)) {
            status = SUCCESS_BREAK;
        }

        if (status == SUCCESS && (cellRow < 0 || cellRow >= rows || cellCol < 0 || cellCol >= cols)) {
            status = INPUT_FAIL;
        }

        if (status == SUCCESS) {
            res = validateNextChar();

            if (res == FAIL) {
                status = INPUT_FAIL;
            }
        }

        if (status == SUCCESS) {
            lifeGrid[cellRow][cellCol] = ALIVE;
        }
    }
    freopen("/dev/tty", "r", stdin);

    return status;
}

int validateNextChar() {
    int status = SUCCESS;
    int nextChar = getchar();
    if ((nextChar != ' ') && (nextChar != '\n') && (nextChar != EOF)) {
        status = FAIL;
    }
    return status;
}

// Window functions
int windowSizeCheck(int rows, int cols) {
    int code = SUCCESS;
    int winRows = -1;
    int winCols = -1;
    getmaxyx(stdscr, winRows, winCols);

    if (winRows <= rows || winCols <= cols) {
        char err_msg[] = "Window size is not enough.";
        printw("%s\n", err_msg);
        printw("Current size: %d x %d\n", winRows, winCols);
        printw("Required minimum size: %d x %d", rows, cols);
        refresh();

        code = WIN_SIZE_ERROR;
    }
    return code;
}

/*
 *  Cell-state related functions
 */

void prepareNextGen(int **lifeGrid, int **futureStates, int rows, int cols) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int aliveNeighbors = countNeighbors(lifeGrid, rows, cols, row, col);
            int currentCell = lifeGrid[row][col];

            if (currentCell == DEAD && aliveNeighbors == 3) {
                futureStates[row][col] = ALIVE;
            }

            if (currentCell == DEAD && aliveNeighbors != 3) {
                continue;
            }

            if (currentCell == ALIVE && aliveNeighbors < 2) {
                futureStates[row][col] = DEAD;
                continue;
            }

            if (currentCell == ALIVE && aliveNeighbors > 3) {
                futureStates[row][col] = DEAD;
                continue;
            }
            // NO CONDITION: here by default survives
        }
    }
}

void deployNextGen(int **lifeGrid, int **futureStates, int rows, int cols) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            int currentCellState = futureStates[row][col];
            if (currentCellState == FSTATE_DEFAULT) {
                continue;
            }
            lifeGrid[row][col] = currentCellState;
        }
    }
}

int countNeighbors(int **lifeGrid, int rows, int cols, int cellRow, int cellCol) {
    /*  #-> -- 2 chars away from hash tag, thus: row < cellRow - 1 + 3
     *  -O>
     *  -->
     * */
    int aliveNeighbors = 0;

    for (int row = cellRow - 1; row < cellRow + 2; row++) {
        for (int col = cellCol - 1; col < cellCol + 2; col++) {
            if (row == cellRow && col == cellCol) {
                continue;
            }

            // Tortoidal border elimination
            int wrappedRow = row;
            int wrappedCol = col;

            if (row < 0) {
                wrappedRow = rows - 1;
            } else if (row >= rows) {
                wrappedRow = 0;
            }

            if (col < 0) {
                wrappedCol = cols - 1;
            } else if (col >= cols) {
                wrappedCol = 0;
            }

            int currentCell = lifeGrid[wrappedRow][wrappedCol];
            if (currentCell == ALIVE) {
                aliveNeighbors++;
            }
        }
    }
    return aliveNeighbors;
}

void resetFutureStates(int **futureStates, int rows, int cols) {
    populateMatrix(futureStates, rows, cols, FSTATE_DEFAULT);
    return;
}

/*
 * Printing functions
 */

void printLifeGrid(int **lifeGrid, int rows, int cols) {
    /* Printing borders should be REVISED!!! */
    for (int i = 0; i < cols + 5; i++) {
        printw("~");
    }
    println();
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            if (col == 0) {
                printw("| ");
            }
            if (lifeGrid[row][col] == ALIVE) {
                printw("%c", ALIVE_CELL);
            } else {
                printw("%c", DEAD_CELL);
            }
            if (col == cols - 1) {
                printw(" |");
            }
        }
        if (row < rows - 1) {
            println();
        }
    }
    println();
    for (int i = 0; i < cols + 5; i++) {
        printw("~");
    }
    println();
    printw("A-Z increase/decrease speed, Space Bar-quit\n");
}

void println() { printw("\n"); }

/*
 * KEY management functions
 */

int handleKey(int *delayValue) {
    int status = SUCCESS;
    int key = getch();

    if (key == ERR) {
        status = SUCCESS_BREAK;
    }

    if (key == TERMINATE_KEY) {
        status = TERMINATE;
    } else if (key == SPEED_UP) {
        if (*delayValue - TIME_DELTA < 0) {
            *delayValue = 0;
        } else {
            *delayValue -= TIME_DELTA;
        }

    } else if (key == SPEED_DOWN) {
        *delayValue += TIME_DELTA;
    }
    return status;
}

int toLower(int key) {
    if (key >= 65 && key <= 90) {
        key += 32;
    }

    return key;
}

/*
 * PROTECTED AREA!!!
 * DO NOT TOUCH UNLESS YOU KNOW WHAT YOU ARE DOING!
 */
int **createMatrix(int rows, int cols) {
    int **matrix = malloc(sizeof(int *) * rows);

    if (matrix != NULL) {
        for (int row = 0; row < rows; row++) {
            int *rowPtr = malloc(sizeof(int) * cols);

            if (rowPtr == NULL) {
                freeMatrix(matrix, row);
                matrix == NULL;
                break;
            }

            matrix[row] = rowPtr;
        }
    }

    return matrix;
}

/*
 * Implementation specific
 * 2D matrix deallocator
 */
void freeMatrix(int **matrix, int rows) {
    if (matrix == NULL) {
        return;
    }
    for (int row = 0; row < rows; row++) {
        if (matrix[row] == NULL) {
            continue;
        }
        free(matrix[row]);
    }
    refresh();
    free(matrix);
}

/*
 * Function to populate a matrix
 * with a given *value*
 */
void populateMatrix(int **matrix, int rows, int cols, int value) {
    for (int row = 0; row < rows; row++) {
        for (int col = 0; col < cols; col++) {
            matrix[row][col] = value;
        }
    }
}
