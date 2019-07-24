#include <array>
#include <vector>
#include <cmath>
#include <thread>

namespace steriogram
{

    template<int PIXEL_SIZE>
    void WritePattern(int column, unsigned char* dest, void* user_data)
    {
        for (int y = 0; y < column; ++y)
        {
            for (int x = 0; x < column; ++x)
            {
                if (PIXEL_SIZE > 0)
                    dest[(y * column + x) * PIXEL_SIZE + 0] = int(pow(y + column / 3.6, 1.45) * pow(x + column / 3.4, 1.55)) % 256;
                if (PIXEL_SIZE > 1)
                    dest[(y * column + x) * PIXEL_SIZE + 1] = int(pow(y + column / 4.8, 1.15) * pow(x + column / 4.6, 1.25)) % 256;
                if (PIXEL_SIZE > 2)
                    dest[(y * column + x) * PIXEL_SIZE + 2] = int(pow(y + column / 2.2, 1.85) * pow(x + column / 2.4, 1.75)) % 256;
                if (PIXEL_SIZE > 3)
                    dest[(y * column + x) * PIXEL_SIZE + 3] = 255;
            }
        }
    }

    template<int PIXEL_SIZE, int Z_LEVELS>
    void Convert(unsigned char* data, int width, int height,
        void (*WritePattern)(int column, unsigned char* dest, void* user_data) = nullptr,
        void* user_data = nullptr, int column = 0, int x = 0, int y = 0, int stride = 0)
    {
        if (WritePattern == nullptr)
            WritePattern = steriogram::WritePattern<PIXEL_SIZE>;
        if (stride == 0)
            stride = width * PIXEL_SIZE;
        if (column == 0)
            column = 4.5 * pow(width, 0.4);
        std::vector<unsigned char> pattern(column * column);
        WritePattern(column, pattern.data(), user_data);
        int size = std::min(3, PIXEL_SIZE);
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
                        for (int i = 0; i < size; ++i)
                            z += (data + y * stride + x * PIXEL_SIZE)[i];
                        z /= size * (256 / Z_LEVELS);
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
                        memcpy(data + y * stride + x * PIXEL_SIZE,
                            pattern.data() + ((y % column) * column + ((x + shifts[x]) % column)) * PIXEL_SIZE,
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