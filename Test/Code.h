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

float2 TurnToRight(float2 v)
{
    return Float2XY(-v.y, v.x);
}

float2 mul_float_float2(float x, float2 v)
{
    return Float2XY(x * v.x, x * v.y);
}

float2 sub_float2(float2 p1, float2 p2)
{
    return Float2XY(p1.x - p2.x, p1.y - p2.y);
}

float2 add_float2(float2 p1, float2 p2)
{
    return Float2XY(p1.x + p2.x, p1.y + p2.y);
}

Quad2 GetRotatedQuadAroundPoint(float2 center, float2 cos_sin, float2 size)
{
    Quad2 q = {};
    float2 y_dir = cos_sin;
    float2 x_dir = TurnToRight(cos_sin);
    float2 to_y = mul_float_float2((0.5f * size.y), y_dir);
    float2 to_x = mul_float_float2((0.5f * size.x), x_dir);
    float2 top = add_float2(center, to_y);
    float2 bottom = sub_float2(center, to_y);
    q.p[0] = add_float2(top, to_x);
    q.p[1] = sub_float2(top, to_x);
    q.p[2] = sub_float2(bottom, to_x);
    q.p[3] = add_float2(bottom, to_x);
    return q;
}

int TurnsRight(float2 p0, float2 p1, float2 p2)
{
    float2 d0 = sub_float2(p1, p0);
    float2 d1 = sub_float2(p2, p0);
    float det = (d0.x * d1.y) - (d0.y * d1.x);
    int turns_right = (det < 0.0f);
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

typedef struct Input
{
    float2 screen_size;
    float time;
} Input;

float Min2(float x, float y)
{
    if(x < y)
    {
        return x;
    }
    return y;
}

float cosf(float x);

float sinf(float x);

void Update(Input *input, Bitmap *bitmap)
{
    float min_side = 0.75f * Min2(input->screen_size.x, input->screen_size.y);
    float2 mid = mul_float_float2(0.5f, input->screen_size);
    float2 side = Float2XY(min_side, min_side);
    float2 min = sub_float2(mid, mul_float_float2(0.5f, side));
    float2 max = add_float2(mid, mul_float_float2(0.5f, side));
    float2 cos_sin = Float2XY(cosf(input->time), sinf(input->time));
    Quad2 quad = GetRotatedQuadAroundPoint(mid, cos_sin, side);
    DrawQuad2(bitmap, quad, (unsigned int)0x00FF00);
}
