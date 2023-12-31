#define UNICODE
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>

typedef struct Bitmap
{
	unsigned int *memory;
	int width;
	int height;
} Bitmap;

static Bitmap global_bitmap;

static void ResizeBitmap(Bitmap *bitmap, int width, int height)
{
	free(bitmap->memory);
	
	bitmap->width = width;
	bitmap->height = height;
	
	bitmap->memory = malloc(width * height * sizeof(unsigned int));
}

static void DrawScene(Bitmap *bitmap)
{
	unsigned int color = 0xAA5500;
	unsigned int *pixel = bitmap->memory;
	for(int row = 0; row < bitmap->height; row++)
	{
		for(int col = 0; col < bitmap->width; col++)
		{
			*pixel = color;
			pixel++;
		}
	}
}

LRESULT CALLBACK WindowProc(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	switch(message)
	{
		case WM_SIZE:
		{
			RECT rect = {};
			GetClientRect(window, &rect);
			int width = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			
			ResizeBitmap(&global_bitmap, width, height);
			break;
		}
		case WM_DESTROY:
		case WM_CLOSE:
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
	
	Bitmap *bitmap = &global_bitmap;
	
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
		
		HDC context = GetDC(window);
		BITMAPINFO bitmap_info = {};
		BITMAPINFOHEADER *header = &bitmap_info.bmiHeader;
		header->biSize = sizeof(*header);
		header->biWidth = bitmap->width;
		header->biHeight = -bitmap->height;
		header->biPlanes = 1;
		header->biBitCount = 32;
		header->biCompression = BI_RGB;
		
		RECT rect = {};
		GetClientRect(window, &rect);
		
		int width = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		
		DrawScene(bitmap);
		
		StretchDIBits(context,
					  0, 0, bitmap->width, bitmap->height,
					  0, 0, width, height,
					  bitmap->memory,
					  &bitmap_info,
					  DIB_RGB_COLORS,
					  SRCCOPY);
		ReleaseDC(window, context);
	}

	return 0;
}