typedef struct Bitmap
{
    unsigned int *memory;
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

void SetPixelColor(Bitmap *bitmap, int row, int col, unsigned int color)
{
    bitmap->memory[row * bitmap->width + col] = color;
}

typedef struct float2
{
    float x;
    float y;
} float2;

typedef struct Quad2
{
    float2 p[4];
} Quad2;

float2 Float2XY(float x, float y)
{
    float2 result = {};
    result.x = x;
    result.y = y;
    return result;
}

int TurnsRight(float2 p0, float2 p1, float2 p2)
{
    float2 d0 = p1 - p0;
    float2 d1 = p2 - p1;
    float det = (d0.x * d1.y) - (d0.y * d1.x);
    int turns_right = (det < 0.0);
    return turns_right;
}

int IsPointInQuad2(float2 p, Quad2 q)
{
    int is_inside = 1;
    is_inside &= TurnsRight(q.p[0], q.p[1], p);
    is_inside &= TurnsRight(q.p[1], q.p[2], p);
    is_inside &= TurnsRight(q.p[2], q.p[3], p);
    is_inside &= TurnsRight(q.p[3], q.p[0], p);
    return is_inside;
}

void DrawQuad2(Bitmap *bitmap, Quad2 quad, unsigned int color)
{
    float min_x = quad.p[0].x;
    float max_x = quad.p[0].x;
    float min_y = quad.p[0].y;
    float max_y = quad.p[0].y;
    for(int i = 1; i < 4; i++)
    {
        float x = quad.p[i].x;
        float y = quad.p[i].y;
        if(x < min_x)
        {
            min_x = x;
        }
        if(x > max_x)
        {
            max_x = x;
        }
        if(y < min_y)
        {
            min_y = y;
        }
        if(y > max_y)
        {
            max_y = y;
        }
    }
    for(int row = (int)min_y; row < (int)max_y + 1; row++)
    {
        for(int col = (int)min_x; col < (int)max_x + 1; col++)
        {
            float2 p = Float2XY((float)col, (float)row);
            if(IsPointInQuad2(p, quad))
            {
                SetPixelColor(bitmap, row, col, color);
            }
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
