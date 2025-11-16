#define _CRT_SECURE_NO_WARNINGS
#define _WIN32_WINNT 0x0600
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <wchar.h>
#include <string.h>
#include <mmsystem.h>
#include <io.h>
#pragma comment(lib, "winmm.lib")

#define MAP_ROWS 23
#define MAP_COLS 47
#define SCREEN_ROWS (MAP_ROWS + 1)
#define SCREEN_COLS MAP_COLS

#define CH_WALL    L'■'
#define CH_COIN    L'·'
#define CH_EMPTY   L'　'
#define CH_GHOST   L'○'
#define CH_PLAYER  L'●'
#define CH_POWER   L'＠'   // 파워펠릿 문자 여긴 전각인데 인게임에서는 맵 밀림때문에 반각으로 대체함

typedef struct Player { int r, c, score, lives; } Player;
typedef struct Ghost {
    int r, c, sr, sc, dir, gtype;
    bool alive, vulnerable;
} Ghost;

// 유령 AI 추가
typedef enum { SCATTER, CHASE } GhostMode;
GhostMode currentGhostMode = SCATTER;
DWORD modeSwitchTime = 0;
int modePhase = 0;

DWORD phaseDurations[7] = { 7000, 20000, 7000, 20000, 5000, 20000, 5000 };

bool coin[MAP_ROWS][MAP_COLS];
bool powerPellet[MAP_ROWS][MAP_COLS];
Player player;
Ghost ghosts[4];
bool globalVulnerable = false;
DWORD powerEndTime = 0;

CHAR_INFO* frameBuffer = NULL;
CHAR_INFO* prevFrameBuffer = NULL;
CHAR_INFO* tmpRowBuffer = NULL;

double playerMoveAcc = 0.0;
double playerSpeed = 8.0;
DWORD ghostMoveInterval = 300;

int currentDirR = 0, currentDirC = 0;
int desiredDirR = 0, desiredDirC = 0;
const wchar_t* SND_START = L"/sfx/start.wav";
const wchar_t* SND_COIN = L"/sfx/coin.wav";
const wchar_t* SND_POWER = L"/sfx/power.wav";
const wchar_t* SND_EATGHOST = L"/sfx/eat_ghost.wav";
const wchar_t* SND_DIE = L"/sfx/die.wav";
const wchar_t* SND_GAMEOVER = L"/sfx/gameover.wav";
const wchar_t* SND_CLEAR = L"/sfx/clear.wav";

void playSoundIfExists(const wchar_t* path, DWORD flags) {
    if (!path || !*path) return;
    if (_waccess(path, 0) != 0) return;
    PlaySoundW(path, NULL, SND_FILENAME | flags);
}

wchar_t* wcmap[MAP_ROWS] = {
    L"■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■",
    L"■○·····················■·····················○■",
    L"■·■■■■·■■■■■■■■·■■■■■■·■·■■■■■■·■■■■■■■■·■■■■·■",
    L"■·■■■■·■■■■■■■■·■■■■■■·■·■■■■■■·■■■■■■■■·■■■■·■",
    L"■·■·········································■·■",
    L"■·■·■■■■■■■■■·■·■·■■■■■■■■■■■·■·■·■■■■■■■■■·■·■",
    L"■·■···＠■■·····■·■······■······■·■·····■■＠···■·■",
    L"■·■·■■·■■·■■■·■·■■■■■■·■·■■■■■■·■·■■■·■■·■■·■·■",
    L"■·■·■■·■■·■■■·■·■　　　　　　　　　　　　　■　■·■■■·■■·■■·■·■",
    L"■·■·■■·■■·····■·■　■■■■　　　■■■■　■·■·····■■·■■·■·■",
    L"■·■·■■····■■■■■·■　■　　　　　　　　　■　■·■■■■■····■■·■·■",
    L"■···■■■■■········　■　　　　●　　　　■　········■■■■■···■",
    L"■·■·■■····■■■■■·■　■　　　　　　　　　■　■·■■■■■····■■·■·■",
    L"■·■·■■·■■·····■·■　■■■■■■■■■■■　■·■·····■■·■■·■·■",
    L"■·■·■■·■■·■■■·■·■　　　　　　　　　　　　　■·■·■■■·■■·■■·■·■",
    L"■·■·■■·■■·■■■·■·■·■■■■■■■■■■■·■·■·■■■·■■·■■·■·■",
    L"■·■···＠■■··············■··············■■＠···■·■",
    L"■·■·■■■■■■■■■·■·■■■■■■·■·■■■■■■·■·■■■■■■■■■·■·■",
    L"■·■·■■■■■■■■■·■·■·············■·■·■■■■■■■■■·■·■",
    L"■·■·············■·■■■■·■·■■■■·■·············■·■",
    L"■·■·■■■■■■■■■■■·■·■■■■·■·■■■■·■·■■■■■■■■■■■·■·■",
    L"■○·····················■·····················○■",
    L"■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■■"
};

