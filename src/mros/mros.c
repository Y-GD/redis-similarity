#include "mros.h"
#include "murmurhash2.h"
#include <math.h>
#include <stdio.h>

static const double beta = 0.32;             // threshold of the number of 1-bits
static const uint32_t layer_bits = 4096 * 8; // the number of 1-bits of each layer, layer_bits must be the multiple of 8

extern void (*RedisModule_Free)(void *ptr);
extern void *(*RedisModule_Calloc)(size_t nmemb, size_t size);

#define CALLOC RedisModule_Calloc
#define FREE RedisModule_Free

// #define CALLOC calloc
// #define FREE free

#define MIN(a, b) (((a) < (b)) ? (a) : (b))

int mros_init(mros *mros, uint64_t entries)
{
    // entry/2^(l-1)=beta*2*layer_bits，保证最后一层不溢出
    if (entries < layer_bits)
    {
        mros->layer_cnt = 2;
    }
    else
    {
        mros->layer_cnt = ceil(log2(1.0 * entries / (layer_bits * beta)));
    }

    // layer_bits * 2^l需要小于2^63,防止mod溢出
    if (mros->layer_cnt + log2(layer_bits) > 63)
    {
        printf("mod overflow\n");
        return 1;
    }

    mros->mod = (1ULL << mros->layer_cnt) * layer_bits;

    mros->bytes = (layer_bits / 8) * (mros->layer_cnt + 1);

    mros->bitmap = (unsigned char *)CALLOC(mros->bytes, sizeof(unsigned char));

    mros->size = 0;

    if (mros->bitmap == NULL)
    {
        printf("memory allocation failure for bitmap\n");
        return 1;
    }

    return 0;
}

static uint64_t mros_calc_hash64(const void *buffer, int len)
{
    return MurmurHash64B(buffer, len, 0xc6a4a7935bd1e995ULL);
}

int mros_add_h(mros *mros, uint64_t hashVal)
{
    uint64_t mod_value = hashVal % mros->mod;

    double idx = log2(mros->mod * 1.0 / (mros->mod - mod_value));

    uint32_t q = MIN(mros->layer_cnt, (uint32_t)(1 + floor(idx)));

    uint64_t bit_index = layer_bits * (q + 1 - pow(2, q)) + floor(mod_value * 1.0 / pow(2, mros->layer_cnt - q));

    mros->bitmap[bit_index / 8] ^= ((unsigned char)1 << (bit_index % 8));

    mros->size++;

    return 0;
}

int mros_add(mros *mros, const void *buffer, int len)
{
    if (mros->bitmap == NULL)
        return -1;
    return mros_add_h(mros, mros_calc_hash64(buffer, len));
}

static uint32_t one_bits_num(const unsigned char *bitmap, uint8_t cur_layer_cnt, uint32_t cur_layer_bits)
{
    uint32_t cnt = 0;
    uint32_t skip = layer_bits / 8;

    const unsigned char *start = bitmap + (cur_layer_cnt - 1) * skip;

    for (uint32_t i = 0; i < cur_layer_bits / 8; i++)
    {
        cnt += __builtin_popcount(*(start + i));
    }

    return cnt;
}

static double sizeEstimate(unsigned char *bitmap, uint32_t layer_cnt)
{
    uint32_t base_layer = 0;

    for (base_layer = layer_cnt; base_layer - 1 > 0; base_layer--)
    {
        if (one_bits_num(bitmap, base_layer - 1, layer_bits) > layer_bits * beta)
        {
            break;
        }
    }

    double sum = 0.0;
    // printf("base_layer:%u\n", base_layer);

    for (size_t i = base_layer; i < layer_cnt; i++)
    {
        // sum += -(layer_bits / 2.0) * log(1 - 2.0 * one_bits_num(bitmap, i, layer_bits) / layer_bits);
        sum += log(1 - 2.0 * one_bits_num(bitmap, i, layer_bits) / layer_bits) / log(1.0 - 1.0 * 2 / layer_bits);
    }

    // sum += -(layer_bits * 1.0) * log(1.0 - 1.0 * one_bits_num(bitmap, layer_cnt, 2 * layer_bits) / layer_bits);
    sum += log(1 - 1.0 * one_bits_num(bitmap, layer_cnt, 2 * layer_bits) / layer_bits) / log(1.0 - 1.0 / layer_bits);

    return round(pow(2.0, base_layer - 1) * sum);
}

static unsigned char *xorMerge(unsigned char *s1, unsigned char *s2, uint32_t len)
{
    unsigned char *ret = (unsigned char *)CALLOC(len, sizeof(unsigned char));

    for (uint32_t i = 0; i < len; i++)
    {
        ret[i] = s1[i] ^ s2[i];
    }

    return ret;
}

double mros_jaccard_calculate(mros *m1, mros *m2)
{
    if (m1->layer_cnt != m2->layer_cnt)
    {
        return -1;
    }
    uint32_t len = (m1->layer_cnt + 1) * (layer_bits / 8);
    unsigned char *merge = xorMerge(m1->bitmap, m2->bitmap, len);

    double symmetric_diffrenece = sizeEstimate(merge, m1->layer_cnt);
    // printf("symmetric_diffrenece:%lf\n", symmetric_diffrenece);

    FREE(merge);

    double tmp = (m1->size + m2->size - symmetric_diffrenece) / (m1->size + m2->size + symmetric_diffrenece);

    return tmp > 0 ? tmp : 0;
}

void mros_print(mros *mros)
{
    printf("memory usage(bytes): %llu\n", mros->bytes);
    printf("layer_cnt: %u\n", mros->layer_cnt);
    printf("layer_bits: %u\n", layer_bits);
    printf("mod: %llu\n", mros->mod);
    printf("size: %u\n", mros->size);
}

void mros_free(mros *mros)
{
    FREE(mros->bitmap);
}
