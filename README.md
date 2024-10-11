# TinyPacker

TinyPacker is a lightweight compression/decompression tool specifically designed for small files. It employs a simple and efficient sliding windows algorithm that is particularly well-suited for handling small text files, configuration files, or other small data files.

## Features

- Optimized for small files, ideal for handling files ranging from a few KB to a few MB
- Simple and fast compression and decompression algorithm
- Command-line interface, easy to integrate with other tools or scripts
- Written in C, offering good cross-platform compatibility

## Usage

After compiling TinyPacker, you can use the following commands for compression and decompression:

## Compression:

```bash
./tiny_packer -c input_file output_file
```

## Decompression:

```bash
./tiny_packer -d input_file output_file
```

## How It Works

The compression algorithm works as follows:

- **1.Literal Encoding** - Sequences of bytes that do not match any previous sequence are encoded as literals.
- **2.Back-Reference Encoding** - Repeated sequences of bytes are identified and encoded as a backreference to a previous occurrence of the same sequence.

## Performance

TinyPacker uses a simple sliding window algorithm to find and encode repeated sequences within the data. This approach is particularly effective for small files with repetitive content.

## Limitations

- TinyPacker is optimized for small files and may not be as effective as professional compression tools for large files
- Compression ratios can vary significantly depending on file content

## Use Cases

- Compressing configuration files
- Reducing the size of small log files
- Compressing small text-based data files
- Quick compression for temporary storage or transmission of small files
