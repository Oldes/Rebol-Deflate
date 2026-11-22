//   ____  __   __        ______        __
//  / __ \/ /__/ /__ ___ /_  __/__ ____/ /
// / /_/ / / _  / -_|_-<_ / / / -_) __/ _ \
// \____/_/\_,_/\__/___(@)_/  \__/\__/_// /
//  ~~~ oldes.huhuman at gmail.com ~~~ /_/
//
// Project: Rebol/Deflate extension
// SPDX-License-Identifier: MIT
// ===========================================================================
// Entry points for the Rebol/Deflate extension module.
//
//   REB_EXT defined ... standalone deflate-x64.rebx
//   REB_EXT absent .... compiled into the host
//
// One-time setup lives in Deflate_Init(), called from the generated `_init`
// command when the module body evaluates - not from the entry points, so
// that `Options: [delay]` can postpone it until the module is imported.
//

#include "gen-deflate.h"

#ifdef REB_EXT
// Standalone builds are their own binary and must supply the storage.
// Embedded builds use host-lib.c's definition, declared extern by reb-lib.h.
RL_LIB *RL;
#endif

static char *init_block = DEFLATE_EXT_INIT_CODE;


// Registers the "deflate", "gzip" and "zlib" de/compression methods.
// Runs when the module body evaluates, so it happens at the same point in
// both build modes - and only on first import under `delay`.
//
// Returns plain TRUE/FALSE, NOT an RXR_* code: RXR_FALSE is 3, which is
// truthy in C. The generated handler maps the result onto RXR_TRUE/RXR_FALSE.
int Deflate_Init(void) {
	RL_REGISTER_COMPRESS_METHOD(cb_cast("deflate"), CompressDeflate, DecompressDeflate);
	RL_REGISTER_COMPRESS_METHOD(cb_cast("gzip"),    CompressGzip,    DecompressGzip);
	RL_REGISTER_COMPRESS_METHOD(cb_cast("zlib"),    CompressZlib,    DecompressZlib);
	return TRUE;
}


#ifdef REB_EXT

/***********************************************************************
**  Standalone extension library
***********************************************************************/

RXIEXT const char *RX_Init(int opts, RL_LIB *lib) {
	REBYTE ver[8];
	RL = lib;
	RL_VERSION(ver);
	debug_print("RX_Init deflate-extension; Rebol v%i.%i.%i\n", ver[1], ver[2], ver[3]);

	if (MIN_REBOL_VERSION > VERSION(ver[1], ver[2], ver[3])) return 0;
	if (!CHECK_STRUCT_ALIGN) {
		trace("CHECK_STRUCT_ALIGN failed!");
		return 0;
	}
	return init_block;
}

RXIEXT int RX_Quit(int opts) {
	return 0;
}

// Reports the RL_API ABI this was built against, so `load-extension`
// can refuse an incompatible host. An absent symbol means ABI 0.
RXIEXT int RX_Abi(void) {
	return RL_ABI_VERSION;
}

// Resolved by name, so the spelling is fixed. The bounds-checked
// dispatcher is generated into gen-deflate.c.
RXIEXT int RX_Call(int cmd, RXIFRM *frm, void *ctx) {
	return Deflate_RX_Call(cmd, frm, ctx);
}

#endif
