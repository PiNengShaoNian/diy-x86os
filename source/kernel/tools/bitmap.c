#include "tools/bitmap.h"
#include "tools/klib.h"

int bitmap_byte_count(int count)
{
    return (count + 7) / 8;
}

void bitmap_init(bitmap_t *bitmap, uint8_t *bits, int count, int init_bit)
{
    bitmap->bit_count = count;
    bitmap->bits = bits;

    int bytes = bitmap_byte_count(bitmap->bit_count);
    kernel_memset(bits, init_bit ? 0xFF : 0, bytes);
}

int bitmap_get_bit(bitmap_t *bitmap, int index)
{
    return (bitmap->bits[index / 8] & (1 << (index % 8))) != 0;
}

void bitmap_set_bit(bitmap_t *bitmap, int index, int count, int bit)
{
    for (int i = 0; i < count && (index < bitmap->bit_count); i++, index++)
    {
        if (bit)
            bitmap->bits[index / 8] |= (1 << (index % 8));
        else
            bitmap->bits[index / 8] &= ~(1 << (index % 8));
    }
}

int bitmap_is_set(bitmap_t *bitmap, int index)
{
    return bitmap_get_bit(bitmap, index) ? 1 : 0;
}

int bitmap_alloc_nbits(bitmap_t *bitmap, int bit, int count)
{

    int l = 0;
    int r = -1;
    int n = bitmap->bit_count;
    while (l < n)
    {
        if (r + 1 < n && bitmap_get_bit(bitmap, ++r))
            l = r + 1;

        if (r - l + 1 >= count)
        {
            bitmap_set_bit(bitmap, l, count, ~bit);
            return l;
        }
    }

    return -1;
}