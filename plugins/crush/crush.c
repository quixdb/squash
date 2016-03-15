/* crush.c
 * Written and placed in the public domain by Ilya Muravyov
 * Modified for use as a library and converted to C by Evan Nemerson */

#ifdef _MSC_VER
#  if !defined(_CRT_SECURE_NO_WARNINGS)
#    define _CRT_SECURE_NO_WARNINGS
#  endif
#  if !defined(_CRT_DISABLE_PERFCRIT_LOCKS)
#    define _CRT_DISABLE_PERFCRIT_LOCKS
#  endif
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "crush.h"

#define W_BITS 21
#define W_SIZE (1<<W_BITS)
#define W_MASK (W_SIZE-1)
#define SLOT_BITS 4
#define NUM_SLOTS (1<<SLOT_BITS)

#define A_BITS 2
#define B_BITS 2
#define C_BITS 2
#define D_BITS 3
#define E_BITS 5
#define F_BITS 9
#define A (1<<A_BITS)
#define B ((1<<B_BITS)+A)
#define C ((1<<C_BITS)+B)
#define D ((1<<D_BITS)+C)
#define E ((1<<E_BITS)+D)
#define F ((1<<F_BITS)+E)
#define MIN_MATCH 3
#define MAX_MATCH ((F-1)+MIN_MATCH)

#define BUF_SIZE (1<<26)

#define TOO_FAR (1<<16)

#define HASH1_LEN MIN_MATCH
#define HASH2_LEN (MIN_MATCH+1)
#define HASH1_BITS 21
#define HASH2_BITS 24
#define HASH1_SIZE (1<<HASH1_BITS)
#define HASH2_SIZE (1<<HASH2_BITS)
#define HASH1_MASK (HASH1_SIZE-1)
#define HASH2_MASK (HASH2_SIZE-1)
#define HASH1_SHIFT ((HASH1_BITS+(HASH1_LEN-1))/HASH1_LEN)
#define HASH2_SHIFT ((HASH2_BITS+(HASH2_LEN-1))/HASH2_LEN)

#if defined(_WIN32)
#elif defined(__APPLE__)
#  include <libkern/OSByteOrder.h>
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || defined(__bsdi__) || defined(__DragonFly__)
#  include <sys/endian.h>
#elif defined(__sun) || defined(sun)
#  include <sys/byteorder.h>
#else
#  include <endian.h>
#endif

#if !defined(LITTLE_ENDIAN)
#  define LITTLE_ENDIAN 4321
#endif
#if !defined(BIG_ENDIAN)
#  define BIG_ENDIAN 1234
#endif
#if !defined(BYTE_ORDER)
#  if defined(_BIG_ENDIAN)
#    define BYTE_ORDER BIG_ENDIAN
#  else
#    define BYTE_ORDER LITTLE_ENDIAN
#  endif
#endif

#if defined(__GNUC__)
#  define ENDIAN_BSWAP16(v) __builtin_bswap16(v)
#  define ENDIAN_BSWAP32(v) __builtin_bswap32(v)
#  define ENDIAN_BSWAP64(v) __builtin_bswap64(v)
#elif defined(_MSC_VER)
#  include <intrin.h>
#  define ENDIAN_BSWAP16(v) ((uint16_t) _byteswap_ushort ((unsigned short) v))
#  define ENDIAN_BSWAP32(v) ((uint32_t) _byteswap_ulong ((unsigned long) v))
#  define ENDIAN_BSWAP64(v) ((uint64_t) _byteswap_uint64 ((unsigned __int64) v))
#else
#  define ENDIAN_BSWAP16(v) ((((v) & 0xff00) >>  8) | \
			     (((v) & 0x00ff) <<  8))
#  define ENDIAN_BSWAP32(v) ((((v) & 0xff000000) >> 24) | \
			     (((v) & 0x00ff0000) >>  8) | \
			     (((v) & 0x0000ff00) <<  8) | \
			     (((v) & 0x000000ff) << 24))
