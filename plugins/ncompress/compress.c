/* Based on ncompress.  Hacked to use buffers instead of file
   descriptors, and not use global variables.
 */

#include <sys/types.h>
#include <string.h>

#include <stdio.h>
#include <assert.h>

#include "compress.h"

/* Default buffer sizes */
#ifndef BUFSIZ
#define BUFSIZ 8192
#endif

#ifndef	IBUFSIZ
# define IBUFSIZ BUFSIZ
#endif
#ifndef	OBUFSIZ
# define OBUFSIZ BUFSIZ
#endif

/* Defines for first few bytes of header */
#define	MAGIC_1		(char_type)'\037'/* First byte of compressed file				*/
#define	MAGIC_2		(char_type)'\235'/* Second byte of compressed file				*/
#define BIT_MASK	0x1f			/* Mask for 'number of compresssion bits'		*/
									/* Masks 0x20 and 0x40 are free.  				*/
									/* I think 0x20 should mean that there is		*/
									/* a fourth header byte (for expansion).    	*/
#define BLOCK_MODE	0x80			/* Block compresssion if table is full and		*/
									/* compression rate is dropping flush tables	*/

/* the next two codes should not be changed lightly, as they must not	*/
/* lie within the contiguous general code space.						*/
#define FIRST	257					/* first free entry 							*/
#define	CLEAR	256					/* table clear output code 						*/

#define INIT_BITS 9			/* initial number of bits/code */

#ifndef	BYTEORDER
# if defined(BYTE_ORDER)
#  if BYTE_ORDER == LITTLE_ENDIAN
#   define BYTEORDER 4321
#  else
#   define BYTEORDER 1234
#  endif
# else
#  warning Unable to figure out byteorder, defaulting to slow byte swapping
#  define BYTEORDER 0000
# endif
#endif

#ifndef	NOALLIGN
# define NOALLIGN 0
#endif

/* System has no binary mode */
#ifndef	O_BINARY
# define O_BINARY 0
#endif

/* Modern machines should work fine with FAST */
#define FAST

#ifdef FAST
#	define	HBITS		17			/* 50% occupancy */
#	define	HSIZE	   (1<<HBITS)
#	define	HMASK	   (HSIZE-1)
#	define	HPRIME		 9941
#	define	BITS		   16
#else
#	if BITS == 16
#		define HSIZE	69001		/* 95% occupancy */
#	endif
#	if BITS == 15
#		define HSIZE	35023		/* 94% occupancy */
#	endif
#	if BITS == 14
#		define HSIZE	18013		/* 91% occupancy */
#	endif
#	if BITS == 13
#		define HSIZE	9001		/* 91% occupancy */
#	endif
#	if BITS <= 12
#		define HSIZE	5003		/* 80% occupancy */
#	endif
#endif

#define CHECK_GAP 10000

typedef long int      code_int;
typedef long int      count_int;
typedef long int      cmp_code_int;
typedef unsigned char char_type;

#define ARGVAL() (*++(*argv) || (--argc && *++argv))

#define MAXCODE(n)	(1L << (n))

union bytes {
	long word;
	struct {
#if BYTEORDER == 4321
		char_type b1;
		char_type b2;
		char_type b3;
		char_type b4;
#elif BYTEORDER == 1234
		char_type b4;
		char_type b3;
		char_type b2;
		char_type b1;
#else
		int dummy;
#endif
	} bytes;
};
#if BYTEORDER == 4321 && NOALLIGN == 1
# define	output(b,o,c,n)	{													\
							*(long *)&((b)[(o)>>3]) |= ((long)(c))<<((o)&0x7);\
							(o) += (n);										\
						}
#elif BYTEORDER == 1234
# define	output(b,o,c,n)	{	char_type	*p = &(b)[(o)>>3];				\
							union bytes i;									\
							i.word = ((long)(c))<<((o)&0x7);				\
							p[0] |= i.bytes.b1;								\
							p[1] |= i.bytes.b2;								\
							p[2] |= i.bytes.b3;								\
							(o) += (n);										\
						}
#else
# define	output(b,o,c,n)	{	char_type	*p = &(b)[(o)>>3];				\
							long		 i = ((long)(c))<<((o)&0x7);	\
							p[0] |= (char_type)(i);							\
							p[1] |= (char_type)(i>>8);						\
							p[2] |= (char_type)(i>>16);						\
							(o) += (n);										\
						}
