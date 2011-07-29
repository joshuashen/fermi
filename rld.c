#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include "rld.h"

static const char LogTable256[256] = {
#define LT(n) n, n, n, n, n, n, n, n, n, n, n, n, n, n, n, n
    -1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3,
    LT(4), LT(5), LT(5), LT(6), LT(6), LT(6), LT(6),
    LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7), LT(7)
};

// this table is generated by rld_gen_ddec_table() below
static int16_t ddec_table[256] = {
       0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0,   0, 249, 265, 281, 297, 313, 329, 345, 361, 377, 393, 409, 425, 441, 457, 473, 489,
     119, 119, 119, 119, 135, 135, 135, 135, 151, 151, 151, 151, 167, 167, 167, 167, 183, 183, 183, 183, 199, 199, 199, 199, 215, 215, 215, 215, 231, 231, 231, 231,
    -137,-137,-153,-153,-169,-169,-185,-185,-201,-201,-217,-217,-233,-233,-249,-249,-266,-282,-298,-314,-330,-346,-362,-378,-394,-410,-426,-442,-458,-474,-490,-506,
      85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85,  85, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101, 101,
     -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37, -37,
     -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53, -53,
     -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -70, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86, -86,
    -102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-102,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,-118,
};

static inline int ilog2(uint32_t v)
{
	register uint32_t t, tt;
	if ((tt = (v >> 16))) return (t = (tt >> 8)) ? 24 + LogTable256[t] : 16 + LogTable256[tt];
	return (t = (v >> 8)) ? 8 + LogTable256[t] : LogTable256[v];
}

inline uint32_t rld_delta_enc1(uint32_t x, int *width)
{
	int y = ilog2(x);
	int z = ilog2(y + 1);
	*width = (z<<1) + 1 + y;
	return (x^(1<<y)) | (y+1)<<y;
}

void rld_gen_ddec_table()
{
	int i, j, w;
	for (i = 1; i < 32; ++i) { // traverse gamma codes up to 9 bits
		w = (ilog2(i)<<1) + 1;
		for (j = 0; j < 1<<(9-w); ++j)
			ddec_table[i<<(9-w)|j] = (i-1)<<4 | w; // i-1 because the highest bit is removed
	}
	for (i = 2; i < 32; ++i) { // traverse delta codes up to 9 bits
		uint32_t x = rld_delta_enc1(i, &w);
		for (j = 0; j < 1<<(9-w); ++j)
			ddec_table[x<<(9-w)|j] = ~(i<<4 | w);
	}
	ddec_table[0] = 0;
	for (i = 0; i < 8; ++i) {
		printf("    ");
		for (j = 0; j < 32; ++j) printf("%4d,", ddec_table[i*32+j]);
		putchar('\n');
	}
}

inline int rld_delta_dec1(uint64_t x, int *w)
{
	if (x>>63) {
		*w = 1;
		return 1;
	} else {
		int z = ddec_table[x>>55];
		if (z > 0) {
			int a = z>>4, b = z&0xf;
			*w = a + b;
			return x<<(z&0xf)>>(64-a)|1u<<a;
		} else if (z < 0) {
			z = ~z;
			*w = z & 0xf;
			return z>>4;
		}
	}
	return 0;
}

rld_t *rld_enc_init(int asize, int bbits)
{
	rld_t *e;
	e = calloc(1, sizeof(rld_t));
	e->z = malloc(sizeof(void*));
	e->z[0] = calloc(RLD_SUPBLK_SIZE, 8);
	e->n = 1;
	e->bhead = e->head = e->z[0];
	e->p = e->head + asize;
	e->bsize = 1<<bbits;
	e->btail = e->bhead + e->bsize - 1;
	e->cnt = calloc(asize, 8);
	e->abits = ilog2(asize) + 1;
	e->r = 64;
	e->asize = asize;
	e->bbits = bbits;
	return e;
}

int rld_push(rld_t *e, int l, uint8_t c)
{
	int i, w;
	uint64_t x = rld_delta_enc1(l, &w) << e->abits | c;
	w += e->abits;
	if (w > e->r) {
		if (e->p == e->btail) { // jump to the next block
			for (i = 0; i < e->asize; ++i) e->bhead[i] = e->cnt[i];
			if (e->p + 1 - e->head == RLD_SUPBLK_SIZE) { // allocate a new superblock
				++e->n;
				e->z = realloc(e->z, e->n * sizeof(void*));
				e->p = e->head = e->bhead = e->z[e->n - 1] = calloc(RLD_SUPBLK_SIZE, 8);
			} else e->bhead += e->bsize;
			e->btail = e->bhead + e->bsize - 1;
			e->p = e->bhead + e->asize;
			e->r = 64 - w;
			*e->p |= x << e->r;
		} else {
			w -= e->r;
			*e->p++ |= x >> w;
			*e->p = x << (e->r = 64 - w);
		}
	} else e->r -= w, *e->p |= x << e->r;
	++e->cnt[c];
	return 0;
}

uint64_t rld_finish(rld_t *e)
{
	int i;
	for (i = 0; i < e->asize; ++i) e->bhead[i] = e->cnt[i];
	return (((uint64_t)(e->n - 1) * RLD_SUPBLK_SIZE) + (e->p - e->head)) * 64 + (64 - e->r);
}

inline uint32_t rld_pullb(rld_t *e)
{
	int y, w = 0;
	uint64_t x;
	x = e->p[0] << (64 - e->r) | (e->p < e->btail && e->r < 64? e->p[1] >> e->r : 0);
	y = rld_delta_dec1(x, &w);
	y = y << e->abits | (x << w >> (64 - e->abits));
	w += e->abits;
	if (e->r > w) e->r -= w;
	else ++e->p, e->r = 64 + e->r - w;
	return y;
}
/*
int main(int argc, char *argv[])
{
	//rld_gen_ddec_table();
	if (argc > 1) {
		int w, x, z, ww;
		uint64_t y;
		x = rld_delta_enc1(atoi(argv[1]), &w);
		y = (uint64_t)x << (64 - w);
		z = rld_delta_dec1(y, &ww);
		printf("%d==%d, %d==%d\n", z, atoi(argv[1]), w, ww);
	}
	int i, j = 0;
	rld_t *e = rld_enc_init(6, 5);
	for (i = 1; i < 5550; ++i) rld_push(e, i, 0);
	rld_finish(e);
	rld_dec_initb(e, j);
	for (i = 1; i < 5550; ++i) {
		int y = rld_pullb(e);
		if (y == 0) {
			j += e->bsize;
			rld_dec_initb(e, j);
			y = rld_pullb(e);
		}
		printf("%d\t%d\n", i, y>>3);
	}
	return 0;
}
*/
