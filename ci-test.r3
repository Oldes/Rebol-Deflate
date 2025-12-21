Rebol [
    title: "Rebol/Deflate extension CI test"
]

print ["Running test on Rebol build:" mold to-block system/build]

;; make sure that we load a fresh extension
try [system/modules/deflate: none]
;; use current directory as a modules location
system/options/modules: what-dir

;; import the extension
deflate: import 'deflate

print as-yellow "Content of the module..."
? deflate


print ["Deflate version:" deflate/version]


errors: 0
file: %src/rebol-extension.h ;; File to use as a source to compress.
text: to string! read file
sha1: checksum text 'sha1

;-----------------------------------------------------------------------
print-horizontal-line

print as-yellow "Basic deflate de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: to decimal! dt [bin: deflate/compress/level text :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        str: to string! deflate/decompress bin
        equal? text str
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ] 
]

print as-yellow "Basic zlib de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: to decimal! dt [bin: deflate/compress/zlib/level text :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        str: to string! deflate/decompress/zlib bin
        equal? text str
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ] 
]

print as-yellow "Basic gzip de/compression using various qualities."
for quality 1 9 1 [
    unless all [
        time: to decimal! dt [bin: deflate/compress/gzip/level text :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        str: to string! deflate/decompress/gzip bin
        equal? text str
    ][
        print [as-red "Failed to compress using quality:" quality]
        ++ errors
    ] 
]


print as-yellow "Using short input..."
short-text: "X"
for quality 1 9 1 [
    unless all [
        time: to decimal! dt [bin: deflate/compress/level short-text :quality]
        print ["  quality:" quality "^-compressed size:" length? bin "time:" time]
        str: to string! deflate/decompress bin
        equal? short-text str
    ][
        print [as-red "Failed to de/compress short text using quality:" quality]
        ++ errors
    ] 
]


;-----------------------------------------------------------------------
print-horizontal-line

prin as-yellow "TEST RESULT: "
print either errors == 0 [as-green "OK"][as-red "FAILED!"]
quit/return errors

