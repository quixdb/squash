#if !defined(CRUSH_H)
#define CRUSH_H

#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef size_t (*CrushReadFunc)(void* ptr, size_t size, void* user_data);
typedef size_t (*CrushWriteFunc)(const void* ptr, size_t size, void* user_data);
typedef void (*CrushDestroyNotify)(void* ptr);
typedef void* (*CrushMalloc)(size_t size, void* user_data);
typedef void (*CrushFree)(void* ptr, void* user_data);

typedef struct {
	int bit_buf;
	int bit_count;
	unsigned char* buf;
  unsigned char* obuf;
  int obuf_pos;
	CrushReadFunc reader;
	CrushWriteFunc writer;
  CrushMalloc alloc;
  CrushFree dealloc;
	void* user_data;
	CrushDestroyNotify user_data_destroy;
} CrushContext;

int crush_init (CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, void* user_data, CrushDestroyNotify destroy_data);
int crush_init_full (CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, CrushMalloc alloc, CrushFree dealloc, void* user_data, CrushDestroyNotify destroy_data);
int crush_init_stdio (CrushContext* ctx, FILE* in, FILE* out);
void crush_destroy (CrushContext* ctx);
int crush_compress (CrushContext* ctx, int level);
int crush_decompress (CrushContext* ctx);

#ifdef __cplusplus
}
#endif

#endif /* !defined(CRUSH_H) */