#endif
#if BYTEORDER == 4321 && NOALLIGN == 1
# define	input(b,o,c,n,m){													\
							(c) = (*(long *)(&(b)[(o)>>3])>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}
#else
# define	input(b,o,c,n,m){	char_type 		*p = &(b)[(o)>>3];			\
							(c) = ((((long)(p[0]))|((long)(p[1])<<8)|		\
									 ((long)(p[2])<<16))>>((o)&0x7))&(m);	\
							(o) += (n);										\
						}
#endif

#define	htabof(i)				htab[i]
#define	codetabof(i)			codetab[i]
#define	tab_prefixof(i)			codetabof(i)
#define	tab_suffixof(i)			((char_type *)(htab))[i]
#define	de_stack				((char_type *)&(htab[HSIZE-1]))
#define	clear_htab()			memset(htab, -1, sizeof(htab))
#define	clear_tab_prefixof()	memset(codetab, 0, 256);

#ifdef FAST
static const int primetab[256] =		/* Special secondary hash table.		*/
{
	 1013, -1061, 1109, -1181, 1231, -1291, 1361, -1429,
	 1481, -1531, 1583, -1627, 1699, -1759, 1831, -1889,
	 1973, -2017, 2083, -2137, 2213, -2273, 2339, -2383,
	 2441, -2531, 2593, -2663, 2707, -2753, 2819, -2887,
	 2957, -3023, 3089, -3181, 3251, -3313, 3361, -3449,
	 3511, -3557, 3617, -3677, 3739, -3821, 3881, -3931,
	 4013, -4079, 4139, -4219, 4271, -4349, 4423, -4493,
	 4561, -4639, 4691, -4783, 4831, -4931, 4973, -5023,
	 5101, -5179, 5261, -5333, 5413, -5471, 5521, -5591,
	 5659, -5737, 5807, -5857, 5923, -6029, 6089, -6151,
	 6221, -6287, 6343, -6397, 6491, -6571, 6659, -6709,
	 6791, -6857, 6917, -6983, 7043, -7129, 7213, -7297,
	 7369, -7477, 7529, -7577, 7643, -7703, 7789, -7873,
	 7933, -8017, 8093, -8171, 8237, -8297, 8387, -8461,
	 8543, -8627, 8689, -8741, 8819, -8867, 8963, -9029,
	 9109, -9181, 9241, -9323, 9397, -9439, 9511, -9613,
	 9677, -9743, 9811, -9871, 9941,-10061,10111,-10177,
	10259,-10321,10399,-10477,10567,-10639,10711,-10789,
	10867,-10949,11047,-11113,11173,-11261,11329,-11423,
	11491,-11587,11681,-11777,11827,-11903,11959,-12041,
	12109,-12197,12263,-12343,12413,-12487,12541,-12611,
	12671,-12757,12829,-12917,12979,-13043,13127,-13187,
	13291,-13367,13451,-13523,13619,-13691,13751,-13829,
	13901,-13967,14057,-14153,14249,-14341,14419,-14489,
	14557,-14633,14717,-14767,14831,-14897,14983,-15083,
	15149,-15233,15289,-15359,15427,-15497,15583,-15649,
	15733,-15791,15881,-15937,16057,-16097,16189,-16267,
	16363,-16447,16529,-16619,16691,-16763,16879,-16937,
	17021,-17093,17183,-17257,17341,-17401,17477,-17551,
	17623,-17713,17791,-17891,17957,-18041,18097,-18169,
	18233,-18307,18379,-18451,18523,-18637,18731,-18803,
	18919,-19031,19121,-19211,19273,-19381,19429,-19477
};
#endif

/*
 * compress fdin to fdout
 *
 * Algorithm:  use open addressing double hashing (no chaining) on the
 * prefix code / next character combination.  We do a variant of Knuth's
 * algorithm D (vol. 3, sec. 6.4) along with G. Knott's relatively-prime
 * secondary probe.  Here, the modular division first probe is gives way
 * to a faster exclusive-or manipulation.  Also do block compression with
 * an adaptive reset, whereby the code table is cleared when the compression
 * ratio decreases, but after the table fills.  The variable-length output
 * codes are re-sized at this point, and a special CLEAR code is generated
 * for the decompressor.  Late addition:  construct the table according to
 * file size for noticeable speed improvement on small files.  Please direct
 * questions about this implementation to ames!jaw.
 */
