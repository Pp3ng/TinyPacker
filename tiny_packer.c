#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <stdint.h>

static int find_match_length(const uint8_t *ptr1, const uint8_t *ptr2, const uint8_t *end)
{
  int match_length = 0;
  while (ptr2 < end && *ptr1 == *ptr2)
  {
    ++match_length;
    ptr1++;
    ptr2++;
  }
  return match_length;
}

static uint8_t *write_literal(uint8_t *output, const uint8_t *input, int length)
{
  assert(length >= 1 && length <= 64);
  *output++ = length + 191;
  assert((output[-1] & 0xc0) == 0xc0);
  memcpy(output, input, length);
  return output + length;
}

#define FLUSH_LITERAL                                   \
  {                                                     \
    w = write_literal(w, literal_start, literal_count); \
    literal_start += literal_count;                     \
    literal_count = 0;                                  \
  }

static uint8_t *write_back_reference(uint8_t *output, int offset, int length)
{
  assert(offset <= -1 && offset >= -256);
  assert(length >= 4 && length <= 195);
  *output++ = 195 - length;
  *output++ = offset;
  return output;
}

long compress(uint8_t *output, const uint8_t *input, long input_size)
{
  if (input_size == 0)
  {
    return 0; // Handle empty input file
  }

  uint8_t *w = output;
  const uint8_t *r = input;
  const uint8_t *end = input + input_size;
  const uint8_t *literal_start = input;
  int literal_count = 0;

  while (r < end)
  {
    const uint8_t *p = r - 1;
    int best_length = 0;
    const uint8_t *best_position = p;

    while (p > input && p >= (r - 255))
    {
      int length = find_match_length(p, r, end);
      if (length > best_length)
      {
        best_length = length;
        best_position = p;
        if (best_length >= 195)
          break; // Exit loop early
      }
      p--;
    }

    if (best_length > 3)
    {
      if (literal_count)
        FLUSH_LITERAL;
      if (best_length > 0x7f + 0x40 + 4)
        best_length = 0x7f + 0x40 + 4;
      literal_start += best_length;
      int offset = best_position - r;
      r += best_length;
      w = write_back_reference(w, offset, best_length);
    }
    else
    {
      literal_count++;
      if (literal_count == 0x40)
        FLUSH_LITERAL;
      r++;
    }
  }

  if (literal_count)
    FLUSH_LITERAL;
  return w - output;
}

long decompress(uint8_t *output, const uint8_t *input, long input_size)
{
  if (input_size == 0)
  {
    return 0; // Handle empty input file
  }

  uint8_t *w = output;
  const uint8_t *r = input;
  const uint8_t *end = input + input_size;

  while (r < end)
  {
    int length = (uint8_t)(*r++);

    if ((length & 0xc0) == 0xc0)
    {
      length &= 0x3f;
      while (length-- >= 0)
        *w++ = *r++;
    }
    else
    {
      int offset = -256 | (signed char)(*r++);
      length = 195 - length;
      assert(length >= 4 && length <= 195);
      do
      {
        *w = w[offset];
        ++w;
      } while (--length);
    }
  }

  return w - output;
}

int main(int argc, char **argv)
{
  if (argc < 4 || (!strcmp(argv[1], "-c") && argc < 4) || (!strcmp(argv[1], "-d") && argc < 4))
  {
    puts("Usage:\n  Compress: zpack -c <INPUT> <OUTPUT>\n  Decompress: zpack -d <INPUT> <OUTPUT>");
    return 0;
  }

  const char *mode = argv[1];
  const char *input_filename = argv[2];
  const char *output_filename = argv[3];

  FILE *input_file = fopen(input_filename, "rb");
  if (!input_file)
  {
    perror(input_filename);
    return 1;
  }

  fseek(input_file, 0, SEEK_END);
  long input_size = ftell(input_file);
  fseek(input_file, 0, SEEK_SET);

  uint8_t *input_buffer = malloc(input_size + 1); // Allocate 1 extra byte to ensure null termination
  if (!input_buffer)
  {
    perror("Failed to allocate memory for input file");
    fclose(input_file);
    return 1;
  }
  size_t bytes_read = fread(input_buffer, 1, input_size, input_file);
  if (bytes_read != (size_t)input_size)
  {
    perror("Error reading input file");
    free(input_buffer);
    fclose(input_file);
    return 1;
  }
  input_buffer[input_size] = '\0'; // Ensure null termination
  fclose(input_file);

  if (!strcmp(mode, "-c"))
  {
    // Compression mode
    uint8_t *output_buffer = malloc(input_size * 2); // Allocate output buffer for compressed data
    if (!output_buffer)
    {
      perror("Failed to allocate memory for compressed data");
      free(input_buffer);
      return 1;
    }

    long compressed_size = compress(output_buffer, input_buffer, input_size);
    printf("%s: size=%ld compressed=%ld %ld%%\n", input_filename, input_size, compressed_size, compressed_size * 100 / input_size);

    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file)
    {
      perror("Failed to open output file");
      free(input_buffer);
      free(output_buffer);
      return 1;
    }
    fwrite(output_buffer, 1, compressed_size, output_file);
    fclose(output_file);

    free(output_buffer);
  }
  else if (!strcmp(mode, "-d"))
  {
    // Decompression mode
    uint8_t *output_buffer = malloc(input_size * 10); // Allocate a larger buffer for decompressed data
    if (!output_buffer)
    {
      perror("Failed to allocate memory for decompressed data");
      free(input_buffer);
      return 1;
    }

    long decompressed_size = decompress(output_buffer, input_buffer, input_size);
    printf("%s: size=%ld decompressed=%ld\n", input_filename, input_size, decompressed_size);

    FILE *output_file = fopen(output_filename, "wb");
    if (!output_file)
    {
      perror("Failed to open output file");
      free(input_buffer);
      free(output_buffer);
      return 1;
    }
    fwrite(output_buffer, 1, decompressed_size, output_file);
    fclose(output_file);

    free(output_buffer);
  }
  else
  {
    puts("Invalid mode. Use -c for compression or -d for decompression.");
    free(input_buffer);
    return 1;
  }

  free(input_buffer);
  return 0;
}