void compose_frameBuffer_from_game_state();
void render_partial_updates(HANDLE hOut, bool forceFullWrite);
void check_collect_and_collision();

void setConsoleSizeAndFont() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    if (hOut == INVALID_HANDLE_VALUE) return;
    COORD bufferSize = { (SHORT)SCREEN_COLS, (SHORT)SCREEN_ROWS };
    SetConsoleScreenBufferSize(hOut, bufferSize);
    SMALL_RECT winRect = { 0, 0, (SHORT)(SCREEN_COLS - 1), (SHORT)(SCREEN_ROWS - 1) };
    SetConsoleWindowInfo(hOut, TRUE, &winRect);
    CONSOLE_FONT_INFOEX cfi;
    cfi.cbSize = sizeof(CONSOLE_FONT_INFOEX);
    GetCurrentConsoleFontEx(hOut, FALSE, &cfi);
    wcscpy_s(cfi.FaceName, LF_FACESIZE, L"Lucida Console");
    cfi.dwFontSize.X = 14;
    cfi.dwFontSize.Y = 28;
    cfi.FontWeight = FW_NORMAL;

    SetCurrentConsoleFontEx(hOut, FALSE, &cfi);
}

void hideCursor() {
    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_CURSOR_INFO ci = { 1, FALSE };
    SetConsoleCursorInfo(hOut, &ci);
}

bool is_wall(int r, int c) {
    if (r < 0 || r >= MAP_ROWS || c < 0 || c >= MAP_COLS) return true;
    wchar_t ch = wcmap[r][c];
    return (ch == CH_WALL);
}
int manhattan(int a, int b, int c, int d) { return abs(a - c) + abs(b - d); }

// wcmap을 스캔하여 코인/플레이어/유령/파워펠릿 배치
void init_world() {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            coin[r][c] = false;
            powerPellet[r][c] = false;
        }
    }

    player.score = 0; player.lives = 3;
    player.r = 12; player.c = 49;
    for (int i = 0; i < 4; i++) {
        ghosts[i].r = ghosts[i].sr = 0;
        ghosts[i].c = ghosts[i].sc = 0;
        ghosts[i].dir = i % 4;
        ghosts[i].gtype = i;
        ghosts[i].alive = true;
        ghosts[i].vulnerable = false;
    }
    int ghostCount = 0;

    for (int r = 0; r < MAP_ROWS; r++) {
        if (wcmap[r] == NULL) continue;
        for (int c = 0; c < MAP_COLS; c++) {
            wchar_t ch = wcmap[r][c];

            if (ch == CH_COIN) {
                coin[r][c] = true;
            }
            else if (ch == CH_POWER) {
                powerPellet[r][c] = true;
            }

            if (ch == CH_PLAYER) {
                player.r = r; player.c = c;
            }
            else if (ch == CH_GHOST && ghostCount < 4) {
                ghosts[ghostCount].r = ghosts[ghostCount].sr = r;
                ghosts[ghostCount].c = ghosts[ghostCount].sc = c;
                ghostCount++;
            }
        }
    }

    globalVulnerable = false;
    powerEndTime = 0;
    currentDirC = 0;
    currentDirR = 0;
    desiredDirC = 0;
    desiredDirR = 0;

    for (int i = ghostCount; i < 4; i++) {
        bool placed = false;
        for (int r = 1; r < MAP_ROWS - 1 && !placed; r++) {
            for (int c = 1; c < MAP_COLS - 1 && !placed; c++) {
                if (!is_wall(r, c)) {
                    ghosts[i].r = ghosts[i].sr = r;
                    ghosts[i].c = ghosts[i].sc = c;
                    placed = true;
                }
            }
        }
    }
}

