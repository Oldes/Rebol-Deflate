Rebol [
    title: "Rebol/Deflate extension CI test"
]

print ["Running test on Rebol build:" mold to-block system/build]
system/options/quiet: false
system/options/log/rebol: 4

;; make sure that we load a fresh extension
try [system/modules/deflate: none]

if modules-dir: get-env 'REBOL_MODULES_DIR [
    system/options/modules: to-real-file modules-dir
    ls (system/options/modules)
]

;; import the extension
deflate: import 'deflate

print as-yellow "Content of the module..."
? deflate

print ["Deflate version:" deflate/version]


errors: 0
file: %src/rebol-extension.h ;; File to use as a source to compress.
data: read/binary file       ;; raw bytes, no deline, no decoding

;-----------------------------------------------------------------------
print-hline
print as-yellow "Basic deflate de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: dt [bin: deflate/compress/level data :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        equal? data deflate/decompress bin
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ]
]

print as-yellow "Basic zlib de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: dt [bin: deflate/compress/zlib/level data :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        equal? data deflate/decompress/zlib bin
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ]
]

print as-yellow "Basic gzip de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: dt [bin: deflate/compress/gzip/level data :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        equal? data deflate/decompress/gzip bin
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ]
]

print as-yellow "Default compression level (without /level)."
bin-default: deflate/compress data
bin-level6:  deflate/compress/level data 6
either all [
    equal? data deflate/decompress bin-default
    (length? bin-default) < length? data    ;; must really compress, not only store
    equal? bin-default bin-level6           ;; default is level 6
][
    print ["  default size:" length? bin-default]
][
    print [as-red "Default compression level is not 6! sizes:" length? bin-default length? bin-level6]
    ++ errors
]

print as-yellow "Compression levels outside of the 1-9 range."
foreach level [-5 0 10 11 12 99][
    either all [
        bin: attempt [deflate/compress/level data level]
        equal? data deflate/decompress bin
    ][
        print ["  level:" level "^-compressed size:" length? bin]
    ][
        print [as-red "Failed to compress using level:" level]
        ++ errors
    ]
]
;; levels 10-12 must not be worse than 9 and 0 must only store the data
size-of: func [level][length? deflate/compress/level data level]
either all [
    (size-of 12) <= size-of 9
    (size-of 0)  >= length? data
    (size-of -5)  = size-of 0     ;; negative level is clamped to 0
    (size-of 99)  = size-of 12    ;; too high level is clamped to 12
][
    print "  ok: level limits"
][
    print [as-red "Unexpected sizes for levels 0, 12 or out of range."]
    ++ errors
]

print as-yellow "Decompress with /size (maximum output size)."
bin: deflate/compress data
foreach [name size expected] reduce [
    "exact size"    (length? data)       (length? data)
    "larger limit"  (2 * length? data)   (length? data)
    "smaller limit" 100                  100
    "tiny limit"    1                    1
    "zero limit"    0                    0
][
    either all [
        res: attempt [deflate/decompress/size bin size]
        expected = length? res
        equal? res copy/part data expected
    ][
        print ["  ok:" name]
    ][
        print [as-red "Failed to decompress with /size:" name]
        ++ errors
    ]
]
;; the same for the zlib and gzip wrappers
either all [
    equal? data deflate/decompress/zlib/size deflate/compress/zlib data (length? data) + 10
    equal? data deflate/decompress/gzip/size deflate/compress/gzip data (length? data) + 10
][
    print "  ok: zlib and gzip"
][
    print [as-red "Failed to decompress zlib/gzip with /size."]
    ++ errors
]
;; tiny input must not hang while growing the output
either equal? "X" to string! deflate/decompress deflate/compress "X" [
    print "  ok: tiny input"
][
    print [as-red "Failed with tiny input."]
    ++ errors
]

print as-yellow "Using short input..."
short-text: "X"
for quality 1 9 1 [
    unless all [
        bin: deflate/compress/level short-text :quality
        equal? short-text to string! deflate/decompress bin
    ][
        print [as-red "Failed to de/compress short text using quality:" quality]
        ++ errors
    ]
]

;-----------------------------------------------------------------------
print-hline
print as-yellow "Using the registered compression methods."
foreach method [deflate zlib gzip][
    unless all [
        equal? data decompress compress data :method :method
        bin: compress data :method
        print ["  method:" method "^-compressed size:" length? bin]
    ][
        print [as-red "Failed to de/compress using method:" method]
        ++ errors
    ]
]

;-----------------------------------------------------------------------
if find system/schemes 'http [
    print-hline
    print as-yellow "Validate, that support is mentioned in HTTP requests."
    if none? attempt [
        ? system/schemes/http/headers/Accept-Encoding
        find system/schemes/http/headers/Accept-Encoding "deflate"
    ][
        print as-red "Deflate not found in the HTTP's Accept-Encoding."
        ++ errors
    ]
]
;-----------------------------------------------------------------------
print-hline

prin as-yellow "TEST RESULT: "
print either errors == 0 [as-green "OK"][as-red "FAILED!"]
quit/return errors
