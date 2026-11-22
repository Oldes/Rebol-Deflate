//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Deflate extension
// SPDX-License-Identifier: MIT
// ===========================================================================
// Command implementations of the Rebol/Deflate extension.
//
// One function per command; the enum, the declarations, the dispatch table
// and the `_init` handler are all generated from deflate.reb. COMMAND and
// RETURN_ERROR come from reb-ext-common.h, pulled in by gen-deflate.h.
//

#include "gen-deflate.h"

#ifdef _MSC_VER
#pragma warning(disable:4267)   /* conversion from 'size_t' to 'int', possible loss of data */
#endif

static const REBYTE* ERR_NO_COMPRESS    = (const REBYTE*)"Failed to compress using the Deflate encoder!";
static const REBYTE* ERR_NO_DECOMPRESS  = (const REBYTE*)"Failed to decompress using the Deflate decoder!";


//== allocators ================================================================

static void* DeflateDefaultAllocFunc(size_t size) {
	return RL_MEM_ALLOC(0, size);
}
static void DeflateDefaultFreeFunc(void *address) {
	RL_MEM_FREE(0, address);
}

static const struct libdeflate_options options = {
	.sizeof_options = sizeof(options),
	.malloc_func = DeflateDefaultAllocFunc,
	.free_func = DeflateDefaultFreeFunc,
};


//== format handlers ===========================================================

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
		.compress       = libdeflate_deflate_compress,
		.compress_bound = libdeflate_deflate_compress_bound,
		.decompress     = libdeflate_deflate_decompress,
		.decompress_ex  = libdeflate_deflate_decompress_ex
	},
	[DEFLATE_MODE_ZLIB] = {
		.compress       = libdeflate_zlib_compress,
		.compress_bound = libdeflate_zlib_compress_bound,
		.decompress     = libdeflate_zlib_decompress,
		.decompress_ex  = libdeflate_zlib_decompress_ex
	},
	[DEFLATE_MODE_GZIP] = {
		.compress       = libdeflate_gzip_compress,
		.compress_bound = libdeflate_gzip_compress_bound,
		.decompress     = libdeflate_gzip_decompress,
		.decompress_ex  = libdeflate_gzip_decompress_ex
	}
};


//== de/compression ============================================================

/* Default level used when /level is not specified (same as zlib's default). */
#define DEFLATE_DEFAULT_LEVEL 6

// Unified compression
static int CompressCommon(deflate_mode_t mode, const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error) {
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

static int DecompressCommon(deflate_mode_t mode, const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error) {
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

	// libdeflate cannot stop in the middle of a stream, so the data are
	// always decompressed as a whole (growing the output as needed) and
	// the result is cut to the requested limit afterwards.
	// The limit is used only as the initial size of the output buffer.
	while (TRUE) {
		result = h->decompress_ex(ctx, input, len, BIN_HEAD(*output), SERIES_REST(*output), &in_bytes, &out_bytes);
		if (result != LIBDEFLATE_INSUFFICIENT_SPACE) break;
		if (SERIES_REST(*output) >= MAX_I32) {
			libdeflate_free_decompressor(ctx);
			return FALSE;
		}
		SERIES_TAIL(*output) = SERIES_REST(*output);
		RL_EXPAND_SERIES(*output, AT_TAIL, SERIES_REST(*output)); // double the size
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

// Callbacks registered with RL_REGISTER_COMPRESS_METHOD in Deflate_Init.
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


//== commands ==================================================================

COMMAND cmd_deflate_version(RXIFRM *frm, void *ctx) {
	RXA_TYPE(frm, 1) = RXT_TUPLE;
	RXA_TUPLE(frm, 1)[0] = LIBDEFLATE_VERSION_MAJOR;
	RXA_TUPLE(frm, 1)[1] = LIBDEFLATE_VERSION_MINOR;
	RXA_TUPLE(frm, 1)[2] = 0;
	RXA_TUPLE_LEN(frm, 1) = 3;
	return RXR_VALUE;
}

COMMAND cmd_deflate_compress(RXIFRM *frm, void *ctx) {
	REBSER *data    = RXA_SERIES(frm, 1);
	REBINT index    = RXA_INDEX(frm, 1);
	REBFLG ref_part = RXA_REF(frm, 2);
	REBLEN length   = SERIES_TAIL(data) - index;
	REBINT level    = RXA_REF(frm, 4) ? RXA_INT32(frm, 5) : DEFLATE_DEFAULT_LEVEL;
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

COMMAND cmd_deflate_decompress(RXIFRM *frm, void *ctx) {
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
