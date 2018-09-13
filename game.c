
#define _GNU_SOURCE
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include "main_aux.h"
#include "parser.h"
#include "solver.h"
#include "game.h"
#include "stack.h"
#include "linked_list.h"
#include <unistd.h>

extern int blockRows;
extern int blockCols;
extern int markErrors;


cell ** generate_empty_board(){
    int i, j, N;
    cell **board = NULL;

    N =  blockRows * blockCols;
    board = calloc(N, sizeof (*board));
    for (i = 0; i < N; i++) {
        board[i] = calloc(N, sizeof (**board));
    }
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            board[i][j].number = UNASSIGNED;
            board[i][j].isFixed = false;
            board[i][j].asterisk = false;
        }
    }
    return board;
}

void copy_board(cell **source_board, cell **new_board){
    /*
     * Copying source sudoku board to new sudoku board.
     */
    int i,j;
    int N = blockRows * blockCols;


    for (i=0; i < N; i++) {
        for (j=0; j < N; j++){
            /*printf("I: %d, J: %d\n", i, j);*/

            new_board[i][j].number = source_board[i][j].number;
            new_board[i][j].asterisk = source_board[i][j].asterisk;
            new_board[i][j].isFixed = source_board[i][j].isFixed;

        }
    }
}

void mark_asterisks(cell **board) {
    int i, j;
    int N = blockCols * blockRows;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            if (board[i][j].number != 0) {
                valid_check(board, j + 1, i + 1, board[i][j].number);
            }
        }
    }
}

void execute_command(char *parsedCommand[4], cell **board, char* command, int counter, char mode, list *lst){
    /*
     * Evaluates game command (SET/HINT/VALIDATE/RESTART/EXIT) and calls the relavent function to execute it
     */
    /*TODO: missing generate X Y (add when ILP done)*/
    int setFlag = 0;
    int fillFlag = 0;
    cell ** boardAfter;
    if (strcmp(parsedCommand[0], "set") == 0 && counter == 4 && (mode == 'E' || mode == 'S')) {
        if ((!is_integer(parsedCommand[1])) || (!is_integer(parsedCommand[2])) || (!is_integer(parsedCommand[3]))){
            printf(VALUE_RANGE_ERROR, blockCols * blockRows);
            return;
        }
        setFlag = set(board, atoi(parsedCommand[1]),atoi(parsedCommand[2]), atoi(parsedCommand[3]), mode);
        if (setFlag) {
            boardAfter = generate_empty_board(blockRows, blockCols);
            copy_board(board, boardAfter);
            insert_at_tail(boardAfter, lst);
        }
    } else if (strcmp(parsedCommand[0], "hint") == 0 &&  counter == 3 && mode == 'S') {
        if ((!is_integer(parsedCommand[1])) || (!is_integer(parsedCommand[2]))){
            printf(VALUE_RANGE_ERROR, blockCols*blockRows);
            return;
        }
        hint(board, atoi(parsedCommand[1]), atoi(parsedCommand[2]));
    } else if (strcmp(parsedCommand[0], "validate") == 0  && (mode == 'E' || mode == 'S')) {
        /*validate(board);*/
    } else if ((strcmp(parsedCommand[0], "print_board") == 0) && (mode == 'E' || mode == 'S')) {
        print_board(board, mode);
    } else if (strcmp(parsedCommand[0], "mark_errors") == 0 && (mode == 'S')) {
        if (!is_integer(parsedCommand[1])) {
            printf(MARK_ERROR_ERROR);
            return;
        }
        mark_errors_command(atoi(parsedCommand[1]));
    } else if (strcmp(parsedCommand[0], "autofill") == 0 && mode == 'S') {
        fillFlag = auto_fill(board);
        if (fillFlag) {
            boardAfter = generate_empty_board();
            copy_board(board, boardAfter);
            insert_at_tail(boardAfter, lst);
        }
    } else if (strcmp(parsedCommand[0], "save") == 0 && (mode == 'E' || mode == 'S')) {
        save_command(board, parsedCommand[1], mode);
    } else if (strcmp(parsedCommand[0], "num_solutions") == 0 && (mode == 'E' || mode == 'S')) {
        num_solutions(board);
    } else if (((strcmp(parsedCommand[0], "undo") == 0)) && (mode == 'E' || mode == 'S')) {
        undo(lst, board, mode);
    } else if (((strcmp(parsedCommand[0], "redo") == 0)) && (mode == 'E' || mode == 'S')) {
        redo(lst, board, mode);
    } else if (((strcmp(parsedCommand[0], "reset") == 0)) && (mode == 'E' || mode == 'S')) {
        reset(lst, board, mode);
    } else if (strcmp(parsedCommand[0], "exit") == 0) {
        free_board(board);
        free_list(lst);
        exit_game(command);
    } else {
        printf(INVALID_ERROR);
    }
}

