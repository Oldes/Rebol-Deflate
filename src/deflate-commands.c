//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// SPDX-License-Identifier: MIT
// =============================================================================
// Rebol/Deflate extension commands
// =============================================================================


#include "deflate-rebol-extension.h"
#include <stdio.h>
#include <stdlib.h> // malloc
#include <math.h>   // fmin, fmax

#define COMMAND int

#define FRM_IS_HANDLE(n, t)   (RXA_TYPE(frm,n) == RXT_HANDLE && RXA_HANDLE_TYPE(frm, n) == t)
#define ARG_Double(n)         RXA_DEC64(frm,n)
#define ARG_Float(n)          (float)RXA_DEC64(frm,n)
#define ARG_Int32(n)          RXA_INT32(frm,n)
#define ARG_Handle_Series(n)  RXA_HANDLE_CONTEXT(frm, n)->series;
#define OPT_SERIES(n)         (RXA_TYPE(frm,n) == RXT_NONE ? NULL : RXA_SERIES(frm, n))

#define RETURN_HANDLE(hob)                   \
	RXA_HANDLE(frm, 1)       = hob;          \
	RXA_HANDLE_TYPE(frm, 1)  = hob->sym;     \
	RXA_HANDLE_FLAGS(frm, 1) = hob->flags;   \
	RXA_TYPE(frm, 1) = RXT_HANDLE;           \
	return RXR_VALUE

#define RETURN_ERROR(err)  do {RXA_SERIES(frm, 1) = (REBSER*)err; return RXR_ERROR;} while(0)
static const REBYTE* ERR_INVALID_HANDLE = (const REBYTE*)"Invalid Deflate encoder or decoder handle!";
static const REBYTE* ERR_NO_DECODER     = (const REBYTE*)"Failed to create Deflate decoder!";
static const REBYTE* ERR_NO_ENCODER     = (const REBYTE*)"Failed to create Deflate encoder!";
static const REBYTE* ERR_NO_COMPRESS    = (const REBYTE*)"Failed to compress using the Deflate encoder!";
static const REBYTE* ERR_NO_DECOMPRESS  = (const REBYTE*)"Failed to decompress using the Deflate decoder!";
static const REBYTE* ERR_ENCODER_FINISHED = (const REBYTE*)"Deflate encoder is finished!";

#define APPEND_STRING(str, ...) \
	len = snprintf(NULL,0,__VA_ARGS__);\
	if (len > SERIES_REST(str)-SERIES_LEN(str)) {\
		RL_EXPAND_SERIES(str, SERIES_TAIL(str), len);\
		SERIES_TAIL(str) -= len;\
	}\
	len = snprintf( \
		SERIES_TEXT(str)+SERIES_TAIL(str),\
		SERIES_REST(str)-SERIES_TAIL(str),\
		__VA_ARGS__\
	);\
	SERIES_TAIL(str) += len;

int Common_mold(REBHOB *hob, REBSER *str) {
	size_t len;
	if (!str) return 0;
	SERIES_TAIL(str) = 0;
	APPEND_STRING(str, "0#%lx", (unsigned long)(uintptr_t)hob->handle);
	return len;
}

/*
int DeflateEncHandle_free(void *hndl) {
	if (!hndl) return 0;
	REBHOB *hob = (REBHOB*)hndl;
	Deflate_CStream *encoder = (Deflate_CStream*)hob->handle;
	//debug_print("release encoder: %p ser: %p is referenced: %i\n", encoder, hob->series, IS_MARK_HOB(hob) != 0);
	if (encoder) {
		Deflate_freeCStream(encoder);
		hob->handle = NULL;
	}
	UNMARK_HOB(hob);
	return 0;
}
int DeflateDecHandle_free(void* hndl) {
	if (!hndl) return 0;
	REBHOB *hob = (REBHOB*)hndl;
	Deflate_DStream *decoder = (Deflate_DStream*)hob->handle;
	//debug_print("release decoder: %p is referenced: %i\n", decoder, IS_MARK_HOB(hob) != 0);
	if (decoder) {
		Deflate_freeDStream(decoder);
		hob->handle = NULL;
	}
	UNMARK_HOB(hob);
	return 0;
}
*/

void* DeflateDefaultAllocFunc(size_t size) {
	return RL_MEM_ALLOC(0, size);
}
void DeflateDefaultFreeFunc(void *address) {
	RL_MEM_FREE(0, address);
}

static const struct libdeflate_options options = {
	.sizeof_options = sizeof(options),
	.malloc_func = DeflateDefaultAllocFunc,
	.free_func = DeflateDefaultFreeFunc,
};


