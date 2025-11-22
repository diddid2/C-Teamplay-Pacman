#pragma warning(disable: 4996)
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int updateBoard(int row, int column, char act); //입력받은 내용으로 보드의 상태를 변경
int openNumberCounter(void); //열린 칸의 수를 세는 코드 - 클리어 판정에 사용
int printBackBoard(void); //백보드를 출력
int printBoard(void); //보드를 출력
int reset_isOpen(void); //모든 칸을 닫힌 칸으로 초기화
int reset_backBoard(int startRow, int startColumn, int mineNumber); //백보드를 세팅

int isOpen[10][10]; //3-불리언 어레이
char backBoard[10][10];
int stage;
int stageMineNumber[10] = { 10, 11, 12, 14, 15, 17, 20, 22, 25, 30 }; //각 스테이지에 배치된 지뢰의 개수

int main(void) {
    srand(time(NULL));

    for (stage = 0; stage < 10; stage += 1) {
        reset_isOpen();

        int isClear = 0; //3-불리언
        int inputRow, inputColumn;
        char act;

        printf("stage %d\n", stage + 1);

        printf("처음 열 좌표 입력 >> 행_좌표 열_좌표\n>> ");
        scanf("%d %d", &inputRow, &inputColumn);
        int row = inputRow - 1, column = inputColumn - 1; //입력받은 [1~10]을 [0~9]로 변환
        reset_backBoard(row, column, stageMineNumber[stage]);
        updateBoard(row, column, 'o');
        printBackBoard(); //이 줄의 코드는 디버그를 위해 존재하며 최종 제출 시 삭제된 상태로 제출
        printBoard();

        while (!isClear) {
            printf("좌표와 역할 입력 >> 행_좌표 열_좌표 역할\n>> ");
            scanf("%d %d %c", &inputRow, &inputColumn, &act);
            row = inputRow - 1, column = inputColumn - 1; //입력받은 [1~10]을 [0~9]로 변환
            isClear = updateBoard(row, column, act);
            printBoard();
        }; //클리어(2)나 게임 오버(1)로 루프 탈출 - isClear != 0
        if (isClear == 1) {
            printf("게임 오버로 인해 현재 스테이지를 다시 시작합니다\n");
            printBackBoard();
            stage -= 1; //현재 스테이지에 묶어두기 위해 스테이지 증가를 상쇄
        };
        if (isClear == 2) {
            printf("이번 스테이지 클리어를 축하드립니다\n");
            printBackBoard();
        };
    };
    printf("모든 스테이지 클리어를 축하드립니다\n");

    return 0;
};

int updateBoard(int row, int column, char act) {
    switch (act) {
    case 'o':
    case 'O':
        switch (isOpen[row][column]) {
        case 0:
            isOpen[row][column] = 1; //칸 열기
            if (backBoard[row][column] == '@') { return 1; }; //지뢰가 있는 칸을 열었을 경우
            if (openNumberCounter() == 100 - stageMineNumber[stage]) { return 2; }; //지뢰가 없는 칸을 전부 열었을 경우
            break;
        case 1:
            printf("열린 칸을 다시 열 수 없습니다\n");
            break;
        case 2:
            printf("깃발을 꽃은 칸은 열 수 없습니다\n");
            break;
        };
        break;
    case 'f':
    case 'F':
        switch (isOpen[row][column]) {
        case 0:
            isOpen[row][column] = 2; //깃발 꽃기
            break;
        case 1:
            printf("열린 칸에 깃발을 꽃을 수 없습니다\n");
            break;
        case 2:
            isOpen[row][column] = 0; //깃발 회수
            break;
        };
        break;
    default:
    }; //0 - 열리지 않은 칸 && 1 - 열린 칸 && 2 - 깃발을 꽃은 칸

    return 0;
}; //입력받은 내용으로 보드의 상태를 변경

int openNumberCounter(void) {
    int openNumber = 0;
    for (int boardLoop = 0; boardLoop < 100; boardLoop += 1) {
        if (isOpen[boardLoop / 10][boardLoop % 10] == 1) {
            openNumber += 1;
        };
    };
    return openNumber;
}; //열린 칸의 수를 세는 코드 - 클리어 판정에 사용

int printBackBoard(void) {
    printf("\n=== 백보드 (디버그용) ===\n");
    printf("   ");
    for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
        printf(" %2d ", columnLoop + 1);
    };
    printf("\n");
    printf("   ");
    for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
        printf("----");
    };
    printf("-\n");

    for (int rowLoop = 0; rowLoop < 10; rowLoop += 1) {
        printf("%2d |", rowLoop + 1);
        for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
            printf(" %c |", backBoard[rowLoop][columnLoop]);
        };
        printf("\n");
        printf("   ");
        for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
            printf("----");
        };
        printf("-\n");
    };
    printf("\n");

    return 0;
}; //백보드를 출력

int printBoard(void) {
    printf("\n=== 게임 보드 ===\n");
    printf("   ");
    for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
        printf(" %2d ", columnLoop + 1);
    };
    printf("\n");
    printf("   ");
    for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
        printf("----");
    };
    printf("-\n");

    for (int rowLoop = 0; rowLoop < 10; rowLoop += 1) {
        printf("%2d |", rowLoop + 1);
        for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
            if (isOpen[rowLoop][columnLoop] == 1) {
                if (backBoard[rowLoop][columnLoop] == ' ') {
                    printf("   |");
                }
                else {
                    printf(" %c |", backBoard[rowLoop][columnLoop]);
                }
            }
            else if (isOpen[rowLoop][columnLoop] == 0) {
                printf(" ? |");
            }
            else {
                printf(" F |");
            };
        };
        printf("\n");
        printf("   ");
        for (int columnLoop = 0; columnLoop < 10; columnLoop += 1) {
            printf("----");
        };
        printf("-\n");
    };
    printf("\n");

    return 0;
}; //보드를 출력

