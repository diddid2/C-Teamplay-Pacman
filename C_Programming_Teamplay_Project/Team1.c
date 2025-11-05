//#include <stdio.h>
//#include <stdlib.h>
//#include <string.h>
//#include <windows.h>
//
//#define map_height 24
//#define map_width 97
//
//#define SPACE 0
//#define WALL  1
//#define _POINT 2
//#define ITEM  3
//#define PLAYER 4
//#define ENEMY 5
//#define DIED_ENEMY 6
//
//void location(int x, int y) {
//    COORD pos;
//    pos.X = x;
//    pos.Y = y;
//    SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
//}
//
//void setColor(int color) {
//    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), color);
//}
//
//typedef struct {
//    int x;
//    int y;
//} POS;
//
//typedef struct {
//    int map[map_height][map_width];
//    int front_buffer[map_height][map_width];
//    int back_buffer[map_height][map_width];
//    POS point[500];
//    POS item[500];
//    int point_count;
//    int item_count;
//} Map;
//
//void Map_Init(Map* m);
//void Map_Update(Map* m);
//void Map_Render(Map* m);
//int  Map_GetItem(Map* m, int x, int y);
//
//void Map_Init(Map* m) {
//    char temp[map_height][map_width + 1] = {
//" # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # #",
//" # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . # ",
//" # . # # # # . # # # # # # # . # # . # # # # # . # # # # # . # # # # # . # # . # # # # # # # . # ",
//" # . # # # # . # # # # # # # . # # . # # # # # . # # # # # . # # # # # . # # . # # # # # # # . # ",
//" # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . # ",
//" # . # # # # . # # . # # # # # # # # # # # . # # . # # # # . . . . . . # # # # # # . # # # # . # ",
//" # . # # # # . # # . # # # # # # # # # # # . # # . # # # # . . . . . . # # # # # # . # # # # . # ",
//" # . . . . . . # # . . . # # . . . . . . . . # # . . . . . . . . . . . . . # # . . . . . . . . # ",
//" # # # # # # . # # . # # . # # . # # # # # . # # # # # # # # # # # # # # . # # . # # # # # # # # ",
//" # # # # # # . # # . # # . # # . # # # # # . # # # # # # # # # # # # # # . # # . # # # # # # # # ",
//" # . . . . . . . . . . . . . . . . . . . . . . .                     . . . . . . . . . . . . . # ",
//" # . # # # # . # # . # # # # # # # # # # # . # #                     # # . # # # # # # # # # . # ",
//" # . # # # # . # # . # # # # # # # # # # # . # #                     # # . # # # # # # # # # . # ",
//" # . # # # # . # # . . . . . . . . . . . . . . .                     . . . . . . . . . . # # . # ",
//" # . # # # # . # # . # # # # # # # # # # . # # . # # # # # # # # # # . # # . # # # # # . # # . # ",
//" # . # # # # . # # . # # # # # # # # # # . # # . # # # # # # # # # # . # # . # # # # # . # # . # ",
//" # . . . . . . . . . . # # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . # ",
//" # # # # # # . # # # # # . # # . # # # # # . # # . # # # # # . # # # . # # . # # # # # . # # # # ",
//" # # # # # # . # # # # # . # # . # # # # # . # # . # # # # # . # # # . # # . # # # # # . # # # # ",
//" # . . . . . . # # # # # . . . . . . . . . . . . . # # # # # . # # # . . . . # # # # . . . . . # ",
//" # . # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # . # ",
//" # . # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # # # . # # # . # ",
//" # . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . . # ",
//" # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # # "
//    };
//
//    m->point_count = 0;
//    m->item_count = 0;
//
//    for (int i = 0; i < map_height; i++) {
//        for (int j = 0; j < map_width; j++) {
//            switch (temp[i][j]) {
//            case ' ': m->map[i][j] = SPACE; break;
//            case '#': m->map[i][j] = WALL; break;
//            case '.':
//                m->map[i][j] = SPACE;
//                m->point[m->point_count].x = j;
//                m->point[m->point_count].y = i;
//                m->point_count++;
//                break;
//            case '*':
//                m->map[i][j] = SPACE;
//                m->item[m->item_count].x = j;
//                m->item[m->item_count].y = i;
//                m->item_count++;
//                break;
//            default:
//                m->map[i][j] = SPACE;
//            }
//            m->back_buffer[i][j] = m->map[i][j];
//            m->front_buffer[i][j] = m->map[i][j];
//        }
//    }
//}
//
//void Map_Update(Map* m) {
//    for (int i = 0; i < m->point_count; i++)
//        m->back_buffer[m->point[i].y][m->point[i].x] = _POINT;
//    for (int i = 0; i < m->item_count; i++)
//        m->back_buffer[m->item[i].y][m->item[i].x] = ITEM;
//}
//
//void Map_Render(Map* m) {
//    location(0, 0);
//    for (int i = 0; i < map_height; i++) {
//        for (int j = 0; j < map_width; j++) {
//            int tile = m->back_buffer[i][j];
//            switch (tile) {
//            case WALL:  setColor(9);  printf("■"); break;
//            case _POINT:setColor(14); printf("."); break;
//            case ITEM:  setColor(13); printf("☆"); break;
//            case PLAYER:setColor(14); printf("●"); break;
//            case ENEMY: setColor(12); printf("▲"); break;
//            default:    setColor(7);  printf(" "); break;
//            }
//        }
//        printf("\n");
//    }
//    setColor(7);
//}
//
//int Map_GetItem(Map* m, int x, int y) {
//    for (int i = 0; i < m->point_count; i++) {
//        if (m->point[i].x == x && m->point[i].y == y) {
//            for (int j = i; j < m->point_count - 1; j++)
//                m->point[j] = m->point[j + 1];
//            m->point_count--;
//            return _POINT;
//        }
//    }
//    for (int i = 0; i < m->item_count; i++) {
//        if (m->item[i].x == x && m->item[i].y == y) {
//            for (int j = i; j < m->item_count - 1; j++)
//                m->item[j] = m->item[j + 1];
//            m->item_count--;
//            return ITEM;
//        }
//    }
//    return SPACE;
//}
//
//int main() {
//    Map gameMap;
//    system("cls");
//    Map_Init(&gameMap);
//    Map_Update(&gameMap);
//    Map_Render(&gameMap);
//
//    location(0, map_height + 1);
//    printf("\n맵 로드 완료! (아무 키나 누르면 종료)\n");
//    system("pause");
//
//    return 0;
//}