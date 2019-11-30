struct Bitmap
{
	int width;
	int height;
	unsigned int *memory;
};

static void ClearBitmap(Bitmap *bitmap, unsigned int color_code)
{
	unsigned int *pixel = bitmap->memory;
	for(int row = 0; row < bitmap->height; row++)
	{
		for(int col = 0; col < bitmap->width; col++)
		{
			*pixel = 0xFF0000FF;
			pixel++;
		}
	}
}
	