#define UNICODE
#include <windows.h>
#include <stdio.h>

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
		case WM_KEYDOWN:
		{
			if(wparam == VK_ESCAPE)
				DestroyWindow(window);
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			break;
		}
		default:
		{
			return DefWindowProcW(window, message, wparam, lparam);
		}
		
		return 0;
	}
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR p, int pn)
{
	WNDCLASSEX wc = {};
	wc.cbSize = sizeof(WNDCLASSEXW);
	wc.style = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc = &WindowProc;
	wc.hInstance = instance;
	wc.hIcon = LoadIconW(0, IDI_APPLICATION);
	wc.hCursor = LoadCursorW(0, IDC_ARROW);
	wc.lpszClassName = L"M64 Test";
	wc.hIconSm = LoadIconW(0, IDI_APPLICATION);
	
	if(!RegisterClassExW(&wc))
	{
		fprintf(stderr, "ERROR Creating Window Class\n");
		return GetLastError();
	}
	
	RECT rect = {0, 0, 1024, 768};
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, FALSE, WS_EX_OVERLAPPEDWINDOW);
	
	LONG width = rect.right - rect.left;
	LONG height = rect.bottom - rect.top;
	
	HWND window = CreateWindowExW(WS_EX_OVERLAPPEDWINDOW,
								  wc.lpszClassName,
								  L"M64",
								  WS_OVERLAPPEDWINDOW | WS_VISIBLE,
								  CW_USEDEFAULT, CW_USEDEFAULT,
								  width, height, 0, 0, instance, 0);

	if(!window)
	{
		fprintf(stderr, "ERROR Creating Window\n");
		return GetLastError();
	}		
	
	int running = 1;
	while(running)
	{
		MSG message = {};
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			if(message.message == WM_QUIT)
				running = 0;
			TranslateMessage(&message);
			DispatchMessageW(&message);
		}
		
		Sleep(1);
	}

	return 0;
}