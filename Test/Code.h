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

float2 float2_xy(float x, float y)
{
    float2 result = {};
    result.x = x;
    result.y = y;
    return result;
}

float2 TurnToRight(float2 v)
{
    return float2_xy(-v.y, v.x);
}

float2 mul_float_float2(float x, float2 v)
{
    return float2_xy(x * v.x, x * v.y);
}

float2 sub_float2(float2 p1, float2 p2)
{
    return float2_xy(p1.x - p2.x, p1.y - p2.y);
}

float2 add_float2(float2 p1, float2 p2)
{
    return float2_xy(p1.x + p2.x, p1.y + p2.y);
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
            float2 p = float2_xy((float)col, (float)row);
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

typedef struct float3
{
    float x;
    float y;
    float z;
} float3;

float3 float3_xyz(float x, float y, float z)
{
    float3 r = {};
    r.x = x;
    r.y = y;
    r.z = z;
    return r;
}

float3 add_float3(float3 p1, float3 p2)
{
    return float3_xyz(p1.x + p2.x, p1.y + p2.y, p1.z + p2.z);
}

float3 sub_float3(float3 p1, float3 p2)
{
    return float3_xyz(p1.x - p2.x, p1.y - p2.y, p1.z - p2.z);
}

typedef struct float3x3
{
    float v[3][3];
} float3x3;

float3x3 float3x3_v(float v00, float v01, float v02, float v10, float v11, float v12, float v20, float v21, float v22)
{
    float3x3 m = {};
    m.v[0][0] = v00;
    m.v[0][1] = v01;
    m.v[0][2] = v02;
    m.v[1][0] = v10;
    m.v[1][1] = v11;
    m.v[1][2] = v12;
    m.v[2][0] = v20;
    m.v[2][1] = v21;
    m.v[2][2] = v22;
    return m;
}

float3 transform3(float3x3 m, float3 v)
{
    float3 result = {};
    result.x = m.v[0][0] * v.x + m.v[0][1] * v.y + m.v[0][2] * v.z;
    result.y = m.v[1][0] * v.x + m.v[1][1] * v.y + m.v[1][2] * v.z;
    result.z = m.v[2][0] * v.x + m.v[2][1] * v.y + m.v[2][2] * v.z;
    return result;
}

float3 float3_xy_z(float2 xy, float z)
{
    float3 r = {};
    r.x = xy.x;
    r.y = xy.y;
    r.z = z;
    return r;
}

float3x3 GetRotationAroundY(float2 cos_sin)
{
    float c = cos_sin.x;
    float s = cos_sin.y;
    float3x3 tm = float3x3_v(c, 0.0f, s, 0.0f, 1.0f, 0.0f, -s, 0.0f, c);
    return tm;
}

float2 ToXY(float3 v)
{
    return float2_xy(v.x, v.y);
}

void DrawQuad3(Bitmap *bitmap, float3 v1, float3 v2, float3 v3, float3 v4, unsigned int color)
{
    Quad2 quad = {};
    quad.p[0] = ToXY(v1);
    quad.p[1] = ToXY(v2);
    quad.p[2] = ToXY(v3);
    quad.p[3] = ToXY(v4);
    DrawQuad2(bitmap, quad, color);
}

void Update3D(Input *input, Bitmap *bitmap)
{
    FillWithColor(bitmap, (unsigned int)0);
    float min_side = 0.5f * Min2(input->screen_size.x, input->screen_size.y);
    float2 mid = mul_float_float2(0.5f, input->screen_size);
    float3 cube_center = float3_xy_z(mid, 0.0f);
    float cube_side = min_side;
    float2 cos_sin = float2_xy(cosf(input->time), sinf(input->time));
    float3x3 rot_tm = GetRotationAroundY(cos_sin);
    float3 x_side = transform3(rot_tm, float3_xyz(0.5f, 0.0f, 0.0f));
    float3 y_side = transform3(rot_tm, float3_xyz(0.0f, 0.5f, 0.0f));
    float3 z_side = transform3(rot_tm, float3_xyz(0.0f, 0.0f, 0.5f));
    float3 corner_ldf = sub_float3(sub_float3(sub_float3(cube_center, x_side), y_side), z_side);
    float3 corner_ldb = add_float3(sub_float3(sub_float3(cube_center, x_side), y_side), z_side);
    float3 corner_luf = sub_float3(add_float3(sub_float3(cube_center, x_side), y_side), z_side);
    float3 corner_lub = add_float3(add_float3(sub_float3(cube_center, x_side), y_side), z_side);
    float3 corner_rdf = sub_float3(sub_float3(add_float3(cube_center, x_side), y_side), z_side);
    float3 corner_rdb = add_float3(sub_float3(add_float3(cube_center, x_side), y_side), z_side);
    float3 corner_ruf = sub_float3(add_float3(add_float3(cube_center, x_side), y_side), z_side);
    float3 corner_rub = add_float3(add_float3(add_float3(cube_center, x_side), y_side), z_side);
    DrawQuad3(bitmap, corner_luf, corner_ruf, corner_rdf, corner_ldf, (unsigned int)0x00FF00);
    DrawQuad3(bitmap, corner_ruf, corner_rub, corner_rdb, corner_rdf, (unsigned int)0xFFFF00);
    DrawQuad3(bitmap, corner_rub, corner_lub, corner_ldb, corner_rdb, (unsigned int)0x0000FF);
    DrawQuad3(bitmap, corner_lub, corner_luf, corner_ldf, corner_ldb, (unsigned int)0xFF0000);
}

void Update(Input *input, Bitmap *bitmap)
{
    Update3D(input, bitmap);
    return ;
}