void print_separator(int N, int m) {
    /*
     * Helper function to printBoard() which prints block separators
     */
    char dash = '-';
    int i;
    int count = 4 * N + m + 1;
    for (i = 0; i < count; i++) {
        putchar(dash);
    }
    putchar('\n');
}

void print_board(cell **board, char mode) {
    /*
     * Prints the sudoku board according to the format
     */
    int i,j;
    int N = blockRows * blockCols;
    for (i = 0; i < N; i++) {
        if (i % blockRows == 0){
            print_separator(N, blockRows);
        }
        for (j=0; j < N; j++) {
            if (j % blockCols == 0) {
                printf("|");
            }
            printf(" ");
            if (board[i][j].isFixed) {
                printf("%2d", board[i][j].number);
                printf(".");
            }
            else if ((board[i][j].asterisk) && (markErrors || mode == 'E')) {
                printf("%2d", board[i][j].number);
                printf("*");
            }
            else if (board[i][j].number != UNASSIGNED){
                printf("%2d ", board[i][j].number);
            }
            else{ /*printing blank spaces for UNASSIGNED*/
                printf("   ");
            }
        }
        printf("|\n");
    }
    print_separator(N, blockRows);
}

void num_solutions(cell **board) {
    int solutionsCounter = count_solutions(board);
    /*int solutions_counter = count_solutions_rec(board, 0, 0, 0, *blockRows, *blockCols);*/
    if (check_board_erroneous(board)){
        printf("Error: board contains erroneous values\n");
    }
    printf("Number of solutions: %d\n", solutionsCounter);
    if (solutionsCounter == 1) {
        printf("This is a good board!\n");
    } else if (solutionsCounter > 1) {
        printf("The puzzle has more than 1 solution, try to edit it further\n");
    }
}

int count_solutions_rec(cell **board, int i, int j, int counter) {
    /*
     * Stack implementation
     */
    int N = blockRows * blockCols;
    int k;
    if (i == N) {
        i = 0;
        if (++j == N){
            return 1 + counter;
        }
    }
    if (board[i][j].number != 0){
        return count_solutions_rec(board, i+1, j, counter);
    }
    for (k = 1; k <= N; ++k) {
        if (valid_check(board, j, i, k)){
            board[i][j].number = k;
            counter = count_solutions_rec(board, i+1, j, counter);
        }
    }
    board[i][j].number = 0;
    return counter;
}

int count_solutions(cell **board) {
    /*
    Stack implementation

    */

    int N = blockRows * blockCols;

    int returnValue, value;

    SnapShotStruct newSnapshot;

    StackNode* snapshotStack;

    SnapShotStruct currentSnapshot;
    currentSnapshot.i = 0;
    currentSnapshot.j = 0;
    currentSnapshot.counter = 0;
    currentSnapshot.stage = 0;
    if (check_board_erroneous(board)) {
        printf("Error: board contains erroneous values\n");
        return 0;
    }
    push(&snapshotStack, currentSnapshot);

    while (!empty(snapshotStack)) {
        currentSnapshot=top(snapshotStack);
        pop(&snapshotStack);
        if (currentSnapshot.i == N){
            currentSnapshot.i = 0;
            if (++currentSnapshot.j == N) {
                returnValue = currentSnapshot.counter + 1;
                continue;
            }
        }
        if (board[currentSnapshot.i][currentSnapshot.j].number != 0) {
            newSnapshot.i = currentSnapshot.i + 1;
            newSnapshot.j = currentSnapshot.j;
            newSnapshot.counter = currentSnapshot.counter;
            newSnapshot.stage = 0;
            push(&snapshotStack, newSnapshot);
            continue;
        }
        for (value = 1; value <= N; ++value) {
            if (valid_check(board, currentSnapshot.j, currentSnapshot.i, value)){
                board[currentSnapshot.i][currentSnapshot.j].number = value;
                newSnapshot.i = currentSnapshot.i + 1;
                newSnapshot.j = currentSnapshot.j;
                newSnapshot.counter = currentSnapshot.counter;
                newSnapshot.stage = 0;
                push(&snapshotStack, newSnapshot);
                continue;
            }
        }
        board[currentSnapshot.i][currentSnapshot.j].number = 0;
        /*switch (currentSnapshot.stage)
        {
        case 0:
             if (currentSnapshot.i == N){
                 currentSnapshot.i = 0;
                 if (++currentSnapshot.j == N) {
                     returnValue = 1 + currentSnapshot.counter;
                     continue;
                 }
             }
             if (board[currentSnapshot.i][currentSnapshot.j].number != 0) {
                 newSnapshot.i = currentSnapshot.i + 1;
                 newSnapshot.j = currentSnapshot.j;
                 newSnapshot.counter = currentSnapshot.counter;
                 newSnapshot.stage = 0;
                 push(&snapshotStack, newSnapshot);
                 continue;
             }
             for (value = 1; value <= N; ++value) {
                 if (validCheck(board, currentSnapshot.j, currentSnapshot.i, value, numOfRows, numOfCols)){
                     board[currentSnapshot.i][currentSnapshot.j].number = value;
                     newSnapshot.i = currentSnapshot.i + 1;
                     newSnapshot.j = currentSnapshot.j;
                     newSnapshot.counter = currentSnapshot.counter;
                     newSnapshot.stage = 0;
                     push(&snapshotStack, newSnapshot);
                     continue;
                 }
             }
             currentSnapshot.stage = 1;
             push(&snapshotStack, currentSnapshot);

             break;
        case 1:
             newSnapshot.i = currentSnapshot.i + 1;
             newSnapshot.j = currentSnapshot.j;
             newSnapshot.counter = returnValue;
             newSnapshot.stage = 0;
             push(&snapshotStack, newSnapshot);
             continue;
             break;
        }*/
    }
    return returnValue;
}