enum CompressStatus compress (uint8_t* compressed, size_t* compressed_length, const uint8_t* uncompressed, size_t uncompressed_length) {
	long hp;
	size_t rpos;
	size_t outbits;
	size_t rlop;
	int stcode;
	code_int free_ent;
	int boff;
	int n_bits;
	int ratio;
	long checkpoint;
	code_int extcode;
	union {
		long code;
		struct {
			char_type c;
			unsigned short ent;
		} e;
	} fcode;
  long bytes_in = 0;
  size_t bytes_out = 0;
  int maxbits = BITS;
  int block_mode = BLOCK_MODE;
  char_type outbuf[OBUFSIZ+2048];
  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

	ratio = 0;
	checkpoint = CHECK_GAP;
	extcode = MAXCODE(n_bits = INIT_BITS)+1;
	stcode = 1;
	free_ent = FIRST;

	memset(outbuf, 0, sizeof(outbuf));
	bytes_out = 0;
  bytes_in = 0;
	outbuf[0] = MAGIC_1;
	outbuf[1] = MAGIC_2;
	outbuf[2] = (char)(maxbits | block_mode);
	boff = outbits = (3<<3);
	fcode.code = 0;

	clear_htab();

  /* while ((rsize = reader(inbuf, IBUFSIZ, user_data)) > 0) { */
  /*   fprintf (stderr, "bytes in: %zu\n", bytes_in); */
		if (bytes_in == 0) {
			fcode.e.ent = uncompressed[0];
			rpos = 1;
		} else
			rpos = 0;

		rlop = 0;

		do {
			if (free_ent >= extcode && fcode.e.ent < FIRST) {
				if (n_bits < maxbits) {
					boff = outbits = (outbits-1)+((n_bits<<3)-
					                 ((outbits-boff-1+(n_bits<<3))%(n_bits<<3)));
					if (++n_bits < maxbits)
						extcode = MAXCODE(n_bits)+1;
					else
						extcode = MAXCODE(n_bits);
				} else {
					extcode = MAXCODE(16)+OBUFSIZ;
					stcode = 0;
				}
			}

			if (!stcode && bytes_in >= checkpoint && fcode.e.ent < FIRST) {
				long int rat;

				checkpoint = bytes_in + CHECK_GAP;

				if (bytes_in > 0x007fffff) {
					/* shift will overflow */
					rat = (bytes_out+(outbits>>3)) >> 8;

					/* Don't divide by zero */
					if (rat == 0)
						rat = 0x7fffffff;
					else
						rat = bytes_in / rat;
				} else
					/* 8 fractional bits */
					rat = (bytes_in << 8) / (bytes_out+(outbits>>3));
				if (rat >= ratio)
					ratio = (int)rat;
				else {
					ratio = 0;
					clear_htab();
					output(outbuf, outbits, CLEAR, n_bits);
					boff = outbits = (outbits-1)+((n_bits<<3)-
					                 ((outbits-boff-1+(n_bits<<3))%(n_bits<<3)));
					extcode = MAXCODE(n_bits = INIT_BITS)+1;
					free_ent = FIRST;
					stcode = 1;
				}
			}

			if (outbits >= (OBUFSIZ<<3)) {
        if (OBUFSIZ + bytes_out <= *compressed_length) {
          memcpy(compressed + bytes_out, outbuf, OBUFSIZ);
        } else {
          return COMPRESS_WRITE_ERROR;
        }

				outbits -= (OBUFSIZ<<3);
				boff = -(((OBUFSIZ<<3)-boff)%(n_bits<<3));
				bytes_out += OBUFSIZ;

				memcpy(outbuf, outbuf+OBUFSIZ, (outbits>>3)+1);
				memset(outbuf+(outbits>>3)+1, '\0', OBUFSIZ);
			}

			{
				int i = uncompressed_length-rlop;

				if ((code_int)i > extcode-free_ent)
					i = (int)(extcode-free_ent);

				if ((size_t)i > ((sizeof(outbuf) - 32)*8 - outbits)/n_bits)
					i = ((sizeof(outbuf) - 32)*8 - outbits)/n_bits;

				if (!stcode && (long)i > checkpoint-bytes_in)
					i = (int)(checkpoint-bytes_in);

				rlop += i;
				bytes_in += i;
			}

			goto next;
hfound:		fcode.e.ent = codetabof(hp);
next:		if (rpos >= rlop)
				goto endlop;
next2:		fcode.e.c = uncompressed[rpos++];

#ifndef FAST
			{
				code_int	i;
				hp = (((long)(fcode.e.c)) << (BITS-8)) ^ (long)(fcode.e.ent);

				if ((i = htabof(hp)) == fcode.code)
					goto hfound;

				if (i != -1) {
					long disp;

					disp = (HSIZE - hp)-1;	/* secondary hash (after G. Knott) */

					do {
						if ((hp -= disp) < 0)
							hp += HSIZE;

						if ((i = htabof(hp)) == fcode.code)
							goto hfound;
					} while (i != -1);
				}
			}
#else
			{
				long i, p;
				hp = ((((long)(fcode.e.c)) << (HBITS-8)) ^ (long)(fcode.e.ent));

				if ((i = htabof(hp)) == fcode.code)  goto hfound;
				if (i == -1)                         goto out;

				p = primetab[fcode.e.c];
lookup:			hp = (hp+p)&HMASK;
				if ((i = htabof(hp)) == fcode.code)  goto hfound;
				if (i == -1)                         goto out;
				hp = (hp+p)&HMASK;
				if ((i = htabof(hp)) == fcode.code)  goto hfound;
				if (i == -1)                         goto out;
				hp = (hp+p)&HMASK;
				if ((i = htabof(hp)) == fcode.code)  goto hfound;
				if (i == -1)                         goto out;
				goto lookup;
			}
out:		;
#endif
			output(outbuf, outbits, fcode.e.ent, n_bits);

			{
				long fc;
				fc = fcode.code;
				fcode.e.ent = fcode.e.c;

				if (stcode) {
					codetabof(hp) = (unsigned short)free_ent++;
					htabof(hp) = fc;
				}
			}

			goto next;

endlop:		if (fcode.e.ent >= FIRST && rpos < uncompressed_length)
				goto next2;

			if (rpos > rlop) {
				bytes_in += rpos-rlop;
				rlop = rpos;
			}
		} while (rlop < uncompressed_length);
	/* } */

