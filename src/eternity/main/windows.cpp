#include <windows.h>
#include <windowsx.h>

int mouse_x = 0;
int mouse_y = 0;
bool mouse_down = false;

LRESULT WINAPI MsgProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam )
{
	switch( msg )
	{
		case WM_DESTROY:
			PostQuitMessage( 0 );
			return 0;

		case WM_PAINT:
			ValidateRect( hWnd, NULL );
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

int main(int argc, const char* argv[])
{
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
