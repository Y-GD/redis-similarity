#include <stdio.h>
#include "mros.h"
#include "redismodule.h"
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <assert.h>
#include "remros.h"

#define MROS_MODULE_VERSION 1

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
/// Redis Commands                                                           ///
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
static RedisModuleType *MROSType;
static uint64_t default_capacity = 10000000ULL;
static int rsStrcasecmp(const RedisModuleString *rs1, const char *s2);

typedef enum
{
    MROS_OK = 0,
    MROS_MISSING,
    MROS_EMPTY,
    MROS_MISMATCH
} lookupStatus;

static const char *statusStrerror(int status)
{
    switch (status)
    {
    case MROS_MISSING:
    case MROS_EMPTY:
        return "ERR not found";
    case MROS_MISMATCH:
        return REDISMODULE_ERRORMSG_WRONGTYPE;
    case MROS_OK:
        return "ERR item exists";
    default: // LCOV_EXCL_LINE
        return "Unknown error";
    }
}

typedef struct
{
    uint64_t capacity;
    int autocreate;
    int is_multi;
} MROSInsertOptions;

//==========================Helper functions=================================
/**
 * 比较两个字符串是否一致，忽略大小写
 */
static int rsStrcasecmp(const RedisModuleString *rs1, const char *s2)
{
    size_t n1 = strlen(s2);
    size_t n2;
    const char *s1 = RedisModule_StringPtrLen(rs1, &n2);
    if (n1 != n2)
    {
        return -1;
    }
    return strncasecmp(s1, s2, n1);
}

static inline bool _is_resp3(RedisModuleCtx *ctx)
{
    int ctxFlags = RedisModule_GetContextFlags(ctx);
    if (ctxFlags & REDISMODULE_CTX_FLAGS_RESP3)
    {
        return true;
    }
    return false;
}

#define _ReplyMap(ctx) (RedisModule_ReplyWithMap != NULL && _is_resp3(ctx))

static void RedisModule_ReplyWithMapOrArray(RedisModuleCtx *ctx, long len, bool half)
{
    if (_ReplyMap(ctx))
    {
        RedisModule_ReplyWithMap(ctx, half ? len / 2 : len);
    }
    else
    {
        RedisModule_ReplyWithArray(ctx, len);
    }
}
#define BAIL(s)                             \
    do                                      \
    {                                       \
        RedisModule_Log(ctx, "warning", s); \
        return REDISMODULE_ERR;             \
    } while (0)

static int isMulti(const RedisModuleString *rs)
{
    size_t n;
    const char *s = RedisModule_StringPtrLen(rs, &n);
    return s[5] == 'm' || s[5] == 'M';
}

//============================================================================
/**
 * 查看对应key的状态
 */
static int getValue(RedisModuleKey *key, RedisModuleType *expType, void **sbout)
{
    *sbout = NULL;
    if (key == NULL)
    {
        return MROS_MISSING;
    }
    int type = RedisModule_KeyType(key);
    if (type == REDISMODULE_KEYTYPE_EMPTY)
    {
        return MROS_EMPTY;
    }
    else if (type == REDISMODULE_KEYTYPE_MODULE &&
             RedisModule_ModuleTypeGetType(key) == expType)
    {
        *sbout = RedisModule_ModuleTypeGetValue(key);
        return MROS_OK;
    }
    else
    {
        return MROS_MISMATCH;
    }
}

static int mrosGetKeyStatus(RedisModuleKey *key, mros **m)
{
    return getValue(key, MROSType, (void **)m);
}

//========================Insert、Add function======================
/**
 * common function for creating a multi-resoluation odd sketch structure
 */
static mros *mrosCreate(RedisModuleKey *key, uint64_t entry)
{
    mros *m = (mros *)RedisModule_Calloc(1, sizeof(*m));
    if (m != NULL && mros_init(m, entry) == 0)
    {
        printf("m!=NULL:%d\n", m != NULL);

        RedisModule_ModuleTypeSetValue(key, MROSType, m);
        return m;
    }
    else
    {
        RedisModule_Free(m);
        return NULL;
    }
}

