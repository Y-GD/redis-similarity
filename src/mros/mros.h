#ifndef _MROS_H
#define _MROS_H

#include <stdint.h>
#include <stdlib.h>

typedef struct _MROS_
{
    size_t size;           // number of items in the multi-resolution odd sketch
    uint32_t layer_cnt;     // number of layers in the multi-resolution sketch
    uint64_t mod;          // the mod of hash value
    unsigned char *bitmap; // bitmap

    uint64_t bytes; // the memory used of the multi-resolution sketch(bytes)
} mros;

/**
 * Initialize the multi-resolution odd sketch for use.
 *
 * optional numebr of layer_cnt:
 *     layer_cnt=ln(n/m)+2
 *     m is the number of bits of each layer,default is 4096 bits
 *
 * Parameters:
 * -----------
 *     mros   - Pointer to an allocated struct multi-resolution odd sketch.
 *     entries - The expected number of entries which will be inserted.
 *               Must be at least 1000 (in practice, likely much larger).
 *
 *Return:
 * -------
 *     0 - on success
 *     1 - on failure
 */
int mros_init(mros *mros, uint64_t entries);

/**
 * Insert an item into the multi-resolution odd sketch.
 *
 * Parameters:
 *      mros  - Pointer to an allocated struct multi-resolution odd skecth.
 *      buffer - Pointer to buffer containing element to add.
 *      len    - Size of 'buffer'.
 *
 * Return:
 *     0 - successful insertion
 *    -1 - mros not initialized
 */

int mros_add_h(mros *mros, uint64_t hashVal);
int mros_add(mros *mros, const void *buffer, int len);

/**
 * estimate the jaccard simialarity of the two sketches of two sets
 *
 * Parameters:
 *      m1,m2  - Pointers to the allocated struct multi-resolution odd skecthes of each set.
 *
 * Return value:
 *     value>=0 :the jaccard similarity of two sets
 *     value<0 : error, the number of layer_cnt of m1 and m2 must be equal
 */
double mros_jaccard_calculate(mros *m1, mros *m2);

/**
 * Print (to stdout) info about this bloom filter. Debugging aid.
 */
void mros_print(mros *mros);

/**
 * Deallocate internal storage.
 *
 * Upon return, the mros struct is no longer usable. You may call mros_init
 * again on the same struct to reinitialize it again.
 *
 * Parameters:
 * -----------
 *     mros  - Pointer to an allocated struct multi-resolution odd skecth
 *
 * Return: none
 *
 */
void mros_free(mros *mros);
#endif