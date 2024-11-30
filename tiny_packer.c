#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define WINDOW_SIZE 4096
#define MIN_MATCH 3
#define MAX_MATCH 255
#define LITERAL_MAX 255
#define MAGIC_HEADER 0x54504B01 // "TPK1" (TinyPacker v1)
#define FLAG_UNCOMPRESSED 0x01

typedef struct
{
  uint8_t type; // 0 = literal, 1 = match
  union
  {
    struct
    {
      uint8_t length;
      uint8_t data[LITERAL_MAX];
    } literal;
    struct
    {
      uint16_t offset;
      uint8_t length;
    } match;
  };
} Token;

// find longest match
static void find_match(const uint8_t *data, size_t pos, size_t size,
                       uint16_t *out_offset, uint8_t *out_length)
{
  *out_offset = 0;
  *out_length = 0;

  if (pos < MIN_MATCH)
    return;

  size_t window_start = (pos > WINDOW_SIZE) ? pos - WINDOW_SIZE : 0;
  size_t max_match_length = size - pos;
  if (max_match_length > MAX_MATCH)
    max_match_length = MAX_MATCH;

  for (size_t i = window_start; i < pos; i++)
  {
    size_t len = 0;
    while (len < max_match_length && data[i + len] == data[pos + len])
    {
      len++;
    }

    if (len >= MIN_MATCH && len > *out_length)
    {
      *out_length = len;
      *out_offset = pos - i;
      if (len == MAX_MATCH)
        break;
    }
  }
}

size_t compress(const uint8_t *input, size_t input_size, uint8_t *output)
{
  size_t in_pos = 0;
  size_t out_pos = 0;

  // write magic header and flag placeholder
  output[out_pos++] = MAGIC_HEADER & 0xFF;
  output[out_pos++] = (MAGIC_HEADER >> 8) & 0xFF;
  output[out_pos++] = (MAGIC_HEADER >> 16) & 0xFF;
  output[out_pos++] = (MAGIC_HEADER >> 24) & 0xFF;
  output[out_pos++] = 0; // flag placeholder

  // try to compress the first 4KB of data
  const size_t TEST_SIZE = 4096;
  size_t test_size = (input_size < TEST_SIZE) ? input_size : TEST_SIZE;
  uint8_t *temp_buffer = malloc(test_size * 2);
  size_t temp_pos = 0;
  size_t match_count = 0;

  while (in_pos < test_size)
  {
    uint16_t match_offset;
    uint8_t match_length;
    find_match(input, in_pos, test_size, &match_offset, &match_length);

    if (match_length >= MIN_MATCH)
    {
      match_count++;
      temp_pos += 4; // match token size
      in_pos += match_length;
    }
    else
    {
      temp_pos += 2 + 1; // literal token size
      in_pos++;
    }
  }

  free(temp_buffer);

  // if compression ratio is not good enough, store the original data
  float compression_ratio = (float)temp_pos / test_size;
  if (compression_ratio > 0.9 || input_size < 100) {// 90% compression ratio
    output[4] = FLAG_UNCOMPRESSED;
    memcpy(output + out_pos, input, input_size);
    return out_pos + input_size;
  }

  // reset position and start actual compression
  in_pos = 0;

  while (in_pos < input_size)
  {
    uint16_t match_offset;
    uint8_t match_length;
    find_match(input, in_pos, input_size, &match_offset, &match_length);

    if (match_length >= MIN_MATCH)
    {
      output[out_pos++] = 1;
      output[out_pos++] = match_offset & 0xFF;
      output[out_pos++] = (match_offset >> 8) & 0xFF;
      output[out_pos++] = match_length;
      in_pos += match_length;
    }
    else
    {
      uint8_t lit_length = 0;
      size_t lit_start = in_pos;

      while (lit_length < LITERAL_MAX && in_pos < input_size)
      {
        uint16_t next_match_offset;
        uint8_t next_match_length;
        find_match(input, in_pos, input_size, &next_match_offset,
                   &next_match_length);

        if (next_match_length >= MIN_MATCH)
          break;
        lit_length++;
        in_pos++;
      }

      output[out_pos++] = 0;
      output[out_pos++] = lit_length;
      memcpy(output + out_pos, input + lit_start, lit_length);
      out_pos += lit_length;
    }
  }

  return out_pos;
}

