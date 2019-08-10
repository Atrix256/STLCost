// the size of the image that is made into mips
#define IMAGE_SIZE() 512

typedef float ChannelType;

#include <stdio.h>
#include <array>
#include <vector>
#include <chrono>

struct ScopedTimer
{
    ScopedTimer(const char* label)
    {
        printf("%s: ", label);
        m_start = std::chrono::high_resolution_clock::now();
    }

    ~ScopedTimer()
    {
        std::chrono::high_resolution_clock::time_point end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> time_span = std::chrono::duration_cast<std::chrono::duration<double>>(end - m_start);
        printf("%f ms\n", time_span.count() * 1000.0f);
    }

    std::chrono::high_resolution_clock::time_point m_start;
};

// calculate the total number of pixels needed to hold the image of size IMAGE_SIZE() as well as all the mips
constexpr size_t TotalPixelsMipped()
{
    size_t ret = 0;
    size_t size = IMAGE_SIZE();
    while (size)
    {
        ret += size * size;
        size /= 2;
    }
    return ret;
}

constexpr size_t TotalChannelsMipped()
{
    // RGBA
    return TotalPixelsMipped() * 4;
}

constexpr size_t NumMips()
{
    size_t ret = 0;
    size_t size = IMAGE_SIZE();
    while (size)
    {
        ret++;
        size /= 2;
    }
    return ret;
}

void GetMipInfo(size_t desiredMipIndex, size_t& offset, size_t& width)
{
    offset = 0;
    width = IMAGE_SIZE();

    for (size_t mipIndex = 0; mipIndex < desiredMipIndex; ++mipIndex)
    {
        offset += width * width * 4;
        width /= 2;
    }
}

template<typename T>
void MakeMip(T& image, size_t mipIndex)
{
    size_t srcOffset;
    size_t srcWidth;
    GetMipInfo(mipIndex - 1, srcOffset, srcWidth);

    size_t destOffset;
    size_t destWidth;
    GetMipInfo(mipIndex, destOffset, destWidth);

    for (size_t destY = 0; destY < destWidth; ++destY)
    {
        for (size_t destX = 0; destX < destWidth; ++destX)
        {
            for (size_t channel = 0; channel < 4; ++channel)
            {
                // 2x2 box filter source mip pixels
                float value =
                    float(image[((destY * 2 + 0) * srcWidth + destX * 2 + 0) * 4 + srcOffset] + channel) +
                    float(image[((destY * 2 + 0) * srcWidth + destX * 2 + 1) * 4 + srcOffset] + channel) +
                    float(image[((destY * 2 + 1) * srcWidth + destX * 2 + 0) * 4 + srcOffset] + channel) +
                    float(image[((destY * 2 + 1) * srcWidth + destX * 2 + 1) * 4 + srcOffset] + channel);
                image[destOffset] = ChannelType(value / 4.0f);
                destOffset++;
            }
        }
    }
}

template<typename T>
void MakeMips(T& image)
{
    size_t mipCount = NumMips();
    for (size_t mipIndex = 1; mipIndex < mipCount; ++mipIndex)
        MakeMip(image, mipIndex);
}

template<typename T>
void InitImage(T& image)
{
    memset(&image[0], 0, TotalChannelsMipped() * sizeof(float));

    // It doesn't matter what we put into the image since we aren't ever looking at it, but initializing it anyhow.
    size_t i = 0;
    for (size_t y = 0; y < IMAGE_SIZE(); ++y)
    {
        for (size_t x = 0; x < IMAGE_SIZE(); ++x)
        {
            image[i * 4 + 0] = ChannelType(x % 256);
            image[i * 4 + 1] = ChannelType(y % 256);
            image[i * 4 + 2] = ChannelType(0);
            image[i * 4 + 3] = ChannelType(255);
            i++;
        }
    }
}

int main(void)
{
    // std::array
    // dynamically allocated to avoid a stack overflow
    {
        std::array<ChannelType, TotalChannelsMipped()>* array_ptr = new std::array<ChannelType, TotalChannelsMipped()>;
        std::array<ChannelType, TotalChannelsMipped()>& array = *array_ptr;
        printf("std::array:\n");
        {
            ScopedTimer timer("InitImage");
            InitImage(array);
        }
        {
            ScopedTimer timer("MakeMips");
            MakeMips(array);
        }
        delete array_ptr;
    }

    // std::vector
    {
        std::vector<ChannelType> vector;
        printf("\nstd::vector:\n");
        {
            ScopedTimer timer("InitImage");
            vector.resize(TotalChannelsMipped());
            InitImage(vector);
        }
        {
            ScopedTimer timer("MakeMips");
            MakeMips(vector);
        }
    }

    // c array
    {
        ChannelType* carray = nullptr;
        printf("\nc array:\n");
        {
            ScopedTimer timer("InitImage");
            carray = (ChannelType*)malloc(sizeof(ChannelType)*TotalChannelsMipped());
            InitImage(carray);
        }
        {
            ScopedTimer timer("MakeMips");
            MakeMips(carray);
        }
        free(carray);
    }

    system("pause");
    return 0;
}