#include <array>
#include <cmath>
#include <cstring>
#include <functional>
#include <random>
#include <thread>
#include <vector>

namespace stereogram
{

    void RunParallel(int height, std::function<void(int begin, int end)> task)
    {
        std::vector<std::thread>threads(std::thread::hardware_concurrency());
        int progress = int((height + threads.size() - 1) / threads.size());
        for (auto & thread : threads)
        {
            thread = std::thread([&](int index)
            {
                task(index * progress, std::min((index + 1) * progress, height));
            }, std::distance(threads.data(), &thread));
        }
        for (auto & thread : threads)
            thread.join();
    }

    int GetColumn(const int width)
    {
        return 4.4 * pow(width, 0.44);
    }

    template<int PIXEL_SIZE>
    std::vector<unsigned char> CreatePattern(const int column, const int pixel_format)
    {
        const int r = (pixel_format & 0xFF000000) >> 24;
        const int g = (pixel_format & 0x00FF0000) >> 16;
        const int b = (pixel_format & 0x0000FF00) >>  8;
        const int a = (pixel_format & 0x000000FF) >>  0;
        std::vector<unsigned char> pattern(column * column * PIXEL_SIZE);
        RunParallel(column, [&](int begin, int end)
        {
            std::random_device rd;
            std::default_random_engine generator(rd());
            std::uniform_int_distribution<int> distribution(0, 256 * 256 * 256 - 1);
            for (int y = begin; y < end; ++y)
            {
                for (int x = 0; x < column; ++x)
                {
                    int color = distribution(generator);
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
        });
        return pattern;
    }

    void apply_pixel(int* shifts, const int x, const int z, const int side, const int column, const int width)
    {
        for (int k = x + side * column / 2 - side * (z + z % 2 * (side + 1) / 2) / 2;
            side * k <= side * (width / 2 * (1 + side) - (1 + side) / 2); k += side * column)
        {
            int target = k + side * z;
            if (target < 0)
                target += column;
            else if (target >= width)
                target -= column;
            shifts[k] = shifts[target] + side * z;
            if (shifts[k] < 0)
                shifts[k] += column;
            else if (shifts[k] >= column)
                shifts[k] -= column;
        }
    }

    template<int PIXEL_SIZE, int Z_LEVELS, int FLOOR_Z>
    void Convert(unsigned char* data, const int column, const int width, const int height, const unsigned char* pattern)
    {
        RunParallel(height, [&](int begin, int end)
        {
            std::vector<int> shifts(width);
            for (int y = begin; y < end; ++y)
            {
                std::fill(shifts.begin(), shifts.end(), 0);
                int side = (y % 2) * 2 - 1;
                for (int x = width / 2 * (1 - side) - (1 - side) / 2;
                    side * x < side * (width / 2 * (1 - side) - (1 - side) / 2 + side * column);
                    x += side)
                {
                    apply_pixel(shifts.data(), x, FLOOR_Z, side, column, width);
                }
                for (int x = width / 2 * (1 - side) - (1 - side) / 2 + side * column;
                    side * x <= side * (width / 2 * (1 + side) - (1 + side) / 2 - side * column);
                    x += side)
                {
                    int z = 0;
                    for (int i = 0; i < 3; ++i)
                        z += (data + (y * width + x) * PIXEL_SIZE)[i];
                    z /= 3 * (256.0 / Z_LEVELS);
                    apply_pixel(shifts.data(), x, z, side, column, width);
                }
                for (int x = width / 2 * (1 + side) - (1 + side) / 2 - side * column;
                    side * x < side * (width / 2 * (1 + side));
                    x += side)
                {
                    apply_pixel(shifts.data(), x, FLOOR_Z, side, column, width);
                }
                for (int x = 0; x < width; ++x)
                {
                    std::memcpy(data + (y * width + x) * PIXEL_SIZE,
                        pattern + ((y % column) * column + ((x + shifts[x]) % column)) * PIXEL_SIZE,
                        PIXEL_SIZE);
                }
            }
        });
    }

}
