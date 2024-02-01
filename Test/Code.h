typedef struct Bitmap
{
    unsigned int * memory;
    int width;
    int height;
} Bitmap;

void UpdateBitmap(Bitmap *bitmap)
{
    unsigned int *pixel = bitmap->memory;
    for(int row = 0; row < bitmap->height; row++)
    {
        for(int col = 0; col < bitmap->width; col++)
        {
            *pixel = (unsigned int)0x005555;
            pixel++;
        }
    }
}