/**
 * common function for adding one or more elements to a multi-resoluation odd skecth structure
 */
static int MROSInsertCommon(RedisModuleCtx *ctx, RedisModuleString *keyStr, RedisModuleString **items, size_t nitems, const MROSInsertOptions *optinos)
{
    RedisModuleKey *key = RedisModule_OpenKey(ctx, keyStr, REDISMODULE_READ | REDISMODULE_WRITE);
    mros *m;
    const int status = mrosGetKeyStatus(key, &m);
    // 没有对应的key，创建一个mros
    if (status == MROS_EMPTY && optinos->autocreate)
    {
        m = mrosCreate(key, optinos->capacity);
        if (m == NULL)
        {
            return RedisModule_ReplyWithError(ctx, "ERR couldn't create mros structure");
        }
    }
    else if (status != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, statusStrerror(status));
    }

    // 插入多个元素
    if (optinos->is_multi)
    {
        RedisModule_ReplyWithArray(ctx, REDISMODULE_POSTPONED_LEN);
    }

    size_t arr_len = 0;
    int rv = 0;

    for (size_t ii = 0; ii < nitems && rv != -1; ii++)
    {
        size_t n;
        const char *s = RedisModule_StringPtrLen(items[ii], &n);
        rv = mros_add(m, s, n);
        if (rv == -1)
        {
            RedisModule_ReplyWithError(ctx, "ERR mros is not initialized");
        }
        else
        {
            if (_is_resp3(ctx))
            {
                RedisModule_ReplyWithBool(ctx, rv);
            }
            else
            {
                RedisModule_ReplyWithLongLong(ctx, rv);
            }
        }

        arr_len++;
    }

    if (optinos->is_multi)
    {
        RedisModule_ReplySetArrayLength(ctx, arr_len);
    }

    RedisModule_ReplicateVerbatim(ctx);

    return REDISMODULE_OK;
}
/**
 * mros.insert {filter_key} [CAPACITY {cap} ITEMS {item} {item}]
 *
 * -> (Array) (or error)
 */
static int MROSInsert_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);
    MROSInsertOptions options = {
        .autocreate = default_capacity,
        .is_multi = 1,
        .autocreate = 1};

    int item_index = -1;
    if (argc < 4)
    {
        return RedisModule_WrongArity(ctx);
    }

    size_t cur_pos = 2;
    while (cur_pos < argc && item_index < 0)
    {
        size_t arglen;
        const char *arg = RedisModule_StringPtrLen(argv[cur_pos], &arglen);

        switch (tolower(*arg))
        {
        case 'i':
            item_index = ++cur_pos;
            break;
        case 'c':
            if (++cur_pos == argc)
            {
                return RedisModule_WrongArity(ctx);
            }
            if (RedisModule_StringToULongLong(argv[cur_pos++], &options.capacity) != REDISMODULE_OK)
            {
                return RedisModule_ReplyWithError(ctx, "bad capacity");
            }
            break;
        default:
            return RedisModule_ReplyWithError(ctx, "Unknown argument received");
        }
    }

    if (item_index < 0 || item_index >= argc)
    {
        return RedisModule_WrongArity(ctx);
    }

    return MROSInsertCommon(ctx, argv[1], argv + item_index, argc - item_index, &options);
}

/**
 * Adds items to an existing MROS structure. creates a new one on demand if it doesn't exist.
 *
 * MROS.ADD <KEY> ITEMS....
 *
 * returns an array, the nth element of the array is either 1 indicating the item was added successfully, or a error string indicating an error occurred
 */
static int MROSAdd_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    MROSInsertOptions options = {
        .autocreate = 1,
        .capacity = default_capacity};
    options.is_multi = isMulti(argv[0]);

    if ((options.is_multi && argc < 3) || (!options.is_multi && argc != 3))
    {
        return RedisModule_WrongArity(ctx);
    }

    return MROSInsertCommon(ctx, argv[1], argv + 2, argc - 2, &options);
}
//=======================Reverse Mros Structure===========================

