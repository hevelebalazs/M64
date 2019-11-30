#include <Windows.h>

#include "m64_render.hpp"

#define Assert(condition) if(!(condition)) {DebugBreak();}

#define func

static Bitmap global_bitmap;

struct MemArena
{
	char *base_address;
	int used_size;
	int max_size;
};

static MemArena
func CreateMemArena(void *memory, int size)
{
	MemArena arena = {};
	arena.base_address = (char *)memory;
	arena.max_size = size;
	arena.used_size = 0;
	return arena;
}

static void *
func ArenaAlloc(MemArena *arena, int size)
{
	Assert(arena->used_size + size <= arena->max_size);
	char *result = arena->base_address + arena->used_size;
	arena->used_size += size;
	return result;
}

static void
func ArenaPopTo(MemArena *arena, void *address)
{
	arena->used_size = (int)((char *)address - arena->base_address);
	Assert(arena->used_size >= 0);
	Assert(arena->used_size <= arena->max_size);
}

static void *
func ArenaPushData(MemArena *arena, int size, void *data)
{
	char *copy_to = (char *)ArenaAlloc(arena, size);
	char *copy_from = (char *)data;
	for(int index = 0; index < size; index++)
	{
		copy_to[index] = copy_from[index];
	}
	return copy_to;
}

#define ArenaAllocType(arena, type) (type *)ArenaAlloc(arena, sizeof(type))
#define ArenaAllocArray(arena, type, size) (type *)ArenaAlloc(arena, (size) * sizeof(type))
#define ArenaPushVar(arena, variable) ArenaPushData(arena, sizeof(variable), &(variable))

static bool global_running;
static char global_bitmap_memory[256 * 1024 * 1024];
static MemArena global_bitmap_arena;

static void
func ResizeGlobalBitmap(int width, int height)
{
	Assert(width > 0);
	Assert(height > 0);
	
	global_bitmap.width = width;
	global_bitmap.height = height;
	
	int size = (width * height);
	global_bitmap_arena.used_size = size;
	
	global_bitmap.memory = (unsigned int *)global_bitmap_arena.base_address;
}

static BITMAPINFO
func GetBitmapInfo(Bitmap *bitmap)
{
	BITMAPINFO info = {};
	BITMAPINFOHEADER *header = &info.bmiHeader;
	header->biSize = sizeof(*header);
	header->biWidth = bitmap->width;
	header->biHeight = -bitmap->height;
	header->biPlanes = 1;
	header->biBitCount = 32;
	header->biCompression = BI_RGB;
	return info;
}

static LRESULT CALLBACK
func WinCallback(HWND window, UINT message, WPARAM wparam, LPARAM lparam)
{
	LRESULT result = 0;
	switch(message)
	{
		case WM_SIZE:
		{
			RECT rect = {};
			GetClientRect(window, &rect);
			int width  = rect.right - rect.left;
			int height = rect.bottom - rect.top;
			
			ResizeGlobalBitmap(width, height);
			break;
		}
		case WM_DESTROY:
		case WM_CLOSE:
		{
			global_running = false;
			break;
		}
		default:
		{
			result = DefWindowProc(window, message, wparam, lparam);
			break;
		}
	}
	return result;
}

void
func WinUpdate()
{
	Bitmap *bitmap = &global_bitmap;
	ClearBitmap(bitmap, 0xFF0000FF);
}

int CALLBACK
func WinMain(HINSTANCE instance, HINSTANCE prev_instance, LPSTR cmd_line, int cmd_show)
{
	global_running = true;
	
	global_bitmap_arena = CreateMemArena(global_bitmap_memory, 256 * 1024 * 1024);
	
	WNDCLASS win_class = {};
	win_class.style = CS_OWNDC;
	win_class.lpfnWndProc = WinCallback;
	win_class.hInstance = instance;
	win_class.lpszClassName = "M64WindowClass";
	
	RegisterClass(&win_class);
	HWND window = CreateWindowEx(
		0,
		win_class.lpszClassName,
		"M64",
		WS_OVERLAPPEDWINDOW | WS_VISIBLE,
		CW_USEDEFAULT,
		CW_USEDEFAULT,
		1000,
		1000,
		0,
		0,
		instance,
		0
	);
	Assert(window != 0);
	
	MSG message = {};
	while(global_running)
	{
		while(PeekMessage(&message, 0, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&message);
			DispatchMessage(&message);
		}
		
		WinUpdate();
		
		RECT rect = {};
		GetClientRect(window, &rect);
		
		HDC context = GetDC(window);
		Bitmap *bitmap = &global_bitmap;
		BITMAPINFO bitmap_info = GetBitmapInfo(bitmap);
		
		int width  = rect.right - rect.left;
		int height = rect.bottom - rect.top;
		
		StretchDIBits(context,
					  0, 0, bitmap->width, bitmap->height,
					  0, 0, width, height,
					  bitmap->memory,
					  &bitmap_info,
					  DIB_RGB_COLORS,
					  SRCCOPY
		);
		
		ReleaseDC(window, context);
	}
	
	return 0;
}