// 화면 그리기
void compose_frameBuffer_from_game_state() {
    for (int r = 0; r < MAP_ROWS; r++) {
        for (int c = 0; c < MAP_COLS; c++) {
            int idx = r * SCREEN_COLS + c;
            wchar_t ch = wcmap[r] ? wcmap[r][c] : CH_WALL;

            if (ch == CH_WALL) {
                frameBuffer[idx].Char.UnicodeChar = L'█';
                frameBuffer[idx].Attributes = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
            }
            else {
                frameBuffer[idx].Char.UnicodeChar = L' ';
                frameBuffer[idx].Attributes = 0;

                if (powerPellet[r][c]) {
                    frameBuffer[idx].Char.UnicodeChar = L'@'; // 표시용
                    frameBuffer[idx].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
                }
                else if (coin[r][c]) {
                    frameBuffer[idx].Char.UnicodeChar = L'·';
                    frameBuffer[idx].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
                }
            }
        }
    }

    int uiRow = SCREEN_ROWS - 1;
    for (int c = 0; c < SCREEN_COLS; c++) {
        int idx = uiRow * SCREEN_COLS + c;
        frameBuffer[idx].Char.UnicodeChar = L' ';
        frameBuffer[idx].Attributes = 0;
    }

    for (int i = 0; i < 4; i++) {
        if (!ghosts[i].alive) continue;
        int gr = ghosts[i].r, gc = ghosts[i].c;
        if (gr < 0 || gr >= MAP_ROWS || gc < 0 || gc >= MAP_COLS) continue;
        int idx = gr * SCREEN_COLS + gc;
        if (ghosts[i].vulnerable || globalVulnerable) {
            frameBuffer[idx].Char.UnicodeChar = L'V';
            frameBuffer[idx].Attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
        }
        else {
            frameBuffer[idx].Char.UnicodeChar = L'G';
            WORD attr =
                (i == 0 ? (FOREGROUND_RED | FOREGROUND_INTENSITY) :
                    i == 1 ? (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE) :
                    i == 2 ? (FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY) :
                    (FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY));
            switch (ghosts[i].gtype)
            {
            case 0: attr = FOREGROUND_RED | FOREGROUND_INTENSITY; break;
            case 1: attr = FOREGROUND_RED | FOREGROUND_GREEN; break;
            case 2: attr = FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY; break;
            case 3: attr = FOREGROUND_RED | FOREGROUND_BLUE | FOREGROUND_INTENSITY; break;
            }
            frameBuffer[idx].Attributes = attr;
        }
    }

    if (player.r >= 0 && player.r < MAP_ROWS && player.c >= 0 && player.c < MAP_COLS) {
        int idx = player.r * SCREEN_COLS + player.c;
        frameBuffer[idx].Char.UnicodeChar = L'C';
        frameBuffer[idx].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }

    wchar_t ui[128];
    swprintf(ui, 128, L" SCORE: %d    LIVES: %d ", player.score, player.lives);
    int startCol = 0; int uiRowIdx = (SCREEN_ROWS - 1) * SCREEN_COLS + startCol;
    for (int i = 0; ui[i] && startCol + i < SCREEN_COLS; i++) {
        frameBuffer[uiRowIdx + i].Char.UnicodeChar = ui[i];
        frameBuffer[uiRowIdx + i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }
}

void render_partial_updates(HANDLE hOut, bool forceFullWrite) {
    COORD bufSize = { (SHORT)SCREEN_COLS, (SHORT)SCREEN_ROWS };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion;

    if (forceFullWrite) {
        writeRegion.Left = 0; writeRegion.Top = 0; writeRegion.Right = SCREEN_COLS - 1; writeRegion.Bottom = SCREEN_ROWS - 1;
        WriteConsoleOutputW(hOut, frameBuffer, bufSize, bufCoord, &writeRegion);
        memcpy(prevFrameBuffer, frameBuffer, sizeof(CHAR_INFO) * SCREEN_ROWS * SCREEN_COLS);
        return;
    }

    for (int r = 0; r < SCREEN_ROWS; r++) {
        int c = 0;
        while (c < SCREEN_COLS) {
            int idx = r * SCREEN_COLS + c;
            if (frameBuffer[idx].Char.UnicodeChar == prevFrameBuffer[idx].Char.UnicodeChar &&
                frameBuffer[idx].Attributes == prevFrameBuffer[idx].Attributes) {
                c++; continue;
            }
            int startC = c; int runLen = 1;
            while (startC + runLen < SCREEN_COLS) {
                int j = r * SCREEN_COLS + startC + runLen;
                if (frameBuffer[j].Char.UnicodeChar != prevFrameBuffer[j].Char.UnicodeChar ||
                    frameBuffer[j].Attributes != prevFrameBuffer[j].Attributes) runLen++; else break;
            }
            for (int k = 0; k < runLen; k++) tmpRowBuffer[k] = frameBuffer[r * SCREEN_COLS + startC + k];
            writeRegion.Left = (SHORT)startC; writeRegion.Top = (SHORT)r; writeRegion.Right = (SHORT)(startC + runLen - 1); writeRegion.Bottom = (SHORT)r;
            COORD writeBufSize = { (SHORT)runLen, 1 };
            COORD writeBufCoord = { 0, 0 };
            WriteConsoleOutputW(hOut, tmpRowBuffer, writeBufSize, writeBufCoord, &writeRegion);
            for (int k = 0; k < runLen; k++) prevFrameBuffer[r * SCREEN_COLS + startC + k] = tmpRowBuffer[k];
            c = startC + runLen;
        }
    }
}

void move_ghost(Ghost* g) {
    if (!g->alive) { g->r = g->sr; g->c = g->sc; g->alive = true; g->vulnerable = false; return; }

    int dr[4] = { -1, 0, 1, 0 };
    int dc[4] = { 0, 1, 0, -1 };
    int opposite_dir[4] = { 2, 3, 0, 1 };

    int valid_dirs[4];
    int num_valid_dirs = 0;

    for (int d = 0; d < 4; d++) {
        if (d == opposite_dir[g->dir]) continue;

        int nr = g->r + dr[d];
        int nc = g->c + dc[d];

        if (!is_wall(nr, nc)) {
            valid_dirs[num_valid_dirs] = d;
            num_valid_dirs++;
        }
    }

    if (num_valid_dirs == 0) {
        int d = opposite_dir[g->dir];
        int nr = g->r + dr[d];
        int nc = g->c + dc[d];
        if (!is_wall(nr, nc)) {
            valid_dirs[0] = d;
            num_valid_dirs = 1;
        }
    }

    if (num_valid_dirs == 0) {
        return;
    }

    int best_dir = -1;

    if (globalVulnerable) {
        int max_dist = -1;
        for (int i = 0; i < num_valid_dirs; i++) {
            int d = valid_dirs[i];
            int nr = g->r + dr[d];
            int nc = g->c + dc[d];
            int dist = manhattan(nr, nc, player.r, player.c);

            if (dist > max_dist) {
                max_dist = dist;
                best_dir = d;
            }
            else if (dist == max_dist) {
                if (rand() % 2 == 0) {
                    best_dir = d;
                }
            }
        }
    }
    else {
        int targetR, targetC;

        if (currentGhostMode == SCATTER) {
            switch (g->gtype) {
            case 0: targetR = 1; targetC = MAP_COLS - 2; break;
            case 1: targetR = MAP_ROWS - 2; targetC = 1; break;
            case 2: targetR = MAP_ROWS - 2; targetC = MAP_COLS - 2; break;
            case 3: targetR = 1; targetC = 1; break;
            default: targetR = player.r; targetC = player.c; break;
            }
        }
        else {
            switch (g->gtype) {
            case 0:
                targetR = player.r;
                targetC = player.c;
                break;
            case 1:
                if (manhattan(g->r, g->c, player.r, player.c) > 8) {
                    targetR = player.r;
                    targetC = player.c;
                }
                else {
                    targetR = MAP_ROWS - 2; targetC = 1;
                }
                break;
            case 2:
            {
                int pivotR = player.r + (currentDirR * 2);
                int pivotC = player.c + (currentDirC * 2);
                if (currentDirR == -1 && currentDirC == 0) {
                    pivotC -= 2;
                }
                int blinkyR = ghosts[0].r;
                int blinkyC = ghosts[0].c;

                int vecR = pivotR - blinkyR;
                int vecC = pivotC - blinkyC;

                targetR = blinkyR + (vecR * 2);
                targetC = blinkyC + (vecC * 2);
                break;
            }
            case 3:
            {
                targetR = player.r + (currentDirR * 4);
                targetC = player.c + (currentDirC * 4);

                if (currentDirR == -1 && currentDirC == 0) {
                    targetC -= 4;
                }
                break;
            }
            default:
                targetR = player.r;
                targetC = player.c;
                break;
            }
        }
        int min_dist = 1000000;
        for (int i = 0; i < num_valid_dirs; i++) {
            int d = valid_dirs[i];
            int nr = g->r + dr[d];
            int nc = g->c + dc[d];
            int dist = manhattan(nr, nc, targetR, targetC);

            if (dist < min_dist) {
                min_dist = dist;
                best_dir = d;
            }
        }
    }

    if (best_dir != -1) {
        g->r = g->r + dr[best_dir];
        g->c = g->c + dc[best_dir];
        g->dir = best_dir;
    }
}

void check_collect_and_collision() {
    if (powerPellet[player.r][player.c]) {
        powerPellet[player.r][player.c] = false;
        player.score += 50;
        globalVulnerable = true;
        powerEndTime = GetTickCount() + 8000;
        for (int i = 0; i < 4; i++) ghosts[i].vulnerable = true;
        playSoundIfExists(SND_POWER, SND_ASYNC);
    }
    else if (coin[player.r][player.c]) {
        coin[player.r][player.c] = false;
        player.score += 10;
        playSoundIfExists(SND_COIN, SND_ASYNC);
    }

    for (int i = 0; i < 4; i++) {
        Ghost* g = &ghosts[i];
        if (!g->alive) continue;
        if (g->r == player.r && g->c == player.c) {
            if (g->vulnerable || globalVulnerable) {
                player.score += 200;
                g->alive = false;
                playSoundIfExists(SND_EATGHOST, SND_ASYNC);
            }
            else {
                player.lives--;
                playSoundIfExists(SND_DIE, SND_ASYNC);

                int bestR = -1, bestC = -1, bestDist = -1;
                for (int r = 0; r < MAP_ROWS; r++) {
                    for (int c = 0; c < MAP_COLS; c++) {
                        if (is_wall(r, c)) continue;
                        int mind = 100000;
                        for (int gg = 0; gg < 4; gg++) {
                            int d = manhattan(r, c, ghosts[gg].sr, ghosts[gg].sc);
                            if (d < mind) mind = d;
                        }
                        if (mind > bestDist) { bestDist = mind; bestR = r; bestC = c; }
                    }
                }
                if (bestR >= 0) { player.r = bestR; player.c = bestC; }
                else { player.r = 12; player.c = 49; }
                for (int j = 0; j < 4; j++) {
                    ghosts[j].r = ghosts[j].sr; ghosts[j].c = ghosts[j].sc;
                    ghosts[j].alive = true; ghosts[j].vulnerable = false;
                }
                globalVulnerable = false;
                currentGhostMode = SCATTER;
                modePhase = 0;
                modeSwitchTime = GetTickCount() + phaseDurations[0];
                Sleep(600);
                return;
            }
        }
    }
}

//게임 오버 화면
void show_game_over(HANDLE hOut, int finalScore) {
    playSoundIfExists(SND_GAMEOVER, SND_ASYNC);

    for (int i = 0; i < SCREEN_ROWS * SCREEN_COLS; i++) {
        frameBuffer[i].Char.UnicodeChar = L' ';
        frameBuffer[i].Attributes = 0;
    }

    wchar_t title[] = L"======== 게임 오버 ========";
    wchar_t scoreLine[128];
    swprintf(scoreLine, 128, L"FINAL SCORE: %d", finalScore);
    wchar_t prompt[] = L"엔터 키를 누르면 종료됩니다";

    int rr = 10;
    int cc_title = (SCREEN_COLS - (int)wcslen(title)) / 2;
    int cc_score = (SCREEN_COLS - (int)wcslen(scoreLine)) / 2;
    int cc_prompt = (SCREEN_COLS - (int)wcslen(prompt)) / 2;

    for (int i = 0; title[i]; i++) {
        frameBuffer[rr * SCREEN_COLS + cc_title + i].Char.UnicodeChar = title[i];
        frameBuffer[rr * SCREEN_COLS + cc_title + i].Attributes = FOREGROUND_RED | FOREGROUND_INTENSITY;
    }
    for (int i = 0; scoreLine[i]; i++) {
        frameBuffer[(rr + 2) * SCREEN_COLS + cc_score + i].Char.UnicodeChar = scoreLine[i];
        frameBuffer[(rr + 2) * SCREEN_COLS + cc_score + i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }
    for (int i = 0; prompt[i]; i++) {
        frameBuffer[(rr + 4) * SCREEN_COLS + cc_prompt + i].Char.UnicodeChar = prompt[i];
        frameBuffer[(rr + 4) * SCREEN_COLS + cc_prompt + i].Attributes = FOREGROUND_GREEN | FOREGROUND_INTENSITY;
    }

    COORD bufSize = { (SHORT)SCREEN_COLS, (SHORT)SCREEN_ROWS };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(SCREEN_COLS - 1), (SHORT)(SCREEN_ROWS - 1) };
    WriteConsoleOutputW(hOut, frameBuffer, bufSize, bufCoord, &writeRegion);

    while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) Sleep(50);
}

