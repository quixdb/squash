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

static void*
crush_malloc (size_t size, void* user_data) {
  (void) user_data;
  return malloc(size);
}

static void
crush_free (void* ptr, void* user_data) {
  (void) user_data;
  return free(ptr);
}

void crush_init_full(CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, CrushMalloc alloc, CrushFree dealloc, void* user_data, CrushDestroyNotify destroy_data)
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
	memset(ctx->buf, 0, BUF_SIZE+MAX_MATCH);
}

void crush_init(CrushContext* ctx, CrushReadFunc reader, CrushWriteFunc writer, void* user_data, CrushDestroyNotify destroy_data)
{
  crush_init_full(ctx, reader, writer, crush_malloc, crush_free, user_data, destroy_data);
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

void crush_init_stdio(CrushContext* ctx, FILE* in, FILE* out)
{
	struct CrushStdioData* data = (struct CrushStdioData*)ctx->alloc(sizeof(struct CrushStdioData), ctx->user_data);
	data->in = in;
	data->out = out;

	crush_init(ctx, crush_stdio_fread, crush_stdio_fwrite, data, crush_stdio_destroy);
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

  memset (head, 0, (HASH1_SIZE+HASH2_SIZE) * sizeof(int));
  memset (prev, 0, W_SIZE * sizeof(int));

	const int max_chain[]={4, 256, 1<<12};

	int size;
	while ((size=ctx->reader(ctx->buf, BUF_SIZE, ctx->user_data))>0)
	{
		int i;
		int h1=0;
		int h2=0;
		int p=0;

		ctx->writer(&size, sizeof(size), ctx->user_data); /* Little-endian */

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
	int size;
	while (ctx->reader(&size, sizeof(size), ctx->user_data)>0) /* Little-endian */
	{
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

	crush_init_stdio(&ctx, in, out);

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
