REBOL [
	Title:   "Rebol Deflate Extension"
	Name:    deflate
	Version: 0.1.0
	Needs:   3.20.5
	Author:  @Oldes
	License: MIT
	Options: [delay]
	Url:     https://github.com/Oldes/Rebol-Deflate
	Exports: []
	Purpose: {
		Deflate, zlib and gzip (de)compression using libdeflate, plus
		registration of "deflate", "zlib" and "gzip" as compression
		methods usable from Rebol's compress/decompress natives.

		Data only - never evaluated. The C header (gen-deflate.h) and the
		command table (gen-deflate.c) are generated from this file by
		make-extension.r3.
	}
]

;; ---------------------------------------------------------------------------
;; Banner put on top of the generated files.
logo: {//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Deflate extension
// SPDX-License-Identifier: MIT
// =============================================================================
// NOTE: auto-generated file, do not modify!}

;; ---------------------------------------------------------------------------
;; C-side configuration
;;
;; NOTE: libdeflate.h is carried here rather than by `c-include:`, because
;; the template only emits `c-include:` inside the `#ifdef REB_EXT` branch
;; and the libdeflate types are needed in both build modes.
;; LIBDEFLATE_DLL must stay undefined even for the shared build: with it,
;; the functions are dllimport-ed on Windows and their addresses are no
;; longer constants for the static handler table in deflate-commands.c.
c-header: {
#include "libdeflate.h"

#ifndef NO_LIMIT
#define NO_LIMIT ((REBCNT)-1)
#endif

int CompressDeflate(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressDeflate(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int CompressGzip(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressGzip(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int CompressZlib(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressZlib(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
}

;; ---------------------------------------------------------------------------
;; No `handles:` block: libdeflate has no streaming API, so there are no
;; encoder/decoder handles (the old sources only had them commented out).

;; ---------------------------------------------------------------------------
;; Commands. Order is significant - it fixes the command indices.
;; The `_init` command is injected as the first one by the generator.
commands: [
	version: ["Native Deflate version"]
	compress: [
		"Compress data using Deflate"
		data [binary! any-string!] "Input data to compress."
		/part                      "Limit the input data to a given length."
		 length [integer!]         "Length of input data."
		/level quality [integer!]  "Compression level from 1 to 9."
		/zlib "Assumes the zlib wrapper format"
		/gzip "Assumes the gzip wrapper format"
	]
	decompress: [
		"Decompress data using Deflate"
		data [binary! any-string!] "Input data to decompress."
		/part                      "Limit the input data to a given length."
		 length [integer!]         "Length of input data."
		/size                      "Limit the output size."
		 bytes [integer!]          "Maximum number of uncompressed bytes."
		/zlib "Assumes the zlib wrapper format"
		/gzip "Assumes the gzip wrapper format"
	]
]

;; ---------------------------------------------------------------------------
;; Module body. Registers "deflate" with the HTTP scheme's Accept-Encoding,
;; the same as the extension being converted did from its init code.
mezzanine: [
	attempt [
		unless find system/schemes/http/headers/Accept-Encoding "deflate" [
			append system/schemes/http/headers/Accept-Encoding ",deflate"
		]
	]
]
