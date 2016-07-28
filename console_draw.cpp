#include <stdlib.h>
#include <Windows.h>
#include <stdio.h>
#include <math.h>

#include "console_draw.h"

typedef struct SConsoleDraw
{
	 int startx,starty;
	 int width,height;
	 CHAR_INFO *lpbuffer_back;
	 CHAR_INFO *lpbuffer_front;
	 CHAR_INFO *lpbuffer;
	 CONSOLE_CURSOR_INFO lpConsoleCursorInfo;
	 HANDLE hOut;
	 COORD dwBufferSize;
	 COORD dwBufferCoord;
	 SMALL_RECT lpWriteRegion;

} SConsoleDraw;

HANDLE CD_GetConsoleHandle()
{
	HANDLE hCD;

	hCD = (SConsoleDraw*) malloc(sizeof(SConsoleDraw));
	if (hCD == NULL) {
		printf("ERROR IN GETTING CONSOLE HANDLE");
	}
	return hCD;
}

HANDLE CD_CreateWindow(int X, int Y, int WIDTH, int HEIGHT)
{
	SConsoleDraw* hCD = (SConsoleDraw*) CD_GetConsoleHandle();

	hCD->width=WIDTH;
	hCD->height=HEIGHT;
	
	hCD->lpbuffer_back = (CHAR_INFO*)malloc((WIDTH + 1) * (HEIGHT + 1) * sizeof(CHAR_INFO));
	hCD->lpbuffer_front = (CHAR_INFO*)malloc((WIDTH + 1) * (HEIGHT + 1) * sizeof(CHAR_INFO));
	hCD->lpbuffer = hCD->lpbuffer_front;

	hCD->startx=X;
	hCD->starty=Y;

	hCD->dwBufferCoord.X = X;
	hCD->dwBufferCoord.Y = Y;
   
	hCD->dwBufferSize.X = WIDTH;
	hCD->dwBufferSize.Y = HEIGHT;
	
    hCD->lpWriteRegion.Top = Y;
    hCD->lpWriteRegion.Bottom = Y + HEIGHT - 1;
    hCD->lpWriteRegion.Left = X;
    hCD->lpWriteRegion.Right = X + WIDTH - 1;

	hCD->hOut = GetStdHandle(STD_OUTPUT_HANDLE);
	
	hCD->lpConsoleCursorInfo.bVisible=0;
	hCD->lpConsoleCursorInfo.dwSize=1;
	
	CONSOLE_FONT_INFOEX cfi;
	cfi.cbSize = sizeof cfi;
	cfi.nFont = 0;
	cfi.dwFontSize.X = 8;
	cfi.dwFontSize.Y = 8;
	cfi.FontFamily = FF_DONTCARE;
	cfi.FontWeight = FW_NORMAL;
	wcscpy_s(cfi.FaceName, L"Raster Fonts");
	SetCurrentConsoleFontEx(GetStdHandle(STD_OUTPUT_HANDLE), FALSE, &cfi);


	SetConsoleWindowInfo(hCD->hOut, 1, &hCD->lpWriteRegion);
	SetConsoleCursorInfo(hCD->hOut, &hCD->lpConsoleCursorInfo);

	ReadConsoleOutput(hCD->hOut, hCD->lpbuffer, hCD->dwBufferSize, hCD->dwBufferCoord, &hCD->lpWriteRegion);
	
	SetConsoleScreenBufferSize(hCD->hOut, hCD->dwBufferSize);
	SetConsoleDisplayMode(hCD->hOut, 2, &hCD->dwBufferSize);

	return ((void*)hCD);
}

void CD_Refresh(HANDLE hCD)
{
	int i;
	SConsoleDraw* hCDTmp = (SConsoleDraw*) hCD;

	for (i=0;i<hCDTmp->width * hCDTmp->height;i++)
	{
		hCDTmp->lpbuffer[i].Char.AsciiChar=' ';
		hCDTmp->lpbuffer[i].Attributes=0;
	}
}

void CD_SetBufferChar(HANDLE hCD, int X, int Y, char CHAR, WORD ATTR)
{
	SConsoleDraw* hCDTmp = (SConsoleDraw*) hCD;

	int listnum = Y * hCDTmp->width + X;
	if (X < hCDTmp->width && Y < hCDTmp->height && X >= 0 && Y >= 0)
	{
		hCDTmp->lpbuffer[listnum].Char.AsciiChar = CHAR;
		hCDTmp->lpbuffer[listnum].Attributes = ATTR;
	}
}

CHAR CD_GetBufferChar(HANDLE hCD, int X, int Y) {
	SConsoleDraw* hCDTmp = (SConsoleDraw*)hCD;

	int listnum = Y * hCDTmp->width + X;
	if (X < hCDTmp->width && Y < hCDTmp->height && X >= 0 && Y >= 0) {
		return hCDTmp->lpbuffer[listnum].Char.AsciiChar;
	}
	return 0;
}

WORD CD_GetBufferAttr(HANDLE hCD, int X, int Y) {
	SConsoleDraw* hCDTmp = (SConsoleDraw*)hCD;

	int listnum = Y * hCDTmp->width + X;
	if (X < hCDTmp->width && Y < hCDTmp->height && X >= 0 && Y >= 0) {
		return hCDTmp->lpbuffer[listnum].Attributes;
	}
	return 0;
}

void CD_DrawScreen(HANDLE hCD)
{
	SConsoleDraw* hCDTmp = (SConsoleDraw*) hCD;
	WriteConsoleOutput(hCDTmp->hOut, hCDTmp->lpbuffer, hCDTmp->dwBufferSize, hCDTmp->dwBufferCoord, &hCDTmp->lpWriteRegion);
	if (hCDTmp->lpbuffer == hCDTmp->lpbuffer_front) {
		hCDTmp->lpbuffer = hCDTmp->lpbuffer_back;
	} else {
		hCDTmp->lpbuffer = hCDTmp->lpbuffer_front;
	}
	
}

void CD_SetCursorPos(HANDLE hCD, int x, int y)
{
	SConsoleDraw* hCDTmp = (SConsoleDraw*) hCD;
	COORD tmp;
	tmp.X=x;
	tmp.Y=y;
	SetConsoleCursorPosition(hCDTmp->hOut, tmp);
}