COMMAND cmd_init_words(RXIFRM *frm, void *ctx) {
	arg_words  = RL_MAP_WORDS(RXA_SERIES(frm,1));
	type_words = RL_MAP_WORDS(RXA_SERIES(frm,2));

	// custom initialization may be done here...

	return RXR_TRUE;
}

COMMAND cmd_version(RXIFRM *frm, void *ctx) {
	RXA_TYPE(frm, 1) = RXT_TUPLE;
	RXA_TUPLE(frm, 1)[0] = LIBDEFLATE_VERSION_MAJOR;
	RXA_TUPLE(frm, 1)[1] = LIBDEFLATE_VERSION_MINOR;
	RXA_TUPLE(frm, 1)[2] = 0;
	RXA_TUPLE_LEN(frm, 1) = 3;

	return RXR_VALUE;
}

typedef enum {
    DEFLATE_MODE_DEFLATE,
    DEFLATE_MODE_ZLIB,
    DEFLATE_MODE_GZIP
} deflate_mode_t;


typedef size_t (*compress_fn_t)(
    struct libdeflate_compressor *ctx,
    const void *in,  size_t in_size,
    void       *out, size_t out_size
);

typedef size_t (*compress_bound_fn_t)(
    struct libdeflate_compressor *ctx,
    size_t                       in_size
);

typedef enum libdeflate_result (*decompress_fn_t)(
    struct libdeflate_decompressor *ctx,
    const void *in,  size_t in_size,
    void       *out, size_t out_size,
    size_t *actual_out
);

typedef enum libdeflate_result (*decompress_ex_fn_t)(
    struct libdeflate_decompressor *ctx,
    const void *in,  size_t in_size,
    void       *out, size_t out_size,
    size_t *in_consumed,
    size_t *out_consumed
);

typedef struct {
    compress_fn_t       compress;
    compress_bound_fn_t compress_bound;
    decompress_fn_t     decompress;
    decompress_ex_fn_t  decompress_ex;
} deflate_handlers_t;

static const deflate_handlers_t handlers[] = {
    [DEFLATE_MODE_DEFLATE] = {
        .compress = libdeflate_deflate_compress,
        .compress_bound = libdeflate_deflate_compress_bound,
        .decompress = libdeflate_deflate_decompress,
        .decompress_ex = libdeflate_deflate_decompress_ex
    },
    [DEFLATE_MODE_ZLIB] = {
    	.compress = libdeflate_zlib_compress,
        .compress_bound = libdeflate_zlib_compress_bound,
        .decompress = libdeflate_zlib_decompress,
        .decompress_ex = libdeflate_zlib_decompress_ex
    },
    [DEFLATE_MODE_GZIP] = {
    	.compress = libdeflate_gzip_compress,
        .compress_bound = libdeflate_gzip_compress_bound,
        .decompress = libdeflate_gzip_decompress,
        .decompress_ex = libdeflate_gzip_decompress_ex
    }
};

// Unified compression
int CompressCommon(deflate_mode_t mode, const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
    struct libdeflate_compressor *ctx;
    const deflate_handlers_t *h;
    
    if (mode >= sizeof(handlers)/sizeof(handlers[0])) return FALSE;
    h = &handlers[mode];
    if (!h->compress || !h->compress_bound) return FALSE;

    if (level > 12) level = 12;
    ctx = libdeflate_alloc_compressor_ex(level, &options);
    if (!ctx) return FALSE;

    size_t size = h->compress_bound(ctx, len);
    if (size == 0 || size > MAX_I32) {
        libdeflate_free_compressor(ctx);
        return FALSE;
    }
    *output = RL_MAKE_BINARY((REBLEN)size);

    size = h->compress(ctx, input, len, BIN_HEAD(*output), SERIES_REST(*output));

    libdeflate_free_compressor(ctx);
    
    if (size == 0) return FALSE;
    SERIES_TAIL(*output) = (REBLEN)size;
    return TRUE;
}

COMMAND cmd_compress(RXIFRM *frm, void *ctx) {
	REBSER *data    = RXA_SERIES(frm, 1);
	REBINT index    = RXA_INDEX(frm, 1);
	REBFLG ref_part = RXA_REF(frm, 2);
	REBLEN length   = SERIES_TAIL(data) - index;
	REBINT level    = RXA_REF(frm, 4) ? RXA_INT32(frm, 5) : 0;
	REBFLG ref_zlib = RXA_REF(frm, 6);
	REBFLG ref_gzip = RXA_REF(frm, 7);
	REBSER *output  = NULL;
	REBINT  error   = 0;

	deflate_mode_t mode = DEFLATE_MODE_DEFLATE;
	if (ref_zlib)  mode = DEFLATE_MODE_ZLIB;
	if (ref_gzip)  mode = DEFLATE_MODE_GZIP;

	if (ref_part) length = (REBLEN)MAX(0, MIN(length, RXA_INT64(frm, 3)));

	if (!CompressCommon(mode, (const REBYTE*)BIN_SKIP(data, index), length, (REBCNT)level, &output, &error)) {
		RETURN_ERROR(ERR_NO_COMPRESS);
	}

	RXA_SERIES(frm, 1) = output;
	RXA_TYPE(frm, 1) = RXT_BINARY;
	RXA_INDEX(frm, 1) = 0;
	return RXR_VALUE;
}