void save_command(cell **board, char *filePath,char mode) {
    /*
    TODO: remove remarks... waiting for validate
    */

    FILE *fp;
    int i, j, N;

    if (mode == 'E') {
        if (check_board_erroneous(board)) {
            printf(ERRONEOUS_ERROR);
            return;
        }
      /*
        else if (validate(board) == false) {
            printf(VALIDATION_ERROR);
            return;
      }
        */
    }

    if ((fp = fopen(filePath, "wb")) == NULL) {
        printf("Error: File cannot be created or modified\n");
    }

    fprintf(fp, "%d %d\n", blockRows, blockCols);
    N = blockRows * blockCols;
    for (i = 0; i < N; i++) {
        for (j = 0; j < N; j++) {
            fprintf(fp, "%d", board[i][j].number);
            if ((board[i][j].isFixed || mode == 'E') && (board[i][j].number != UNASSIGNED)) {
                fprintf(fp, ".");
            }
            fprintf(fp, " ");
        }
        fprintf(fp, "\n");
    }
    fclose(fp);
    printf("Saved to: %s\n", filePath);
}

void mark_errors_command(int value) {
    if (value == 0) {
        markErrors = 0;
    } else if (value == 1) {
        markErrors = 1;

    }
}

cell **load_board(FILE* fp, char mode){/*add char mode- in edit need to clear all fixed*/
    int i, j, N;
    cell **board = NULL;
    char line[257];
    char *token;
    char *delimiter = " \t\r\n";
    if (fgets(line, 257, fp) != NULL){

        token = strtok(line, delimiter);
        blockRows = atoi(token);
        token = strtok(NULL, delimiter);
        blockCols = atoi(token);
    }
    N = blockRows * blockCols;
    board = calloc(N, sizeof (*board));
    for (i = 0; i < N; i++) {
        board[i] = calloc(N, (sizeof **board));
    }
    for (i = 0; i < N; i++) {
        fgets(line, 256, fp);
        token = strtok(line, delimiter);
        for (j = 0; j < N; j++) {
            board[i][j].number = token[0] - '0';
            if ((token[1] == '.') && (mode == 'S')) {
                    board[i][j].isFixed = true;
            }
            else if (token[1] == '*') {
                board[i][j].asterisk = true;
                }
            token = strtok(NULL, delimiter);
        }
    }
    mark_asterisks(board);
    return board;
}



cell ** edit_command(char* parsedCommand[4], char mode){
    cell **board = NULL;
    FILE* fp = NULL;
    if (parsedCommand[1] != '\0'){
        fp = fopen(parsedCommand[1], "r");
        if (fp != NULL) {
            board = load_board(fp, mode);
            fclose(fp);
        }
        else {
            printf("Error: File cannot be opened\n");
            return board;
        }
    }
    else {
        blockRows = 3;
        blockCols = 3;
        board = generate_empty_board();
    }
    markErrors = 1;
    print_board(board, mode);
    return board;
}