#  define ENDIAN_BSWAP64(v) ((((v) & 0xff00000000000000ULL) >> 56) | \
			     (((v) & 0x00ff000000000000ULL) >> 40) | \
			     (((v) & 0x0000ff0000000000ULL) >> 24) | \
			     (((v) & 0x000000ff00000000ULL) >>  8) | \
			     (((v) & 0x00000000ff000000ULL) <<  8) | \
			     (((v) & 0x0000000000ff0000ULL) << 24) | \
			     (((v) & 0x000000000000ff00ULL) << 40) | \
			     (((v) & 0x00000000000000ffULL) << 56));
#endif

#if defined(_WIN32) || (defined(BYTE_ORDER) && defined(LITTLE_ENDIAN) && (BYTE_ORDER == LITTLE_ENDIAN))
#  define ENDIAN_FROM_LE16(x) (x)
#  define ENDIAN_FROM_LE32(x) (x)
#  define ENDIAN_FROM_LE64(x) (x)
#  define ENDIAN_FROM_BE16(x) ENDIAN_BSWAP16(x)
#  define ENDIAN_FROM_BE32(x) ENDIAN_BSWAP32(x)
#  define ENDIAN_FROM_BE64(x) ENDIAN_BSWAP64(x)
#elif defined(BYTE_ORDER) && defined(BIG_ENDIAN) && (BYTE_ORDER == BIG_ENDIAN)
#  define ENDIAN_FROM_LE16(x) ENDIAN_BSWAP16(x)
#  define ENDIAN_FROM_LE32(x) ENDIAN_BSWAP32(x)
#  define ENDIAN_FROM_LE64(x) ENDIAN_BSWAP64(x)
#  define ENDIAN_FROM_BE16(x) (x)
#  define ENDIAN_FROM_BE32(x) (x)
#  define ENDIAN_FROM_BE64(x) (x)
#else /* Can't determine endianness at compile time */
#  include <stdint.h>
#  if defined(__cplusplus)
extern "C"
#  endif

enum {
  ENDIAN_RT_LITTLE_ENDIAN = UINT32_C(0x03020100),
  ENDIAN_RT_BIG_ENDIAN    = UINT32_C(0x00010203)
};

#define ENDIAN_RT_BYTE_ORDER (((union { unsigned char bytes[4]; uint32_t value; }) { { 0, 1, 2, 3 } }).value)

#  if defined(__cplusplus)
}
#  endif

#  define ENDIAN_FROM_LE16(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_LITTLE_ENDIAN) ? (v) : ENDIAN_BSWAP16(v))
#  define ENDIAN_FROM_BE16(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_BIG_ENDIAN)    ? (v) : ENDIAN_BSWAP16(v))
#  define ENDIAN_FROM_LE32(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_LITTLE_ENDIAN) ? (v) : ENDIAN_BSWAP32(v))
#  define ENDIAN_FROM_BE32(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_BIG_ENDIAN)    ? (v) : ENDIAN_BSWAP32(v))
#  define ENDIAN_FROM_LE64(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_LITTLE_ENDIAN) ? (v) : ENDIAN_BSWAP64(v))
#  define ENDIAN_FROM_BE64(v) ((ENDIAN_RT_BYTE_ORDER == ENDIAN_RT_BIG_ENDIAN)    ? (v) : ENDIAN_BSWAP64(v))
#endif

#define ENDIAN_TO_LE16(x) ENDIAN_FROM_LE16(x)
#define ENDIAN_TO_LE32(x) ENDIAN_FROM_LE32(x)
#define ENDIAN_TO_LE64(x) ENDIAN_FROM_LE64(x)
#define ENDIAN_TO_BE16(x) ENDIAN_FROM_BE16(x)
#define ENDIAN_TO_BE32(x) ENDIAN_FROM_BE32(x)
#define ENDIAN_TO_BE64(x) ENDIAN_FROM_BE64(x)

static void*
crush_malloc (size_t size, void* user_data) {
  (void) user_data;
  return calloc(size, 1);
}

static void
crush_free (void* ptr, void* user_data) {
  (void) user_data;
  return free(ptr);
}