/**
 * reserves a new empty mros structure with custom parameters
 *
 * mros.reserve <KEY> <MAX_CAPACITY (uint)>
 */
static int MROSReserve_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);

    if (argc != 3)
    {
        return RedisModule_WrongArity(ctx);
    }

    uint64_t max_capacity = 0ULL;
    if (RedisModule_StringToULongLong(argv[2], &max_capacity) != REDISMODULE_OK)
    {
        return RedisModule_ReplyWithError(ctx, "Invalid capacity specified");
    }

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    mros *m;
    const int status = mrosGetKeyStatus(key, &m);
    if (status != MROS_EMPTY)
    {
        return RedisModule_ReplyWithError(ctx, statusStrerror(status));
    }

    if (mrosCreate(key, max_capacity) == NULL)
    {
        RedisModule_ReplyWithError(ctx, "could not create mros structure");
    }
    else
    {
        RedisModule_ReplyWithSimpleString(ctx, "OK");
    }

    RedisModule_ReplicateVerbatim(ctx);
    return REDISMODULE_OK;
}

//=======================MROS Infor Information===========================

/**
 * mros.info
 */
static int MROSInfo_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);

    if (argc != 2)
    {
        return RedisModule_WrongArity(ctx);
    }

    mros *m;
    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    const int status = mrosGetKeyStatus(key, &m);
    if (status != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, statusStrerror(status));
    }

    RedisModule_ReplyWithMapOrArray(ctx, 5 * 2, true);
    RedisModule_ReplyWithSimpleString(ctx, "Capacity");
    RedisModule_ReplyWithLongLong(ctx, m->mod);
    RedisModule_ReplyWithSimpleString(ctx, "Size");
    RedisModule_ReplyWithLongLong(ctx, m->size);
    RedisModule_ReplyWithSimpleString(ctx, "layer_cnt");
    RedisModule_ReplyWithLongLong(ctx, m->layer_cnt);
    RedisModule_ReplyWithSimpleString(ctx, "layer_size(bytes)");
    RedisModule_ReplyWithLongLong(ctx, m->bytes / (m->layer_cnt + 1));
    RedisModule_ReplyWithSimpleString(ctx, "total_memory_size(bytes)");
    RedisModule_ReplyWithLongLong(ctx, m->bytes);

    return REDISMODULE_OK;
}

//=======================estimate Jaccard Similairty======================

/**
 *
 * estimate Jaccard Similairty between two sets
 * mros.jaccard <Key1> <Key2>
 */
static int MROSJaccard_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);

    if (argc != 3)
    {
        return RedisModule_WrongArity(ctx);
    }

    RedisModuleKey *key1 = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    RedisModuleKey *key2 = RedisModule_OpenKey(ctx, argv[2], REDISMODULE_READ);

    mros *m1;
    mros *m2;

    const int status1 = mrosGetKeyStatus(key1, &m1);
    const int status2 = mrosGetKeyStatus(key2, &m2);

    if (status1 != MROS_OK || status2 != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, "Invalid key status");
    }

    double value = mros_jaccard_calculate(m1, m2);

    if (value < 0)
    {
        return RedisModule_ReplyWithError(ctx, "the layer_cnt of 2 mros structures must be equal");
    }
    else
    {
        return RedisModule_ReplyWithDouble(ctx, value);
    }
}

/**
 *
 * get bitmap of the given key
 * mros.getbitmap <Key>
 */

static int MROSGetBitMap_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);
    if (argc != 2)
    {
        return RedisModule_WrongArity(ctx);
    }

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    mros *m;
    const int status = mrosGetKeyStatus(key, &m);
    if (status != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, "Invalid key status");
    }

    RedisModule_ReplyWithMapOrArray(ctx, 4 * 2, true);
    RedisModule_ReplyWithSimpleString(ctx, "bitmap");
    RedisModule_ReplyWithStringBuffer(ctx, (char *)m->bitmap, m->bytes);
    // RedisModule_ReplyWithVerbatimString(ctx, m->bitmap, m->bytes);
    // RedisModule_ReplyWithCString(ctx, m->bitmap);

    RedisModule_ReplyWithSimpleString(ctx, "Size");
    RedisModule_ReplyWithLongLong(ctx, m->size);

    RedisModule_ReplyWithSimpleString(ctx, "layer_cnt");
    RedisModule_ReplyWithLongLong(ctx, m->layer_cnt);

    RedisModule_ReplyWithSimpleString(ctx, "layer_size(bytes)");
    RedisModule_ReplyWithLongLong(ctx, m->bytes / (m->layer_cnt + 1));

    return REDISMODULE_OK;
}

