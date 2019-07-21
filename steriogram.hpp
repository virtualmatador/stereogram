#include <array>
#include <vector>
#include <cmath>


template<int PIXEL_SIZE, int Z_LEVELS>
int GetHeight(unsigned char* pixel, void* user_data)
{
    int z = 0;
    int size = std::min(3, PIXEL_SIZE);
    for (int i = 0; i < size; ++i)
        z += pixel[i];
    z /= size * (256 / Z_LEVELS);
    return z;
}

template<int PIXEL_SIZE>
void WritePixel(int shift, int x, int y, int column, unsigned char* pixel, void* user_data)
{
    double yy = y % column;
    double xx = (x + shift) % column;
    if (PIXEL_SIZE > 0)
        pixel[0] = int(pow(yy + column / 3.6, 1.45) * pow(xx + column / 3.4, 1.55)) % 256;
    if (PIXEL_SIZE > 1)
        pixel[1] = int(pow(yy + column / 4.8, 1.15) * pow(xx + column / 4.6, 1.25)) % 256;
    if (PIXEL_SIZE > 2)
        pixel[2] = int(pow(yy + column / 2.2, 1.85) * pow(xx + column / 2.4, 1.75)) % 256;
    if (PIXEL_SIZE > 3)
        pixel[3] = 255;
}

template<int PIXEL_SIZE>
void Steriogram(unsigned char* data, int width, int height,
    int (*GetHeight)(unsigned char* pixel, void* user_data) = nullptr,
    void (*WritePixel)(int shift, int x, int y, int column, unsigned char* pixel, void* user_data) = nullptr,
    void* user_data = nullptr, int column = 0, int x = 0, int y = 0, int stride = 0)
{
    if (GetHeight == nullptr)
        GetHeight = ::GetHeight<PIXEL_SIZE, 16>;
    if (WritePixel == nullptr)
        WritePixel = ::WritePixel<PIXEL_SIZE>;
    if (stride == 0)
        stride = width * PIXEL_SIZE;
    if (column == 0)
    	column = 4.5 * pow(width, 0.4);
	std::vector<int> shifts(width);
	for (int y = 0; y < height; ++y)
	{
        std::fill(shifts.begin(), shifts.end(), 0);
		int side = (y % 2) * 2 - 1;
		for (int x = width / 2 - side * width / 2 - (1 - side) / 2 + side * column;
			side * x <= side * (width / 2 + side * width / 2 - (1 + side) / 2 - side * column);
			x += side)
		{
            int z = GetHeight(data + y * stride + x * PIXEL_SIZE, user_data);
			if (z != 0)
			{
				for (int k = x + side * column / 2 - side * (z + z % 2 * (side + 1) / 2) / 2;
					side * k <= side * (width / 2 + side * width / 2 - (1 + side) / 2); k += side * column)
				{
					shifts[k] = side * z;
					int target = k + side * z;
					if (target < 0)
						target += column;
					else if (target >= width)
						target -= column;
					shifts[k] += shifts[target];
					if (shifts[k] < 0)
						shifts[k] += column;
					else if (shifts[k] >= column)
						shifts[k] -= column;
				}
			}
		}
		for (int x = 0; x < width; ++x)
            WritePixel(shifts[x], x, y, column, data + y * stride + x * PIXEL_SIZE, user_data);
	}
}
