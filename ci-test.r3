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