//============================AOF Function=============================
// #define MAX_SCANDUMP_SIZE (1024 * 1024 * 16)
#define MAX_SCANDUMP_SIZE (1024 * 16)
/**
 * MROS.SCANDUMP <KEY> <ITER>
 *
 * returns an (iterator,data) pair which can be used for LOADCHUNK later on
 */
static int MROSScanDump_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);
    if (argc != 3)
    {
        return RedisModule_WrongArity(ctx);
    }

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ);
    mros *m;
    const int status = mrosGetKeyStatus(key, &m);
    if (status != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, statusStrerror(status));
    }

    long long iter;
    if (RedisModule_StringToLongLong(argv[2], &iter) != REDISMODULE_OK)
    {
        return RedisModule_ReplyWithError(ctx, "second argument must be numberic");
    }

    RedisModule_ReplyWithArray(ctx, 2);
    if (iter == 0)
    {
        size_t hdr_len;
        char *hdr = MROS_GetEncodedHeader(m, &hdr_len);
        RedisModule_ReplyWithLongLong(ctx, MROS_CHUNKITER_INIT);
        RedisModule_ReplyWithStringBuffer(ctx, (const char *)hdr, hdr_len);
        MROS_FreeEncodedHeader(hdr);
    }
    else
    {
        size_t bufLen = 0;
        const char *buf = MROS_GetEncodedChunk(m, &iter, &bufLen, MAX_SCANDUMP_SIZE);
        RedisModule_ReplyWithLongLong(ctx, iter);
        RedisModule_ReplyWithStringBuffer(ctx, buf, bufLen);
    }

    return REDISMODULE_OK;
}

typedef struct __attribute__((packed))
{
    uint64_t size;
    uint32_t layer_cnt;
    uint64_t mod;
    uint64_t bytes;
} dumpedMROSHeader;

char *MROS_GetEncodedHeader(const mros *m, size_t *hdrlen)
{
    dumpedMROSHeader *hdr = RedisModule_Alloc(sizeof(dumpedMROSHeader));
    hdr->size = m->size;
    hdr->layer_cnt = m->layer_cnt;
    hdr->mod = m->mod;
    hdr->bytes = m->bytes;
    *hdrlen = sizeof(dumpedMROSHeader);
    return (char *)hdr;
}

void MROS_FreeEncodedHeader(char *hdr)
{
    RedisModule_Free(hdr);
}

const char *MROS_GetEncodedChunk(const mros *m, long long *curIter, size_t *len,
                                 size_t maxChunkSize)
{
    *len = maxChunkSize;

    size_t offset = *curIter - 1;
    size_t remaining = m->bytes - offset;

    if (remaining <= 0)
    {
        return NULL;
    }

    *len = maxChunkSize;
    if (remaining < maxChunkSize)
    {
        *len = remaining;
    }

    *curIter += *len;

    return (const char *)(m->bitmap + offset);
}
/**
 * MROS.LOADCHUNK <KEY> <ITER> <DATA>
 * Incrementally loads a mros structure.
 */