cell **solve_command(char* parsedCommand[4],char mode){
    cell **board = NULL;
    FILE* fp = NULL;
    if (parsedCommand[1] == '\0'){
        printf(INVALID_ERROR);
        return board;
    }
    fp = fopen(parsedCommand[1], "r");
    if (fp != NULL) {
        board = load_board(fp, mode);
        fclose(fp);
        print_board(board, mode);
    } else {
        printf("Error: File doesn't exist or cannot be opened\n");
    }
    return board;
}


bool val_in_block(cell **board, int column, int row, int val){
    /*
     * Checks if value exist in the block containing given row and column
     */

    bool valExist = false;
    int initialCol, initialRow, colIndex, rowIndex;
    initialCol = get_block_col_index(column);
    initialRow = get_block_row_index(row);
    for (colIndex = initialCol; (colIndex < blockCols + initialCol); colIndex++) {
        for (rowIndex = initialRow; (rowIndex < blockRows + initialRow); rowIndex++) {
            if((colIndex == column - 1) && (rowIndex == row - 1)) { /*not checking cell to be changed*/}
            else if (board[rowIndex][colIndex].number == val) {
                board[rowIndex][colIndex].asterisk= true;
                valExist = true;
            }
        }
    }
    return valExist;
}

bool val_in_row(cell **board, int column, int row, int val){
    /*
     * Checks if value exist in the given row
     */

    int N = blockRows * blockCols;
    bool valExist = false;
    int colIndex;
    for (colIndex = 0; colIndex < N; colIndex++) {
        if(colIndex == column - 1) {/* not checking cell to be changed*/}
        else if (board[row - 1][colIndex].number == val) {
            valExist = true;
            if (board[row - 1][colIndex].isFixed == false) {
                board[row - 1][colIndex].asterisk = true;
            }
        }
    }
    return valExist;
}

bool val_in_column(cell **board, int column, int row, int val) {
    /*
     * Checks if value exist in the given column
     */
    int N = blockRows * blockCols;
    bool valExist = false;
    int rowIndex;
    for (rowIndex = 0; rowIndex < N; rowIndex++) {
        if(rowIndex == row - 1){/*not checking cell to be changed*/}
        else if (board[rowIndex][column - 1].number == val) {
            valExist = true;
            if (board[rowIndex][column - 1].isFixed == false) {
                board[rowIndex][column - 1].asterisk = true;
            }
        }
    }
    return valExist;
}

bool valid_check(cell **board, int column, int row, int val) {
    /*
     * Checks if validation of given value in cell <row,column> according to sudoku rules
     * marks aserisks
     */

    if(val_in_block(board, column, row, val) | val_in_row(board, column, row, val) | val_in_column(board, column, row, val)){
        board[row - 1][column - 1].asterisk = true;
        return false;
    }
    else{
        board[row - 1][column - 1].asterisk = false;
        return true;
    }

}

void game_over(cell **board){
    /*
     * Checks if board is full and solved correctly
     */
    /*TODO: validaeion with ILP*/

    int col, row;
    /*bool full = true;*/
    if (check_board_erroneous(board)){
        return;
    }
        for (col = 0; col < NUM_OF_COLUMNS; col++) {
        for (row = 0; row < NUM_OF_ROWS; row++) {
            if (board[row][col].number == UNASSIGNED) {
                /*full = false;*/
            }
        }
    }
    /*
    if (full){
        if validate(board){
            printf(GAME_OVER);
            mode = 'I';
        }
    else{
     printf("Puzzle solution erroneous\n");

    }
     */
}

int auto_fill(cell **board)	{
    /*
     * Autofills cells which contain a single legal value

     */
    int i, j, k, candidate;
    int fillFlag = false;
    int numOfCandidates = 0;
    int maxValue = blockRows * blockCols + 1;
    cell **copyOfBoard;
    int N = blockRows * blockCols;
    if (check_board_erroneous(board)){
        printf("Error: board contains erroneous values\n");
        return false;
    }
    copyOfBoard = generate_empty_board();
    copy_board(board, copyOfBoard);
    for (i = 0; i < N; i++){
        for (j = 0; j < N; j++){
            if (copyOfBoard[i][j].number == UNASSIGNED){
                for (k = 1; k < maxValue; k++){
                    if (valid_check(copyOfBoard, j + 1, i + 1, k)){
                        candidate = k;
                        numOfCandidates++;
                    }
                }
                if (numOfCandidates == 1){
                    board[i][j].number = candidate;
                    validate_risks(board, j + 1, i + 1);
                    fillFlag = true;
                    printf("Cell <%d,%d> set to %d\n", (j+1), (i+1), candidate);
                }
                numOfCandidates = 0;
            }

        }
    }
    print_board(board, 'E');
    game_over(board);
    return fillFlag;
}

