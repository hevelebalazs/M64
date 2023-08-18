struct tdef Bitmap
{
	int width;
	int height;
	unsigned int *memory;
	MemArena *arena;
};

static void 
func ResizeBitmap(Bitmap *bitmap, int width, int height)
{
	bitmap->width = width;
	bitmap->height = height;

	bitmap->arena->used_size = 0;
	bitmap->memory = ArenaPushArray(bitmap->arena, width * height, unsigned int);
}

static int
func MinInt(int v1, int v2)
{
	int min = (v1 < v2) ? v1 : v2;
	return min;
}

static int
func MaxInt(int v1, int v2)
{
	int max = (v1 > v2) ? v1 : v2;
	return max;
}

static void
func BitmapDrawRect(Bitmap *bitmap, int minx, int maxx, int miny, int maxy, unsigned int color)
{
	minx = MaxInt(minx, 0);
	maxx = MinInt(maxx, bitmap->width - 1);
	miny = MaxInt(miny, 0);
	maxy = MinInt(maxy, bitmap->height - 1);

	for(int y = miny; y <= maxy; y++)
	{
		unsigned int *row = bitmap->memory + y * bitmap->width;
		for(int x = minx; x <= maxx; x++)
		{
			row[x] = color;
		}
	}
}

enum tdef BitmapRenderId // Int32
{
	BitmapStopRenderId,
	BitmapDrawRectId // minx, maxx, miny, maxy: Int32, color: UInt32
};

struct tdef MemoryPosition
{
	char *at;
};

static int
func ReadInt(MemoryPosition *pos)
{
	int value = *(int *)pos->at;
	pos->at += sizeof(int);
	return value;
}

static unsigned int
func ReadUInt(MemoryPosition *pos)
{
	unsigned int value = *(unsigned int *)pos->at;
	pos->at += sizeof(unsigned int);
	return value;
}

static void
func ExecuteRenderCommands(Bitmap *bitmap, MemArena *arena)
{
	MemoryPosition pos = {};
	pos.at = arena->memory;
	while(1)
	{
		int command = ReadInt(&pos);
		switch(command)
		{
			case BitmapStopRenderId:
			{
				return;
			}
			case BitmapDrawRectId:
			{
				int minx = ReadInt(&pos);
				int maxx = ReadInt(&pos);
				int miny = ReadInt(&pos);
				int maxy = ReadInt(&pos);
				unsigned int color = ReadUInt(&pos);
				BitmapDrawRect(bitmap, minx, maxx, miny, maxy, color);
				break;
			}
			default:
			{
				DebugBreak();
			}
		}
	}
}
