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

Streaming API is not part of this extension. Use [Rebol/Zlib-ng](https://github.com/Oldes/Rebol-Zlib-ng) instead, if needed.

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
* `/zlib` Assumes the zlib wrapper format
* `/gzip` Assumes the gzip wrapper format

#### `decompress` `:data`
Decompress data using Deflate
* `data` `[binary! any-string!]` Input data to decompress.
* `/part` Limit the input data to a given length.
* `length` `[integer!]` Length of input data.
* `/size` Limit the output size.
* `bytes` `[integer!]` Maximum number of uncompressed bytes.
* `/zlib` Assumes the zlib wrapper format
* `/gzip` Assumes the gzip wrapper format


## Other extension values:
```rebol
attempt [
	unless find system/schemes/http/headers/Accept-Encoding "deflate" [
		append system/schemes/http/headers/Accept-Encoding ",deflate"
	]
]
```