size_t decompress(const uint8_t *input, size_t input_size, uint8_t *output)
{
  if (input_size < 5)
    return 0; // at least 5 bytes needed(4 magic + 1 flag)

  // check magic header
  uint32_t magic =
      input[0] | (input[1] << 8) | (input[2] << 16) | (input[3] << 24);
  if (magic != MAGIC_HEADER)
  {
    fprintf(stderr, "Invalid file format\n");
    return 0;
  }

  // check if the data is uncompressed
  if (input[4] & FLAG_UNCOMPRESSED)
  {
    memcpy(output, input + 5, input_size - 5);
    return input_size - 5;
  }

  size_t in_pos = 5;
  size_t out_pos = 0;

  while (in_pos < input_size)
  {
    uint8_t type = input[in_pos++];

    if (type == 0)
    {
      uint8_t length = input[in_pos++];
      memcpy(output + out_pos, input + in_pos, length);
      in_pos += length;
      out_pos += length;
    }
    else
    {
      uint16_t offset = input[in_pos] | (input[in_pos + 1] << 8);
      uint8_t length = input[in_pos + 2];
      in_pos += 3;

      for (int i = 0; i < length; i++)
      {
        output[out_pos] = output[out_pos - offset];
        out_pos++;
      }
    }
  }

  return out_pos;
}

// file processing
static uint8_t *read_file(const char *filename, size_t *size)
{
  FILE *f = fopen(filename, "rb");
  if (!f)
  {
    perror("Failed to open input file");
    return NULL;
  }

  fseek(f, 0, SEEK_END);
  *size = ftell(f);
  fseek(f, 0, SEEK_SET);

  uint8_t *buffer = malloc(*size);
  if (!buffer)
  {
    perror("Failed to allocate memory");
    fclose(f);
    return NULL;
  }

  if (fread(buffer, 1, *size, f) != *size)
  {
    perror("Failed to read file");
    free(buffer);
    fclose(f);
    return NULL;
  }

  fclose(f);
  return buffer;
}

static int write_file(const char *filename, const uint8_t *data, size_t size)
{
  FILE *f = fopen(filename, "wb");
  if (!f)
  {
    perror("Failed to open output file");
    return 0;
  }

  size_t written = fwrite(data, 1, size, f);
  fclose(f);

  return written == size;
}

int main(int argc, char **argv)
{
  if (argc != 4)
  {
    printf("Usage:\n"
           "  Compress:   %s -c <input> <output>\n"
           "  Decompress: %s -d <input> <output>\n",
           argv[0], argv[0]);
    return 1;
  }

  const char *mode = argv[1];
  const char *input_file = argv[2];
  const char *output_file = argv[3];

  // read input file
  size_t input_size;
  uint8_t *input_data = read_file(input_file, &input_size);
  if (!input_data)
    return 1;

  if (strcmp(mode, "-c") == 0)
  {
    // compress mode
    uint8_t *output_data = malloc(input_size * 2); // worst case scenario
    if (!output_data)
    {
      perror("Failed to allocate memory");
      free(input_data);
      return 1;
    }

    size_t compressed_size = compress(input_data, input_size, output_data);
    printf("Compressed: %zu -> %zu bytes (%.2f%%)\n", input_size,
           compressed_size, (float)compressed_size * 100 / input_size);

    if (!write_file(output_file, output_data, compressed_size))
    {
      free(input_data);
      free(output_data);
      return 1;
    }

    free(output_data);
  }
  else if (strcmp(mode, "-d") == 0)
  {
    // decompress mode
    uint8_t *output_data = malloc(input_size * 10); // predicted size for worst case
    if (!output_data)
    {
      perror("Failed to allocate memory");
      free(input_data);
      return 1;
    }

    size_t decompressed_size = decompress(input_data, input_size, output_data);
    printf("Decompressed: %zu -> %zu bytes\n", input_size, decompressed_size);

    if (!write_file(output_file, output_data, decompressed_size))
    {
      free(input_data);
      free(output_data);
      return 1;
    }

    free(output_data);
  }
  else
  {
    printf("Invalid mode: use -c for compression or -d for decompression\n");
    free(input_data);
    return 1;
  }

  free(input_data);
  return 0;
}