int DecompressCommon(deflate_mode_t mode, const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
    struct libdeflate_decompressor *ctx;
    int result;
    REBU64 out_len;
    size_t out_bytes = 0;
    size_t in_bytes = 0;
    const deflate_handlers_t *h = &handlers[mode];

    if (mode >= sizeof(handlers)/sizeof(handlers[0]) || !h->decompress) return FALSE;

    ctx = libdeflate_alloc_decompressor();
    if (!ctx) return FALSE;

    out_len = (limit != NO_LIMIT) ? limit : len << 2;

    if (out_len == 0) {
        *output = RL_MAKE_BINARY(1);
        libdeflate_free_decompressor(ctx);
        return TRUE;
    }
    if (out_len > MAX_I32) out_len = MAX_I32;
    *output = RL_MAKE_BINARY((REBLEN)out_len);

    if (limit != NO_LIMIT) {
        out_bytes = limit;
        result = h->decompress(ctx, input, len, BIN_HEAD(*output), out_bytes, NULL);
    } else {
retry:
        result = h->decompress_ex(ctx, input, len, BIN_HEAD(*output), SERIES_REST(*output), &in_bytes, &out_bytes);
        if (result == LIBDEFLATE_INSUFFICIENT_SPACE) {
            SERIES_TAIL(*output) = SERIES_REST(*output);
            RL_EXPAND_SERIES(*output, AT_TAIL, len >> 1);
            goto retry;
        }
    }

    if (result > 0) {
        *error = result;
        libdeflate_free_decompressor(ctx);
        return FALSE;
    }
    if (limit != NO_LIMIT && out_bytes > limit) out_bytes = limit;
    libdeflate_free_decompressor(ctx);
    SERIES_TAIL(*output) = (REBLEN)out_bytes;
    return TRUE;
}


int CompressDeflate(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
	return CompressCommon(DEFLATE_MODE_DEFLATE, input, len, level, output, error);
}
int CompressGzip(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
	return CompressCommon(DEFLATE_MODE_GZIP, input, len, level, output, error);
}
int CompressZlib(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
	return CompressCommon(DEFLATE_MODE_ZLIB, input, len, level, output, error);
}

int DecompressDeflate(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
	return DecompressCommon(DEFLATE_MODE_DEFLATE, input, len, limit, output, error);
}
int DecompressGzip(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
	return DecompressCommon(DEFLATE_MODE_GZIP, input, len, limit, output, error);
}
int DecompressZlib(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
	return DecompressCommon(DEFLATE_MODE_ZLIB, input, len, limit, output, error);
}

COMMAND cmd_decompress(RXIFRM *frm, void *ctx) {
	REBSER *data    = RXA_SERIES(frm, 1);
	REBINT index    = RXA_INDEX(frm, 1);
	REBFLG ref_part = RXA_REF(frm, 2);
	REBI64 length   = SERIES_TAIL(data) - index;
	REBI64 limit    = RXA_REF(frm, 4) ? RXA_INT64(frm, 5) : NO_LIMIT;
	REBFLG ref_zlib = RXA_REF(frm, 6);
	REBFLG ref_gzip = RXA_REF(frm, 7);
	REBSER *output  = NULL;
	REBINT  error   = 0;

	deflate_mode_t mode = DEFLATE_MODE_DEFLATE;
	if (ref_zlib)  mode = DEFLATE_MODE_ZLIB;
	if (ref_gzip)  mode = DEFLATE_MODE_GZIP;

	if (ref_part) length = MAX(0, MIN(length, RXA_INT64(frm, 3)));
	if (length < 0 || length > MAX_I32) {
		RETURN_ERROR(ERR_NO_DECOMPRESS);
	}

	if (!DecompressCommon(mode, (const REBYTE*)BIN_SKIP(data, index), (REBLEN)length, (REBCNT)limit, &output, &error)) {
		RETURN_ERROR(ERR_NO_DECOMPRESS);
	}

	RXA_SERIES(frm, 1) = output;
	RXA_TYPE(frm, 1) = RXT_BINARY;
	RXA_INDEX(frm, 1) = 0;
	return RXR_VALUE;
}
