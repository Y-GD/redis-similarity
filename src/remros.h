#pragma once

#include "mros/mros.h"

char *MROS_GetEncodedHeader(const mros *m, size_t *hdrlen);

void MROS_FreeEncodedHeader(char *hdr);

const char *MROS_GetEncodedChunk(const mros *m, long long *curIter, size_t *len,
                                 size_t maxChunkSize);

#define MROS_CHUNKITER_INIT 1
#define MROS_CHUNKITER_DONE 0

mros *MROS_NewFromHeader(const char *buf, size_t bufLen, const char **errmsg);

int MROS_LoadEncodedChunk(mros *m, long long iter, const char *buf, size_t bufLen,
                          const char **errmsg);