int crush_init_full(CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, CrushMalloc alloc, CrushFree dealloc, void* user_data, CrushDestroyNotify destroy_data)
{
	ctx->bit_buf = 0;
	ctx->bit_count = 0;

	ctx->reader = reader;
	ctx->writer = writer;
	ctx->user_data = user_data;
	ctx->user_data_destroy = destroy_data;
  ctx->alloc = alloc;
  ctx->dealloc = dealloc;

	ctx->buf = (unsigned char*)alloc(BUF_SIZE+MAX_MATCH, user_data);

  return 0;
}

int crush_init(CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, void* user_data, CrushDestroyNotify destroy_data)
{
  return crush_init_full(ctx, reader, writer, crush_malloc, crush_free, user_data, destroy_data);
}

struct CrushStdioData {
	FILE* in;
	FILE* out;
};

static void crush_stdio_destroy (void* user_data)
{
	struct CrushStdioData* data = (struct CrushStdioData*) user_data;

	fclose(data->in);
	fclose(data->out);
	free(user_data);
}

static size_t total_read = 0;
static size_t total_written = 0;

static size_t crush_stdio_fread (void* ptr, size_t size, void* user_data)
{
	return fread(ptr, 1, size, ((struct CrushStdioData*) user_data)->in);
}

static size_t crush_stdio_fwrite (const void* ptr, size_t size, void* user_data)
{
	return fwrite(ptr, 1, size, ((struct CrushStdioData*) user_data)->out);
}

int crush_init_stdio(CrushContext* ctx, FILE* in, FILE* out)
{
	struct CrushStdioData* data = (struct CrushStdioData*)ctx->alloc(sizeof(struct CrushStdioData), ctx->user_data);
  if (data == NULL)
    return -1;

	data->in = in;
	data->out = out;

	return crush_init(ctx, crush_stdio_fread, crush_stdio_fwrite, data, crush_stdio_destroy);
}

void crush_destroy(CrushContext* ctx)
{
	if (ctx->user_data_destroy != NULL && ctx->user_data != NULL)
	{
		ctx->user_data_destroy(ctx->user_data);
	}
	ctx->dealloc(ctx->buf, ctx->user_data);
}

static void init_bits(CrushContext* ctx)
{
	ctx->bit_count=ctx->bit_buf=0;
}

static void put_bits(CrushContext* ctx, int n, int x)
{
	ctx->bit_buf|=x<<ctx->bit_count;
	ctx->bit_count+=n;
	while (ctx->bit_count>=8)
	{
		ctx->writer(&(ctx->bit_buf), 1, ctx->user_data);
		ctx->bit_buf>>=8;
		ctx->bit_count-=8;
	}
}

static void flush_bits(CrushContext* ctx)
{
	put_bits(ctx, 7, 0);
	ctx->bit_count=ctx->bit_buf=0;
}

static int get_bits(CrushContext* ctx, int n)
{
	int x;
	while (ctx->bit_count<n)
	{
		unsigned char c;
		ctx->reader(&c, 1, ctx->user_data);
		ctx->bit_buf|=c<<ctx->bit_count;
		ctx->bit_count+=8;
	}
	x=ctx->bit_buf&((1<<n)-1);
	ctx->bit_buf>>=n;
	ctx->bit_count-=n;
	return x;
}

static int update_hash1(int h, int c)
{
	return ((h<<HASH1_SHIFT)+c)&HASH1_MASK;
}

static int update_hash2(int h, int c)
{
	return ((h<<HASH2_SHIFT)+c)&HASH2_MASK;
}

static int get_min(int a, int b)
{
	return a<b?a:b;
}

static int get_max(int a, int b)
{
	return a>b?a:b;
}

static int get_penalty(int a, int b)
{
	int p=0;
	while (a>b)
	{
		a>>=3;
		++p;
	}
	return p;
}