int main() {
    srand((unsigned int)time(NULL));
    setConsoleSizeAndFont();
    hideCursor();

    frameBuffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_ROWS * SCREEN_COLS);
    prevFrameBuffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_ROWS * SCREEN_COLS);
    tmpRowBuffer = (CHAR_INFO*)malloc(sizeof(CHAR_INFO) * SCREEN_COLS);

    if (!frameBuffer || !prevFrameBuffer || !tmpRowBuffer) {
        fprintf(stderr, "메모리 할당 실패\n");
        return 1;
    }

    for (int i = 0; i < SCREEN_ROWS * SCREEN_COLS; i++) {
        prevFrameBuffer[i].Char.UnicodeChar = 0xFFFF;
        prevFrameBuffer[i].Attributes = 0xFFFF;
    }

    HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);

    for (int i = 0; i < SCREEN_ROWS * SCREEN_COLS; i++) {
        frameBuffer[i].Char.UnicodeChar = L' ';
        frameBuffer[i].Attributes = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    }

    wchar_t* intro1 = L"==================================";
    wchar_t* intro2 = L"===         Pac-Man!         ===";
    wchar_t* intro3 = L"   엔터 키를 누르면 게임이 시작됩니다   ";

    int r0 = 10, c0 = 33;

    for (int i = 0; intro1[i]; i++) frameBuffer[r0 * SCREEN_COLS + c0 + i].Char.UnicodeChar = intro1[i];
    for (int i = 0; intro2[i]; i++) frameBuffer[(r0 + 1) * SCREEN_COLS + c0 + i].Char.UnicodeChar = intro2[i];
    for (int i = 0; intro3[i]; i++) frameBuffer[(r0 + 2) * SCREEN_COLS + c0 + i].Char.UnicodeChar = intro3[i];
    for (int i = 0; intro1[i]; i++) frameBuffer[(r0 + 3) * SCREEN_COLS + c0 + i].Char.UnicodeChar = intro1[i];

    COORD bufSize = { (SHORT)SCREEN_COLS, (SHORT)SCREEN_ROWS };
    COORD bufCoord = { 0, 0 };
    SMALL_RECT writeRegion = { 0, 0, (SHORT)(SCREEN_COLS - 1), (SHORT)(SCREEN_ROWS - 1) };

    WriteConsoleOutputW(hOut, frameBuffer, bufSize, bufCoord, &writeRegion);

    while (!(GetAsyncKeyState(VK_RETURN) & 0x8000))
        Sleep(40);

    playSoundIfExists(SND_START, SND_ASYNC);

    system("cls");
    init_world();

    modeSwitchTime = GetTickCount() + phaseDurations[0];
    currentGhostMode = SCATTER;
    modePhase = 0;

    compose_frameBuffer_from_game_state();
    render_partial_updates(hOut, true);

    LARGE_INTEGER freq, lastTime, curTime;
    QueryPerformanceFrequency(&freq);
    QueryPerformanceCounter(&lastTime);

    DWORD lastGhostMoveTick = GetTickCount();
    DWORD lastRenderTick = GetTickCount();
    const DWORD renderIntervalMs = 16;

    bool running = true;
    while (running) {
        QueryPerformanceCounter(&curTime);
        double dt = (double)(curTime.QuadPart - lastTime.QuadPart) / (double)freq.QuadPart;
        lastTime = curTime;

        bool anyDirKey = false;
        int newDirR = 0, newDirC = 0;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000 || GetAsyncKeyState('A') & 0x8000) { newDirR = 0; newDirC = -1; anyDirKey = true; }
        else if (GetAsyncKeyState(VK_RIGHT) & 0x8000 || GetAsyncKeyState('D') & 0x8000) { newDirR = 0; newDirC = 1; anyDirKey = true; }
        if (GetAsyncKeyState(VK_UP) & 0x8000 || GetAsyncKeyState('W') & 0x8000) { newDirR = -1; newDirC = 0; anyDirKey = true; }
        else if (GetAsyncKeyState(VK_DOWN) & 0x8000 || GetAsyncKeyState('S') & 0x8000) { newDirR = 1; newDirC = 0; anyDirKey = true; }

        if (anyDirKey) {
            desiredDirR = newDirR;
            desiredDirC = newDirC;

            if (currentDirR == 0 && currentDirC == 0) {
                int nextR = player.r + desiredDirR;
                int nextC = player.c + desiredDirC;
                if (!is_wall(nextR, nextC)) {
                    currentDirR = desiredDirR;
                    currentDirC = desiredDirC;
                    playerMoveAcc = 0.0;
                }
            }
        }

        if (GetAsyncKeyState(VK_ESCAPE) & 0x8000) break;

        playerMoveAcc += dt * playerSpeed;
        while (playerMoveAcc >= 1.0) {
            if ((desiredDirR != 0 || desiredDirC != 0) &&
                (desiredDirR != currentDirR || desiredDirC != currentDirC)) {

                int tryR = player.r + desiredDirR;
                int tryC = player.c + desiredDirC;

                if (!is_wall(tryR, tryC)) {
                    currentDirR = desiredDirR;
                    currentDirC = desiredDirC;
                }
            }

            if (currentDirR == 0 && currentDirC == 0) {
                playerMoveAcc = 0.0;
                break;
            }

            int targetR = player.r + currentDirR;
            int targetC = player.c + currentDirC;

            if (!is_wall(targetR, targetC)) {
                playerMoveAcc -= 1.0;
                player.r = targetR;
                player.c = targetC;
                check_collect_and_collision();
            }
            else {
                playerMoveAcc = 0.0;
                break;
            }
        }

        DWORD nowTick = GetTickCount();

        if (modePhase < 7 && nowTick >= modeSwitchTime) {
            modePhase++;
            currentGhostMode = (currentGhostMode == SCATTER) ? CHASE : SCATTER;
            if (modePhase < 7) {
                modeSwitchTime = nowTick + phaseDurations[modePhase];
            }
            else {
                currentGhostMode = CHASE;
                modePhase = 7;
            }
        }

        if (nowTick - lastGhostMoveTick >= ghostMoveInterval) {
            for (int i = 0; i < 4; i++) move_ghost(&ghosts[i]);
            check_collect_and_collision();
            lastGhostMoveTick = nowTick;
        }

        if (globalVulnerable && GetTickCount() >= powerEndTime) {
            globalVulnerable = false;
            for (int i = 0; i < 4; i++) ghosts[i].vulnerable = false;
        }

        if (nowTick - lastRenderTick >= renderIntervalMs) {
            compose_frameBuffer_from_game_state();
            bool forceFull = (prevFrameBuffer[0].Char.UnicodeChar == 0xFFFF);
            render_partial_updates(hOut, forceFull);
            lastRenderTick = nowTick;
        }

        int remain = 0;
        for (int r = 0; r < MAP_ROWS; r++)
            for (int c = 0; c < MAP_COLS; c++)
                if (coin[r][c] || powerPellet[r][c]) remain++;

        if (remain == 0) {
            playSoundIfExists(SND_CLEAR, SND_ASYNC);

            for (int i = 0; i < SCREEN_ROWS * SCREEN_COLS; i++) {
                frameBuffer[i].Char.UnicodeChar = L' ';
                frameBuffer[i].Attributes = 0;
            }
            wchar_t msg1[128]; swprintf(msg1, 128, L"!! CONGRATS !!  ALL COLLECTED");
            wchar_t msg2[128]; swprintf(msg2, 128, L"FINAL SCORE: %d", player.score);
            int rr = 10, cc = 30;
            for (int i = 0; msg1[i]; i++) frameBuffer[(rr)*SCREEN_COLS + cc + i].Char.UnicodeChar = msg1[i];
            for (int i = 0; msg2[i]; i++) frameBuffer[(rr + 2) * SCREEN_COLS + cc + i].Char.UnicodeChar = msg2[i];
            WriteConsoleOutputW(hOut, frameBuffer, bufSize, bufCoord, &writeRegion);
            while (!(GetAsyncKeyState(VK_RETURN) & 0x8000)) Sleep(50);
            break;
        }

        if (player.lives <= 0) { show_game_over(hOut, player.score); break; }

        Sleep(1);
    }

    PlaySoundW(NULL, NULL, 0);

    free(frameBuffer); free(prevFrameBuffer); free(tmpRowBuffer);
    HANDLE stdOut = GetStdHandle(STD_OUTPUT_HANDLE);
    SetConsoleActiveScreenBuffer(stdOut);
    system("cls");
    return 0;
}
