# TinyPacker

TinyPacker is a lightweight compression/decompression tool based on the LZ77 algorithm, implementing data compression using sliding window technique.

## Features

- Uses 4KB (4096 bytes) sliding window
- Optimized for small files, automatically switches to uncompressed mode for files under 100 bytes
- Supports match lengths from 3 bytes (minimum) to 255 bytes (maximum)
- Automatically detects compression efficiency, switches to uncompressed mode when compression ratio exceeds 90%
- Uses "TPK1" magic number for file format identification

## Usage

Compile:

```bash
gcc tiny_packer.c -o tiny_packer
```

Compress:

```bash
./tiny_packer -c <input_file> <output_file>
```

Decompress:

```bash
./tiny_packer -d <input_file> <output_file>
```

## How It Works

The compression algorithm employs the following strategies:

1. **File Header Structure**

   - 4 bytes magic number (0x54504B01, "TPK1")
   - 1 byte flag (indicates whether compression is used)

2. **Compression Modes**

   - Literals: Direct storage of unmatched byte sequences
   - Match References: Stores (offset, length) pairs pointing to previous identical sequences

3. **Smart Compression Decision**
   - Evaluates compression effectiveness on the first 4KB of input data
   - Chooses uncompressed storage if compression ratio exceeds 90% or file size is less than 100 bytes

## Performance Characteristics

- Achieves good compression ratios for text files with repetitive content (e.g., source code)
- Automatically switches to uncompressed mode for random data (e.g., already compressed files, encrypted data)
- Small files might show slight size increase due to header overhead

## Limitations

- 4KB sliding window limit, not suitable for files with long-distance repetition patterns
- Maximum match length of 255 bytes
- Maximum literal block size of 255 bytes
- Not optimized for large files, primarily designed for small file compression

## Use Cases

- Configuration file compression
- Small text file compression
- Source code file compression
- Scenarios requiring simple and fast compression

## File Format

Compressed file structure:

1. Magic Number: 4 bytes (0x54504B01)
2. Flag: 1 byte
3. Compressed Data:
   - Uncompressed: Direct storage of original data
   - Compressed: Alternating storage of literals and match references
