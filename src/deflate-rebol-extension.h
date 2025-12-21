//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Deflate extension
// SPDX-License-Identifier: MIT
// =============================================================================
// NOTE: auto-generated file, do not modify!


#include "rebol-extension.h"
#include "libdeflate.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4267)   /* conversion from 'size_t' to 'int', possible loss of data */
#endif

#define SERIES_TEXT(s)   ((char*)SERIES_DATA(s))

#define MIN_REBOL_VER 3
#define MIN_REBOL_REV 20
#define MIN_REBOL_UPD 5
#define VERSION(a, b, c) (a << 16) + (b << 8) + c
#define MIN_REBOL_VERSION VERSION(MIN_REBOL_VER, MIN_REBOL_REV, MIN_REBOL_UPD)

#ifndef NO_LIMIT
#define NO_LIMIT ((REBCNT)-1)
#endif


extern REBCNT Handle_DeflateEncoder;
extern REBCNT Handle_DeflateDecoder;

extern u32* arg_words;
extern u32* type_words;

enum ext_commands {
	CMD_Deflate_INIT_WORDS,
	CMD_Deflate_VERSION,
	CMD_Deflate_COMPRESS,
	CMD_Deflate_DECOMPRESS,
};


int cmd_init_words(RXIFRM *frm, void *ctx);
int cmd_version(RXIFRM *frm, void *ctx);
int cmd_compress(RXIFRM *frm, void *ctx);
int cmd_decompress(RXIFRM *frm, void *ctx);

enum ma_arg_words {W_ARG_0
};
enum ma_type_words {W_TYPE_0
};

typedef int (*MyCommandPointer)(RXIFRM *frm, void *ctx);

#define DEFLATE_EXT_INIT_CODE \
	"REBOL [Title: \"Rebol Deflate Extension\" Name: Deflate Type: module Version: 0.1.0 Needs: 3.20.5 Author: Oldes Date: 21-Dec-2025/13:15:20 License: MIT Url: https://github.com/Oldes/Rebol-Deflate]\n"\
	"init-words: command [args [block!] type [block!]]\n"\
	"version: command [\"Native Deflate version\"]\n"\
	"compress: command [\"Compress data using Deflate\" data [binary! any-string!] \"Input data to compress.\" /part \"Limit the input data to a given length.\" length [integer!] \"Length of input data.\" /level quality [integer!] \"Compression level from 1 to 9.\" /zlib \"Assumes the zlib wrapper format\" /gzip \"Assumes the gzip wrapper format\"]\n"\
	"decompress: command [\"Decompress data using Deflate\" data [binary! any-string!] \"Input data to decompress.\" /part \"Limit the input data to a given length.\" length [integer!] \"Length of input data.\" /size \"Limit the output size.\" bytes [integer!] \"Maximum number of uncompressed bytes.\" /zlib \"Assumes the zlib wrapper format\" /gzip \"Assumes the gzip wrapper format\"]\n"\
	"init-words [][]\n"\
	"protect/hide 'init-words\n"\
	"\n"\
	";; Update HTTP scheme that it's able to use the Deflate compression\n"\
	"attempt [\n"\
	"	unless find system/schemes/http/headers/Accept-Encoding \"deflate\" [\n"\
	"		append system/schemes/http/headers/Accept-Encoding \",deflate\"\n"\
	"	]\n"\
	"]\n"

#ifdef  USE_TRACES
#include <stdio.h>
#define debug_print(fmt, ...) do { printf(fmt, __VA_ARGS__); } while (0)
#define trace(str) puts(str)
#else
#define debug_print(fmt, ...)
#define trace(str) 
#endif