static int MROSLoadChunk_RedisCommand(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    RedisModule_AutoMemory(ctx);

    if (argc != 4)
    {
        return RedisModule_WrongArity(ctx);
    }

    long long iter;
    if (RedisModule_StringToLongLong(argv[2], &iter) != REDISMODULE_OK)
    {
        return RedisModule_ReplyWithError(ctx, "second argument must be numberic");
    }

    size_t buf_len;
    const char *buf = RedisModule_StringPtrLen(argv[3], &buf_len);

    RedisModuleKey *key = RedisModule_OpenKey(ctx, argv[1], REDISMODULE_READ | REDISMODULE_WRITE);
    mros *m;
    const int status = mrosGetKeyStatus(key, &m);

    // load header
    if (status == MROS_EMPTY && iter == 1)
    {
        const char *errMsg;

        mros *m = MROS_NewFromHeader(buf, buf_len, &errMsg);
        if (!m)
        {
            return RedisModule_ReplyWithError(ctx, errMsg);
        }
        else
        {
            RedisModule_ModuleTypeSetValue(key, MROSType, m);
            RedisModule_ReplicateVerbatim(ctx);
            return RedisModule_ReplyWithSimpleString(ctx, "OK");
        }
    }
    else if (status != MROS_OK)
    {
        return RedisModule_ReplyWithError(ctx, statusStrerror(status));
    }

    // load chunk
    const char *errMsg;
    if (MROS_LoadEncodedChunk(m, iter, buf, buf_len, &errMsg) != 0)
    {
        return RedisModule_ReplyWithError(ctx, errMsg);
    }
    else
    {
        RedisModule_ReplicateVerbatim(ctx); // Should be replicated?
        return RedisModule_ReplyWithSimpleString(ctx, "OK");
    }
}

mros *MROS_NewFromHeader(const char *buf, size_t bufLen, const char **errmsg)
{
    const dumpedMROSHeader *hdr = (const void *)buf;
    if (bufLen != sizeof(dumpedMROSHeader))
    {
        *errmsg = "ERR received bad data";
        return NULL;
    }

    mros *m = RedisModule_Calloc(1, sizeof(*m));
    m->bitmap = RedisModule_Calloc(hdr->bytes, sizeof(unsigned char));
    m->size = hdr->size;
    m->bytes = hdr->bytes;
    m->layer_cnt = hdr->layer_cnt;
    m->mod = hdr->mod;

    return m;
}

int MROS_LoadEncodedChunk(mros *m, long long iter, const char *buf, size_t bufLen,
                          const char **errmsg)
{
    size_t offset;
    iter -= bufLen;

    offset = iter - 1;

    if (bufLen > m->bytes - offset)
    {
        *errmsg = "ERR invalid chunk - Too big for current filter"; // LCOV_EXCL_LINE
        return -1;
    }

    memcpy(m->bitmap + offset, buf, bufLen);
    return 0;
}
//============================DataType Function========================
#define MROS_ENCODE_VERSION 1

static void MROSRdbSave(RedisModuleIO *io, void *obj)
{
    mros *m = (mros *)obj;

    RedisModule_SaveUnsigned(io, m->bytes);
    RedisModule_SaveUnsigned(io, m->layer_cnt);
    RedisModule_SaveUnsigned(io, m->mod);
    RedisModule_SaveUnsigned(io, m->size);
    RedisModule_SaveStringBuffer(io, (char *)m->bitmap, m->bytes);
}

static void *MROSRdbLoad(RedisModuleIO *io, int encver)
{
    if (encver > MROS_ENCODE_VERSION)
    {
        return NULL;
    }

    mros *m = RedisModule_Calloc(1, sizeof(*m));

    m->bytes = RedisModule_LoadUnsigned(io);
    m->layer_cnt = RedisModule_LoadUnsigned(io);
    m->mod = RedisModule_LoadUnsigned(io);
    m->size = RedisModule_LoadUnsigned(io);
    size_t sztmp;
    m->bitmap = (unsigned char *)RedisModule_LoadStringBuffer(io, &sztmp);

    assert(sztmp == m->bytes);

    return m;
}

static void MROSAofRewrite(RedisModuleIO *aof, RedisModuleString *key, void *value)
{
    mros *m = (mros *)value;
    size_t len;
    char *buf = MROS_GetEncodedHeader(m, &len);
    RedisModule_EmitAOF(aof, "MROS.LOADCHUNK", "slb", key, 1, buf, len);
    MROS_FreeEncodedHeader(buf);

    long long iter = MROS_CHUNKITER_INIT;
    const char *chunk;

    while ((chunk = MROS_GetEncodedChunk(m, &iter, &len, MAX_SCANDUMP_SIZE)) != NULL)
    {
        RedisModule_EmitAOF(aof, "MROS.LOADCHUNK", "slb", chunk, iter, chunk, len);
    }
}

