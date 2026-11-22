[![rebol-deflate](https://github.com/user-attachments/assets/4ed5a794-6a7f-472a-b958-9bc5f0a1f545)](https://github.com/Oldes/Rebol-Deflate)

[![Rebol-Deflate CI](https://github.com/Oldes/Rebol-Deflate/actions/workflows/main.yml/badge.svg)](https://github.com/Oldes/Rebol-Deflate/actions/workflows/main.yml)
[![Gitter](https://badges.gitter.im/rebol3/community.svg)](https://app.gitter.im/#/room/#Rebol3:gitter.im)
[![Zulip](https://img.shields.io/badge/zulip-join_chat-brightgreen.svg)](https://rebol.zulipchat.com/)


# Rebol/Deflate

[libdeflate](https://github.com/ebiggers/libdeflate) extension for [Rebol3](https://github.com/Oldes/Rebol3) (version 3.20.5 and higher)

## Basic Usage
```rebol
deflate: import deflate
bin: deflate/compress mold system
str: deflate/decompress bin

```
Because this extension replaces existing `deflate`, `zlib` and `gzip` compression methods, one can use also simple:
```rebol
bin: compress mold system 'zlib
str: decompress bin 'zlib
```

Compression level, wrapper format and output size limit can be chosen using refinements:
```rebol
bin: deflate/compress/gzip/level data 9        ;; 0 (store only) to 12, default is 6
str: deflate/decompress/gzip/size bin 100      ;; first 100 bytes only
```

Streaming API is not part of this extension. Use [Rebol/Zlib-ng](https://github.com/Oldes/Rebol-Zlib-ng) if you need streaming de/compression.

Other compression extensions: [Rebol/Brotli](https://github.com/Oldes/Rebol-Brotli), [Rebol/Zstd](https://github.com/Oldes/Rebol-Zstd).

## Building

The extension is built using [Siskin Builder](https://github.com/Siskin-framework/Builder/) from the [Rebol-Deflate.nest](Rebol-Deflate.nest) file.
The libdeflate library must be built first (using its CMake build) and only then the extension itself, because the extension links against the installed library.
So for each target (`x86`, `x64`, `arm64`) and each library type (static or shared) run these two eggs in this order, for example for the static x64 build:
```
./siskin Rebol-Deflate.nest cmake-deflate-x64  # 1. build and install the static libdeflate
./siskin Rebol-Deflate.nest deflate-x64        # 2. build the extension
```
(the shared variants are named `cmake-deflate-shared-x64` and `deflate-shared-x64`).

The C header and the command table (`src/gen-deflate.*`) and the command reference below are generated from `src/deflate.reb` by `make-extension.r3`.

## Extension commands:


#### `version`
Native Deflate version

#### `compress` `:data`
Compress data using Deflate
* `data` `[binary! any-string!]` Input data to compress.
* `/part` Limit the input data to a given length.
* `length` `[integer!]` Length of input data.
* `/level`
* `quality` `[integer!]` Compression level from 0 (no compression) to 12 (default is 6).
* `/zlib` Use the zlib wrapper format
* `/gzip` Use the gzip wrapper format (wins over /zlib)

#### `decompress` `:data`
Decompress data using Deflate
* `data` `[binary! any-string!]` Input data to decompress.
* `/part` Limit the input data to a given length.
* `length` `[integer!]` Length of input data.
* `/size` Limit the output size (the data are still fully decompressed first).
* `bytes` `[integer!]` Maximum number of uncompressed bytes; longer output is cut.
* `/zlib` Assumes the zlib wrapper format
* `/gzip` Assumes the gzip wrapper format (wins over /zlib)


## Other extension values:
```rebol
attempt [
	unless find system/schemes/http/headers/Accept-Encoding "deflate" [
		append system/schemes/http/headers/Accept-Encoding ",deflate"
	]
]
```