	if (bytes_in > 0)
		output(outbuf, outbits, fcode.e.ent, n_bits);

  if (((outbits+7)>>3) + bytes_out <= *compressed_length) {
    memcpy(compressed + bytes_out, outbuf, (outbits+7)>>3);
  } else {
    return COMPRESS_WRITE_ERROR;
  }

	bytes_out += (outbits+7)>>3;

  *compressed_length = bytes_out;

	return COMPRESS_OK;
}

/*
 * Decompress stdin to stdout.  This routine adapts to the codes in the
 * file building the "string" table on-the-fly; requiring no table to
 * be stored in the compressed file.  The tables used herein are shared
 * with those of the compress() routine.  See the definitions above.
 */
enum CompressStatus decompress (uint8_t* decompressed, size_t* decompressed_length,
                                const uint8_t* compressed, size_t compressed_length)
{
	char_type *stackp;
	code_int code;
	int finchar;
	code_int oldcode;
	code_int incode;
	int inbits;
	int posbits;
	size_t outpos;
	size_t insize;
	int bitmask;
	code_int free_ent;
	code_int maxcode;
	code_int maxmaxcode;
	int n_bits;
	int rsize;
  long bytes_in;
  size_t bytes_out = 0;
  size_t bytes_read = 0;
  int maxbits = BITS;
  int block_mode = BLOCK_MODE;
  char_type inbuf[IBUFSIZ+64];
  char_type outbuf[OBUFSIZ+2048];
  count_int htab[HSIZE];
  unsigned short codetab[HSIZE];

	bytes_in = 0;
	insize = 0;

  bytes_read = insize = rsize = compressed_length < IBUFSIZ ? compressed_length : IBUFSIZ;
  memcpy (inbuf, compressed, rsize);

	if (insize < 3 || inbuf[0] != MAGIC_1 || inbuf[1] != MAGIC_2) {
		if (rsize < 0)
      return COMPRESS_READ_ERROR;

		return COMPRESS_FAILED;
	}

	maxbits = inbuf[2] & BIT_MASK;
	block_mode = inbuf[2] & BLOCK_MODE;
	maxmaxcode = MAXCODE(maxbits);

	if (maxbits > BITS) {
		return COMPRESS_FAILED;
	}

	bytes_in = insize;
	maxcode = MAXCODE(n_bits = INIT_BITS)-1;
	bitmask = (1<<n_bits)-1;
	oldcode = -1;
	finchar = 0;
	outpos = 0;
	posbits = 3<<3;

	free_ent = ((block_mode) ? FIRST : 256);

	/* As above, initialize the first
	 * 256 entries in the table. */
	clear_tab_prefixof();

	for (code = 255; code >= 0; --code)
		tab_suffixof(code) = (char_type)code;

	do {
resetbuf: ;
		{
			int i, e, o;

			e = insize-(o = (posbits>>3));

			for (i = 0; i < e; ++i)
				inbuf[i] = inbuf[i+o];

			insize = e;
			posbits = 0;
		}

		if (insize < sizeof(inbuf)-IBUFSIZ) {
      const size_t bytes_remaining = compressed_length - bytes_read;
      rsize = bytes_remaining < IBUFSIZ ? bytes_remaining : IBUFSIZ;
      if (rsize != 0) {
        memcpy (inbuf+insize, compressed + bytes_read, rsize);
        bytes_read += rsize;
      }

			insize += rsize;
		}

		inbits = ((rsize > 0) ? (insize - insize%n_bits)<<3 :
		          (insize<<3)-(n_bits-1));

		while (inbits > posbits) {
			if (free_ent > maxcode) {
				posbits = ((posbits-1) + ((n_bits<<3) -
				           (posbits-1+(n_bits<<3))%(n_bits<<3)));

				++n_bits;
				if (n_bits == maxbits)
					maxcode = maxmaxcode;
				else
					maxcode = MAXCODE(n_bits)-1;

				bitmask = (1<<n_bits)-1;
				goto resetbuf;
			}

			input(inbuf, posbits, code, n_bits, bitmask);

			if (oldcode == -1) {
				if (code >= 256)
          return COMPRESS_FAILED;

				outbuf[outpos++] = (char_type)(finchar = (int)(oldcode = code));
				continue;
			}

			if (code == CLEAR && block_mode) {
				clear_tab_prefixof();
				free_ent = FIRST - 1;
				posbits = ((posbits-1) + ((n_bits<<3) -
				           (posbits-1+(n_bits<<3))%(n_bits<<3)));
				maxcode = MAXCODE(n_bits = INIT_BITS)-1;
				bitmask = (1<<n_bits)-1;
				goto resetbuf;
			}

			incode = code;
			stackp = de_stack;

			if (code >= free_ent) { /* Special case for KwKwK string.	*/
				if (code > free_ent) {
					return COMPRESS_FAILED;
				}

				*--stackp = (char_type)finchar;
				code = oldcode;
			}

			while ((cmp_code_int)code >= (cmp_code_int)256) {
				/* Generate output characters in reverse order */
				*--stackp = tab_suffixof(code);
				code = tab_prefixof(code);
			}

			*--stackp =	(char_type)(finchar = tab_suffixof(code));

			/* And put them out in forward order */
			{
				size_t i;

				if (outpos+(i = (de_stack-stackp)) >= OBUFSIZ) {
					do {
						if (i > OBUFSIZ-outpos)
							i = OBUFSIZ-outpos;

						if (i > 0) {
							memcpy(outbuf+outpos, stackp, i);
							outpos += i;
						}

						if (outpos >= OBUFSIZ) {
              if (outpos + bytes_out > *decompressed_length)
                return COMPRESS_WRITE_ERROR;

              memcpy (decompressed + bytes_out, outbuf, outpos);
              bytes_out += outpos;
							outpos = 0;
						}
						stackp+= i;
					} while ((i = (de_stack-stackp)) > 0);
				} else {
					memcpy(outbuf+outpos, stackp, i);
					outpos += i;
				}
			}

			/* Generate the new entry. */
			if ((code = free_ent) < maxmaxcode) {
				tab_prefixof(code) = (unsigned short)oldcode;
				tab_suffixof(code) = (char_type)finchar;
				free_ent = code+1;
			}

			oldcode = incode;	/* Remember previous code.	*/
		}

		bytes_in += rsize;
	} while (rsize > 0);

  if (outpos + bytes_out > *decompressed_length)
    return COMPRESS_WRITE_ERROR;

  if (outpos != 0) {
    memcpy (decompressed + bytes_out, outbuf, outpos);
    bytes_out += outpos;
    outpos = 0;
  }

  *decompressed_length = bytes_out;

  return COMPRESS_OK;
}

void prratio(FILE *stream, long int num, long int den)
{
	/* Doesn't need to be long */
	int q;

	if (den > 0) {
		if (num > 214748L)
			q = (int)(num/(den/10000L));	/* 2147483647/10000 */
		else
			q = (int)(10000L*num/den);		/* Long calculations, though */
	} else
		q = 10000;

	if (q < 0) {
		putc('-', stream);
		q = -q;
	}

	fprintf(stream, "%d.%02d%%", q / 100, q % 100);
}
