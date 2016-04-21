#include <windows.h>
#include <windowsx.h>
#include <stdio.h>

#include "fontanell/ttf.h"

int mouse_x = 0;
int mouse_y = 0;
bool mouse_down = false;

struct shape
{
	float x[64];
	float y[64];
	int n;
};

fontanell::ttf::data* font = 0;

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_PAINT:
			{
				HDC dc = GetDC(hWnd);
				HBRUSH br = GetStockBrush(WHITE_BRUSH);
				HPEN pen = GetStockPen(WHITE_PEN);
				SelectBrush(dc, br);
				SelectPen(dc, pen);
				MoveToEx(dc, 0, 0, 0);
				LineTo(dc, 100, 100);
				ValidateRect(hWnd, NULL);
				ReleaseDC(hWnd, dc);
			}
			return 0;

		case WM_MOUSEMOVE:
			mouse_x = GET_X_LPARAM(lParam); 
			mouse_y = GET_Y_LPARAM(lParam); 
			return 0;

		case WM_LBUTTONDOWN:
			mouse_down = true;
			break;

		case WM_LBUTTONUP:
			mouse_down = false;
			break;

		case WM_SIZE:
			RECT r;
			GetClientRect(hWnd, &r);
			return 0;
	}

	return DefWindowProc( hWnd, msg, wParam, lParam );
}

HWND create_window()
{
	WNDCLASSEXA wc =
	{
		sizeof(WNDCLASSEX), CS_CLASSDC, MsgProc, 0L, 0L,
		GetModuleHandle( NULL ), NULL, NULL, NULL, NULL,
		"ETERNITY", NULL
	};

	wc.hIcon = 0;
	wc.hCursor = LoadCursor( NULL, IDC_ARROW );
	wc.hbrBackground = GetStockBrush(BLACK_BRUSH);

	::RegisterClassExA( &wc );

	RECT rc = { 
		0, 
		0,
		1280,
		720
	};

	AdjustWindowRect(&rc, WS_OVERLAPPEDWINDOW, FALSE);
	
	HWND hWnd = ::CreateWindowA("ETERNITY", "Eternity",
								WS_OVERLAPPEDWINDOW, 100, 100, rc.right-rc.left, rc.bottom-rc.top,
								NULL, NULL, wc.hInstance, NULL );
	
	return hWnd;
}

char fbuf[1024*1024*10];

int main(int argc, const char* argv[])
{
	FILE *fp = fopen("../data/raw/font/Lacuna.ttf", "rb");
	size_t sz = fread(fbuf, 1, sizeof(fbuf), fp);
	fclose(fp);

	font = fontanell::ttf::open(fbuf, sz);

	uint32_t in[2] = {(uint32_t)'A', (uint32_t)'B'};
	uint32_t glyphs[2];
	fontanell::ttf::map(font, in, 2, glyphs);
	fontanell::ttf::get_glyph(font, glyphs[0]);

	HWND window = create_window();
	ShowWindow(window, SW_SHOW);

	bool quit = false;
	while (!quit)
	{
		MSG m;
		WaitMessage();
		while (PeekMessage(&m, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&m);
			DispatchMessage(&m);

			if (m.message == WM_QUIT)
			{
				quit = true;
			}
		}
	}

	return 0;
}