int crush_compress(CrushContext* ctx, int level)
{
  int* head = (int*)ctx->alloc((HASH1_SIZE+HASH2_SIZE) * sizeof(int), ctx->user_data);
  int* prev = (int*)ctx->alloc(W_SIZE * sizeof(int), ctx->user_data);

  if (head == NULL || prev == NULL) {
    free (head);
    free (prev);
    return -1;
  }

	const int max_chain[]={4, 256, 1<<12};

	int32_t size;
	while ((size=ctx->reader(ctx->buf, BUF_SIZE, ctx->user_data))>0)
	{
		int i;
		int h1=0;
		int h2=0;
		int p=0;

    const int32_t size_le = ENDIAN_TO_LE32(size);
		ctx->writer(&size_le, sizeof(size_le), ctx->user_data); /* Little-endian */

		for (i=0; i<HASH1_SIZE+HASH2_SIZE; ++i)
			head[i]=-1;

		for (i=0; i<HASH1_LEN; ++i)
			h1=update_hash1(h1, ctx->buf[i]);
		for (i=0; i<HASH2_LEN; ++i)
			h2=update_hash2(h2, ctx->buf[i]);

		while (p<size)
		{
			int len=MIN_MATCH-1;
			int offset=W_SIZE;

			const int max_match=get_min(MAX_MATCH, size-p);
			const int limit=get_max(p-W_SIZE, 0);

			if (head[h1]>=limit)
			{
				int s=head[h1];
				if (ctx->buf[s]==ctx->buf[p])
				{
					int l=0;
					while (++l<max_match)
						if (ctx->buf[s+l]!=ctx->buf[p+l])
							break;
					if (l>len)
					{
						len=l;
						offset=p-s;
					}
				}
			}

			if (len<MAX_MATCH)
			{
				int chain_len=max_chain[level];
				int s=head[h2+HASH1_SIZE];

				while ((chain_len--!=0)&&(s>=limit))
				{
					if ((ctx->buf[s+len]==ctx->buf[p+len])&&(ctx->buf[s]==ctx->buf[p]))
					{
						int l=0;
						while (++l<max_match)
							if (ctx->buf[s+l]!=ctx->buf[p+l])
								break;
						if (l>len+get_penalty((p-s)>>4, offset))
						{
							len=l;
							offset=p-s;
						}
						if (l==max_match)
							break;
					}
					s=prev[s&W_MASK];
				}
			}

			if ((len==MIN_MATCH)&&(offset>TOO_FAR))
				len=0;

			if ((level>=2)&&(len>=MIN_MATCH)&&(len<max_match))
			{
				const int next_p=p+1;
				const int max_lazy=get_min(len+4, max_match);

				int chain_len=max_chain[level];
				int s=head[update_hash2(h2, ctx->buf[next_p+(HASH2_LEN-1)])+HASH1_SIZE];

				while ((chain_len--!=0)&&(s>=limit))
				{
					if ((ctx->buf[s+len]==ctx->buf[next_p+len])&&(ctx->buf[s]==ctx->buf[next_p]))
					{
						int l=0;
						while (++l<max_lazy)
							if (ctx->buf[s+l]!=ctx->buf[next_p+l])
								break;
						if (l>len+get_penalty(next_p-s, offset))
						{
							len=0;
							break;
						}
						if (l==max_lazy)
							break;
					}
					s=prev[s&W_MASK];
				}
			}

			if (len>=MIN_MATCH) /* Match */
			{
				const int l=len-MIN_MATCH;
				int log=W_BITS-NUM_SLOTS;

				put_bits(ctx, 1, 1);

				if (l<A)
				{
					put_bits(ctx, 1, 1); /* 1 */
					put_bits(ctx, A_BITS, l);
				}
				else if (l<B)
				{
					put_bits(ctx, 2, 1<<1); /* 01 */
					put_bits(ctx, B_BITS, l-A);
				}
				else if (l<C)
				{
					put_bits(ctx, 3, 1<<2); /* 001 */
					put_bits(ctx, C_BITS, l-B);
				}
				else if (l<D)
				{
					put_bits(ctx, 4, 1<<3); /* 0001 */
					put_bits(ctx, D_BITS, l-C);
				}
				else if (l<E)
				{
					put_bits(ctx, 5, 1<<4); /* 00001 */
					put_bits(ctx, E_BITS, l-D);
				}
				else
				{
					put_bits(ctx, 5, 0); /* 00000 */
					put_bits(ctx, F_BITS, l-E);
				}

				--offset;
				while (offset>=(2<<log))
					++log;
				put_bits(ctx, SLOT_BITS, log-(W_BITS-NUM_SLOTS));
				if (log>(W_BITS-NUM_SLOTS))
					put_bits(ctx, log, offset-(1<<log));
				else
					put_bits(ctx, W_BITS-(NUM_SLOTS-1), offset);
			}
			else /* Literal */
			{
				len=1;
				put_bits(ctx, 9, ctx->buf[p]<<1); /* 0 xxxxxxxx */
			}

			while (len--!=0) /* Insert new strings */
			{
				head[h1]=p;
				prev[p&W_MASK]=head[h2+HASH1_SIZE];
				head[h2+HASH1_SIZE]=p;
				++p;
				h1=update_hash1(h1, ctx->buf[p+(HASH1_LEN-1)]);
				h2=update_hash2(h2, ctx->buf[p+(HASH2_LEN-1)]);
			}
		}

		flush_bits(ctx);
	}
	ctx->dealloc(head, ctx->user_data);
	ctx->dealloc(prev, ctx->user_data);
	return 0;
}

