#!/bin/bash

gcc tiny_packer.c -o tiny_packer

mkdir -p test_files
cd test_files

echo "This is a test file with repeated content. This is a test file with repeated content." > test1.txt
dd if=/dev/urandom of=test2.bin bs=1024 count=100 2>/dev/null  # 100KB random data
cp ../tiny_packer.c test3.txt  # test source code file

# Test compression and decompression
for file in test*.{txt,bin}; do
    echo "Testing file: $file"
    
    # Compress
    ../tiny_packer -c "$file" "${file}.packed"
    
    # Decompress
    ../tiny_packer -d "${file}.packed" "${file}.unpacked"
    
    # Compare original file with decompressed file
    if cmp -s "$file" "${file}.unpacked"; then
        echo "✅ Test PASSED: Files match perfectly"
    else
        echo "❌ Test FAILED: Files do not match"
    fi
    echo "------------------------"
done 