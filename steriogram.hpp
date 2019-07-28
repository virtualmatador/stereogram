#include <array>
#include <vector>
#include <cmath>
#include <thread>

namespace steriogram
{

    int GetColumn(const int width)
    {
        return 4.4 * pow(width, 0.44);
    }

    template<int PIXEL_SIZE>
    std::vector<unsigned char> CreateCurvedPattern(const int column, const int order_rgba)
    {
        int r = (order_rgba & 0xFF000000) >> 24;
        int g = (order_rgba & 0x00FF0000) >> 16;
        int b = (order_rgba & 0x0000FF00) >>  8;
        int a = (order_rgba & 0x000000FF) >>  0;
        std::vector<unsigned char> pattern(column * column * 4);
        for (int y = 0; y < column; ++y)
        {
            for (int x = 0; x < column; ++x)
            {
                if (PIXEL_SIZE > r)
                    pattern[(y * column + x) * PIXEL_SIZE + r] = int(pow(y + column / 3.6, 1.45) * pow(x + column / 3.4, 1.55)) % 256;
                if (PIXEL_SIZE > g)
                    pattern[(y * column + x) * PIXEL_SIZE + g] = int(pow(y + column / 4.8, 1.15) * pow(x + column / 4.6, 1.25)) % 256;
                if (PIXEL_SIZE > b)
                    pattern[(y * column + x) * PIXEL_SIZE + b] = int(pow(y + column / 2.2, 1.85) * pow(x + column / 2.4, 1.75)) % 256;
                if (PIXEL_SIZE > a)
                    pattern[(y * column + x) * PIXEL_SIZE + a] = 255;
            }
        }
        return pattern;
    }

    template<int PIXEL_SIZE>
    std::vector<unsigned char> CreateRandomPattern(const int column, const int order_rgba)
    {
        int r = (order_rgba & 0xFF000000) >> 24;
        int g = (order_rgba & 0x00FF0000) >> 16;
        int b = (order_rgba & 0x0000FF00) >>  8;
        int a = (order_rgba & 0x000000FF) >>  0;
        std::vector<unsigned char> pattern(column * column * 4);
        for (int y = 0; y < column; ++y)
        {
            for (int x = 0; x < column; ++x)
            {
                int color = rand() % (256 * 256 * 256);
                if (PIXEL_SIZE > r)
                    pattern[(y * column + x) * PIXEL_SIZE + r] = ((unsigned char*)&color)[0];
                if (PIXEL_SIZE > g)
                    pattern[(y * column + x) * PIXEL_SIZE + g] = ((unsigned char*)&color)[1];
                if (PIXEL_SIZE > b)
                    pattern[(y * column + x) * PIXEL_SIZE + b] = ((unsigned char*)&color)[2];
                if (PIXEL_SIZE > a)
                    pattern[(y * column + x) * PIXEL_SIZE + a] = 255;
            }
        }
        return pattern;
    }

    template<int PIXEL_SIZE, int Z_LEVELS>
    void Convert(unsigned char* data, const int column, const int width, const int height, const unsigned char* pattern)
    {
        std::vector<std::thread>threads(std::thread::hardware_concurrency());
        int end = 0;
        int progress = (height + threads.size() - 1) / threads.size();
        for (auto & thread : threads)
        {
            int start = end;
            end += progress;
            bool run = end < height;
            if (!run)
                end = height;
            thread = std::thread([&, start, end]()
            {
                std::vector<int> shifts(width);
                for (int y = start; y < end; ++y)
                {
                    std::fill(shifts.begin(), shifts.end(), 0);
                    int side = (y % 2) * 2 - 1;
                    for (int x = width / 2 - side * width / 2 - (1 - side) / 2 + side * column;
                        side * x <= side * (width / 2 + side * width / 2 - (1 + side) / 2 - side * column);
                        x += side)
                    {
                        int z = 0;
                        for (int i = 0; i < PIXEL_SIZE; ++i)
                            z += (data + (y * width + x) * PIXEL_SIZE)[i];
                        z /= PIXEL_SIZE * (256 / Z_LEVELS);
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
                        memcpy(data + (y * width + x) * PIXEL_SIZE,
                            pattern + ((y % column) * column + ((x + shifts[x]) % column)) * PIXEL_SIZE,
                            PIXEL_SIZE);
                }
            });
            if (!run)
                break;
        }
        for (auto & thread : threads)
            thread.join();
    }

}
