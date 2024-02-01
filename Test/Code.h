typedef struct Bitmap
{
    unsigned int * memory;
    int width;
    int height;
} Bitmap;

void FillWithColor(Bitmap *bitmap, unsigned int color)
{
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
void DrawRectMinMax(Bitmap *bitmap, int min_row, int min_col, int max_row, int max_col, unsigned int color)
{
    for(int row = min_row; row <= max_row; row++)
    {
        for(int col = min_col; col <= max_col; col++)
        {
            int index = row * bitmap->width + col;
            bitmap->memory[index] = color;
        }
    }
}