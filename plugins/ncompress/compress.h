#if !defined(COMPRESS_H)
#define COMPRESS_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

enum CompressStatus {
  COMPRESS_OK,
  COMPRESS_READ_ERROR,
  COMPRESS_WRITE_ERROR,
  COMPRESS_FAILED
};

typedef size_t (*CompressReadFunc)(void* ptr, size_t size, void* user_data);
typedef size_t (*CompressWriteFunc)(void* ptr, size_t size, void* user_data);

enum CompressStatus compress (uint8_t* compressed, size_t* compressed_length,
                              const uint8_t* uncompressed, size_t uncompressed_length);
enum CompressStatus decompress (uint8_t* decompressed, size_t* decompressed_length,
                                const uint8_t* compressed, size_t compressed_length);

#ifdef __cplusplus
}
#endif

#endif /* !defined(COMPRESS_H) */