int reset_isOpen(void) {
    for (int resetLoop = 0; resetLoop < 100; resetLoop += 1) {
        isOpen[resetLoop / 10][resetLoop % 10] = 0;
    };

    return 0;
}; //모든 칸을 닫힌 칸으로 초기화

int reset_backBoard(int startRow, int startColumn, int mineNumber) {
    int cleanSpace = 9; //비워야 할 공간의 크기를 정의
    if (startRow == 1 || startRow == 10) {
        cleanSpace = cleanSpace / 3 * 2;
    }; //값 변경 9 => 6
    if (startColumn == 1 || startColumn == 10) {
        cleanSpace = cleanSpace / 3 * 2;
    }; //값 변경 9 => 6 || 6 => 4

    int cleanSpaceAt[cleanSpace]; //지뢰가 들어갈 수 없는 칸 목록
    int counter_cleanSpaceAt = 0;
    for (int Loop1 = startRow - 1; Loop1 <= startRow + 1; Loop1 += 1) {
        for (int Loop2 = startColumn - 1; Loop2 <= startColumn + 1; Loop2 += 1) {
            if ((0 <= Loop1 && Loop1 <= 9) || (0 <= Loop2 && Loop2 <= 9)) {
                cleanSpaceAt[counter_cleanSpaceAt] = Loop1 * 10 + Loop2;
                counter_cleanSpaceAt += 1;
            }; //비울 구간이 보드의 범위 내에 들어오면 목록에 추가
        };
    };

    int mineSpace = 100 - cleanSpace; //지뢰가 존재 가능한 칸의 수
    int allMineAt[mineNumber]; //지뢰가 있는 칸 목록
    for (int resetLoop = 0; resetLoop < mineNumber; resetLoop += 1) {
        allMineAt[resetLoop] = -1; //존재할 수 없는 좌표를 통해 환각 지뢰를 방지
    }; //목록의 값을 초기화

    for (int mineLoop = 0; mineLoop < mineNumber; mineLoop += 1) {
        int batch = 0; //불리언
        while (!batch) {
            int newMineAt = rand() % 100; //새 지뢰의 좌표를 단좌표로 생성

            batch = 1; //이 값이 루프의 끝까지 유지될 경우 루프 종료

            for (int checkLoop = 0; checkLoop < cleanSpace; checkLoop += 1) {
                if (newMineAt == cleanSpaceAt[checkLoop]) {
                    batch = 0;
                };
            }; //지뢰가 들어갈 수 없는 칸이 배정된 경우 요청 반려

            for (int checkLoop = 0; checkLoop < mineNumber; checkLoop += 1) {
                if (newMineAt == allMineAt[checkLoop]) {
                    batch = 0;
                };
            }; //지뢰가 이미 들어간 칸이 배정된 경우 요청 반려

            if (batch) {
                allMineAt[mineLoop] = newMineAt; //검사를 통과한 경우 새 지뢰를 추가
            };
        };
    };

    for (int boardLoop = 0; boardLoop < 100; boardLoop += 1) {
        backBoard[boardLoop / 10][boardLoop % 10] = ' ';
    }; //실제 보드의 값을 초기화

    for (int addLoop = 0; addLoop < mineNumber; addLoop += 1) {
        int shortN = allMineAt[addLoop]; //코드 가독성을 위해 축약
        backBoard[shortN / 10][shortN % 10] = '@'; //지뢰를 심는 코드
    };

    for (int boardLoop = 0; boardLoop < 100; boardLoop += 1) {
        if (backBoard[boardLoop / 10][boardLoop % 10] == '@') { continue; }; //지뢰가 있는 칸은 숫자를 배치하지 않고 continue
        int aroundNumber = 0; //최종적으로 완성된 이 숫자가 해당 칸의 숫자
        for (int aroundRow = boardLoop / 10 - 1; aroundRow <= boardLoop / 10 + 1; aroundRow += 1) {
            if (aroundRow < 0 || aroundRow > 9) { continue; }; //존재하지 않는 행은 지뢰를 세지 않고 continue
            for (int aroundColumn = boardLoop % 10 - 1; aroundColumn <= boardLoop % 10 + 1; aroundColumn += 1) {
                if (aroundColumn < 0 || aroundColumn > 9) { continue; }; //존재하지 않는 열은 지뢰를 세지 않고 continue
                if (backBoard[aroundRow][aroundColumn] == '@') {
                    aroundNumber += 1; //지뢰가 있는 칸이 주변에 있을 때 실행
                };
            };
        };
        if (aroundNumber != 0) {
            backBoard[boardLoop / 10][boardLoop % 10] = aroundNumber + 48; //48은 아스키의 숫자로 변환하기 위해 더해야 할 숫자
        }; //숫자가 0이라면 빈 칸으로 표시
    }; //유니코드로 된 숫자를 심는 코드

    return 0;
}; //백보드를 세팅