void validate_risks(cell **board, int column, int row) {
    int initialCol, initialRow, colIndex, rowIndex;
    int N = blockRows * blockCols;
    initialCol = get_block_col_index(column);
    initialRow = get_block_row_index(row);
    for (colIndex = initialCol; (colIndex < blockCols + initialCol); colIndex++) { /*block*/
        for (rowIndex = initialRow; (rowIndex < blockRows + initialRow); rowIndex++) {
            if (colIndex == (column-1) && rowIndex == (row-1)) {
                /*not checking cell to be changed*/
            }
            else if (board[rowIndex][colIndex].asterisk){
                if(valid_check(board, colIndex + 1, rowIndex + 1, board[rowIndex][colIndex].number)){
                    board[rowIndex][colIndex].asterisk = false;
                }
            }
        }
    }
    for (colIndex = 0; (colIndex < N); colIndex++) { /*row*/
        if (colIndex == column - 1) {
        }
        else if (board[row - 1][colIndex].asterisk){
            if (valid_check(board, colIndex + 1, row, board[row - 1][colIndex].number)){
                board[row - 1][colIndex].asterisk = false;
            }
        }
    }

    for (rowIndex = 0; (rowIndex < N); rowIndex++) {  /*col*/
        if (rowIndex == row - 1){
        }
        else if (board[rowIndex][column - 1].asterisk){
            if( valid_check(board, column , rowIndex + 1, board[rowIndex][column - 1].number)){
                board[rowIndex][column - 1].asterisk = false;
            }
        }
    }
}


bool set(cell **board, int column, int row, int val, char mode) {
    int N = blockRows * blockCols;
    if((!valid_board_index(column, N)) || (!valid_board_index(row, N)) || (!valid_set_value(val, N))){
        printf(VALUE_RANGE_ERROR, blockCols * blockRows);
        return false;
    }
     if (board[row - 1][column - 1].isFixed) {
        printf(FIXED_ERROR);
        return false;
     }
     else if (board[row - 1][column - 1].number == val) {
         print_board(board, mode);
         return  false;
    }
    else if (val == 0) {
        board[row - 1][column - 1].number = UNASSIGNED;
        board[row - 1][column - 1].asterisk = false;
        validate_risks(board, column, row);
        print_board(board, mode);
        return true;
    }
    else {
        valid_check(board, column, row, val); /*make valid check to updated astrisk*/
        board[row - 1][column - 1].number = val;
        validate_risks(board, column, row);
        print_board(board, mode);
        if (mode == 'S'){
            game_over(board);
        }

        return true;
    }
}

bool check_board_erroneous(cell **board){
    int colIndex, rowIndex = 0;
    int N = blockRows * blockCols;
    for (rowIndex = 0; rowIndex < N; rowIndex++) {
        for (colIndex = 0; colIndex < N; colIndex++) {
            if (board[rowIndex][colIndex].asterisk) {
                return true;
            }
        }
    }
    return false;
}

void hint(cell **board, int column, int row){
    /*
     * Prints the value of the cell <row,column> on the last solved sudoku board
     */

    /* TODO: after ILP solver working unmark code */

    /* int hint;
    cell **solvedBoard = NULL; */
    if (check_board_erroneous(board)) {
        printf("Error: board contains erroneous values\n");
        return;
    }
    if (board[row - 1][column - 1].isFixed) {
        printf(FIXED_ERROR);
        return;;
    }
    if (board[row - 1][column - 1].number == 0) {
        printf("Error: cell already contains a value\n");
        return;
    }
    /*solvedBoard = validate(board)
      if (solvedBoard == NULL){
      printf("Error: board is unsolvable\n");
      return;
      }
    else{
         hint = solvedBoard_[row-1][column-1].number;
        printf("Hint: set cell to %d\n", hint);
        }
        */
}

void free_board(cell** board){
    int i;
    int N = blockRows * blockCols;
    for (i = 0; i < N; i++) {
        free(board[i]);
    }
    free(board);
    board = NULL;

}

void exit_game(char* command){
    /*
     * Free up memeory and exists the program
     */
    printf("Exiting...\n");
    free(command);

    exit(0);
}