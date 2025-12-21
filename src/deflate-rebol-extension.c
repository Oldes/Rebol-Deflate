//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// SPDX-License-Identifier: MIT
// =============================================================================
// Rebol/Deflate extension
// =============================================================================

#include "deflate-rebol-extension.h"

RL_LIB *RL; // Link back to reb-lib from embedded extensions

//==== Globals ===============================================================//
extern MyCommandPointer Command[];
REBCNT Handle_DeflateEncoder;
REBCNT Handle_DeflateDecoder;

u32* arg_words;
u32* type_words;

//============================================================================//

static const char* init_block = DEFLATE_EXT_INIT_CODE;

int CompressDeflate(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressDeflate(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int CompressGzip(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressGzip(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int CompressZlib(const REBYTE *input, REBLEN len, REBCNT level, REBSER **output, REBINT *error);
int DecompressZlib(const REBYTE *input, REBLEN len, REBLEN limit, REBSER **output, REBINT *error);
int Common_mold(REBHOB *hob, REBSER *ser);

//int DeflateEncHandle_free(void* hndl);
//int DeflateEncHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);
//int DeflateEncHandle_set_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);
//
//int DeflateDecHandle_free(void* hndl);
//int DeflateDecHandle_get_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);
//int DeflateDecHandle_set_path(REBHOB *hob, REBCNT word, REBCNT *type, RXIARG *arg);


RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	RL = lib;
	REBYTE ver[8];
	RL_VERSION(ver);
	debug_print(
		"RXinit deflate-extension; Rebol v%i.%i.%i\n",
		ver[1], ver[2], ver[3]);

	if (MIN_REBOL_VERSION > VERSION(ver[1], ver[2], ver[3])) {
		debug_print(
			"Needs at least Rebol v%i.%i.%i!\n",
			 MIN_REBOL_VER, MIN_REBOL_REV, MIN_REBOL_UPD);
		return 0;
	}
	if (!CHECK_STRUCT_ALIGN) {
		trace("CHECK_STRUCT_ALIGN failed!");
		return 0;
	}

//	REBHSP spec;
//	spec.mold = Common_mold;
//
//	spec.size      = sizeof(void*);
//	spec.flags     = HANDLE_REQUIRES_HOB_ON_FREE;
//	//spec.free      = DeflateEncHandle_free;
//	//spec.get_path  = DeflateEncHandle_get_path;
//	//spec.set_path  = DeflateEncHandle_set_path;
//	Handle_DeflateEncoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"deflate-encoder", &spec);
//
//	spec.size      = sizeof(void*);
//	spec.flags     = HANDLE_REQUIRES_HOB_ON_FREE;
//	//spec.free      = DeflateDecHandle_free;
//	//spec.get_path  = DeflateDecHandle_get_path;
//	//spec.set_path  = DeflateDecHandle_set_path;
//	Handle_DeflateDecoder = RL_REGISTER_HANDLE_SPEC((REBYTE*)"deflate-decoder", &spec);


	RL_REGISTER_COMPRESS_METHOD(cb_cast("deflate"), CompressDeflate, DecompressDeflate);
	RL_REGISTER_COMPRESS_METHOD(cb_cast("gzip"), CompressGzip, DecompressGzip);
	RL_REGISTER_COMPRESS_METHOD(cb_cast("zlib"), CompressZlib, DecompressZlib);


	return init_block;
}

RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *ctx) {
	return Command[cmd](frm, ctx);
}