static size_t MROSMemUsage(const void *value)
{
    const mros *m = (const mros *)value;
    return sizeof(*m) + m->bytes;
}

static void MROSFree(void *value)
{
    mros *m = (mros *)value;
    RedisModule_Free(m->bitmap);
    RedisModule_Free(m);
}

//=============================注册模块=================================
/**
 * 注册模块入口函数
 */
int RedisModule_OnLoad(RedisModuleCtx *ctx, RedisModuleString **argv, int argc)
{
    if (RedisModule_Init(ctx, "mros", MROS_MODULE_VERSION, REDISMODULE_APIVER_1) !=
        REDISMODULE_OK)
    {
        return REDISMODULE_ERR;
    }

    if (argc == 1)
    {
        RedisModule_Log(ctx, "notice", "Found empty string. Assuming ramp-packer validation");
        // Hack for ramp-packer which gives us an empty string.
        size_t tmp;
        RedisModule_StringPtrLen(argv[0], &tmp);
        if (tmp == 0)
        {
            argc = 0;
        }
    }

    if (argc % 2)
    {
        BAIL("Invalid number of arguments passed");
    }

    for (int ii = 0; ii < argc; ii += 2)
    {
        if (!rsStrcasecmp(argv[ii], "max_capacity"))
        {
            uint64_t v;
            if (RedisModule_StringToULongLong(argv[ii + 1], &v) == REDISMODULE_ERR)
            {
                BAIL("Invalid argument for 'MAX_CAPACITY'");
            }
            if (v > default_capacity)
            {
                default_capacity = v;
            }
            else if (v < 0)
            {
                BAIL("INITIAL_SIZE must be > 0");
            }
        }
        else
        {
            BAIL("Unrecognized option");
        }
    }

#define CREATE_CMD(name, tgt, attr)                                                     \
    do                                                                                  \
    {                                                                                   \
        if (RedisModule_CreateCommand(ctx, name, tgt, attr, 1, 1, 1) != REDISMODULE_OK) \
        {                                                                               \
            return REDISMODULE_ERR;                                                     \
        }                                                                               \
    } while (0)
#define CREATE_WRCMD(name, tgt) CREATE_CMD(name, tgt, "write deny-oom")
#define CREATE_ROCMD(name, tgt) CREATE_CMD(name, tgt, "readonly fast")

    // MROS- write commands
    CREATE_WRCMD("mros.insert", MROSInsert_RedisCommand);
    CREATE_WRCMD("mros.add", MROSAdd_RedisCommand);
    CREATE_WRCMD("mros.madd", MROSAdd_RedisCommand);
    CREATE_WRCMD("mros.reserve", MROSReserve_RedisCommand);

    // MROS- read commands
    CREATE_ROCMD("mros.info", MROSInfo_RedisCommand);
    CREATE_ROCMD("mros.jaccard", MROSJaccard_RedisCommand);
    CREATE_ROCMD("mros.getbitmap", MROSGetBitMap_RedisCommand);

    // MROS -AOF
    CREATE_ROCMD("mros.scandump", MROSScanDump_RedisCommand);
    CREATE_WRCMD("mros.loadchunk", MROSLoadChunk_RedisCommand);

    static RedisModuleTypeMethods mrosTypeprocs = {
        .version = REDISMODULE_TYPE_METHOD_VERSION,
        .rdb_load = MROSRdbLoad,
        .rdb_save = MROSRdbSave,
        .aof_rewrite = MROSAofRewrite,
        .free = MROSFree,
        .mem_usage = MROSMemUsage};

    MROSType = RedisModule_CreateDataType(ctx, "MROS--ysw", MROS_ENCODE_VERSION, &mrosTypeprocs);
    if (MROSType == NULL)
    {
        return REDISMODULE_ERR;
    }

    return REDISMODULE_OK;
}