int crush_decompress(CrushContext* ctx)
{
	int32_t size;
	while (ctx->reader(&size, sizeof(size), ctx->user_data)>0) /* Little-endian */
	{
    size = ENDIAN_FROM_LE32(size);
		int p=0;

		if ((size<1)||(size>BUF_SIZE))
		{
			return -1;
		}

		init_bits(ctx);

		while (p<size)
		{
			if (get_bits(ctx, 1))
			{
				int len;
				int log;
				int s;
				if (get_bits(ctx, 1))
					len=get_bits(ctx, A_BITS);
				else if (get_bits(ctx, 1))
					len=get_bits(ctx, B_BITS)+A;
				else if (get_bits(ctx, 1))
					len=get_bits(ctx, C_BITS)+B;
				else if (get_bits(ctx, 1))
					len=get_bits(ctx, D_BITS)+C;
				else if (get_bits(ctx, 1))
					len=get_bits(ctx, E_BITS)+D;
				else
					len=get_bits(ctx, F_BITS)+E;

				log=get_bits(ctx, SLOT_BITS)+(W_BITS-NUM_SLOTS);
				s=~(log>(W_BITS-NUM_SLOTS)
					?get_bits(ctx, log)+(1<<log)
					:get_bits(ctx, W_BITS-(NUM_SLOTS-1)))+p;
				if (s<0)
				{
					return -2;
				}

				ctx->buf[p++]=ctx->buf[s++];
				ctx->buf[p++]=ctx->buf[s++];
				ctx->buf[p++]=ctx->buf[s++];
				while (len--!=0)
					ctx->buf[p++]=ctx->buf[s++];
			}
			else
				ctx->buf[p++]=get_bits(ctx, 8);
		}

		ctx->writer(ctx->buf, p, ctx->user_data);
	}
	return 0;
}

#if defined(CRUSH_CLI)
int main(int argc, char* argv[])
{
	CrushContext ctx;
	FILE* in;
	FILE* out;
  int res;

	if (argc!=4)
	{
		fprintf(stderr,
			"CRUSH by Ilya Muravyov\n"
			"Usage: CRUSH command infile outfile\n"
			"Commands:\n"
			"  c[f,x] Compress (Fast..Max)\n"
			"  d      Decompress\n");
		exit(1);
	}

	in=fopen(argv[2], "rb");
	if (!in)
	{
		perror(argv[2]);
		exit(1);
	}
	out=fopen(argv[3], "wb");
	if (!out)
	{
		perror(argv[3]);
		exit(1);
	}

	res = crush_init_stdio(&ctx, in, out);
  if (res != 0)
    return -1;

	if (*argv[1]=='c')
	{
		printf("Compressing %s...\n", argv[2]);
		if (crush_compress(&ctx, argv[1][1]=='f'?0:(argv[1][1]=='x'?2:1))<0)
		{
			fprintf (stderr, "Failed.\n");
		}
	}
	else if (*argv[1]=='d')
	{
		printf("Decompressing %s...\n", argv[2]);
		if(crush_decompress(&ctx)<0)
		{
			fprintf (stderr, "Failed.\n");
		}
	}
	else
	{
		fprintf(stderr, "Unknown command: %s\n", argv[1]);
		exit(1);
	}

	crush_destroy (&ctx);

	return 0;
}
#endif /* defined(CRUSH_CLI) */
