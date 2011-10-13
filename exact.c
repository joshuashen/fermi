#include <assert.h>
#include "rld.h"
#include "kstring.h"
#include "fermi.h"
#include "kvec.h"

uint64_t fm_backward_search(const rld_t *e, int len, const uint8_t *str, uint64_t *sa_beg, uint64_t *sa_end)
{
	uint64_t k, l, ok, ol;
	int i, c;
	c = str[len - 1];
	k = e->cnt[c]; l = e->cnt[c + 1] - 1;
	for (i = len - 2; i >= 0; --i) {
		c = str[i];
		rld_rank21(e, k - 1, l, c, &ok, &ol);
		k = e->cnt[c] + ok;
		l = e->cnt[c] + ol - 1;
		if (k > l) break;
	}
	if (k > l) return 0;
	*sa_beg = k; *sa_end = l;
	return l - k + 1;
}

int64_t fm_retrieve(const rld_t *e, uint64_t x, kstring_t *s)
{
	uint64_t k = x, *ok;
	ok = alloca(8 * e->asize);
	s->l = 0;
	while (1) {
		int c = rld_rank1a(e, k, ok);
		k = e->cnt[c] + ok[c] - 1;
		if (c == 0) return k;
		kputc(c, s);
	}
}

int fm6_extend(const rld_t *e, const fmintv_t *ik, fmintv_t ok[6], int is_back)
{ // FIXME: this can be accelerated a little by using rld_rank1a() when ik.x[2]==1
	uint64_t tk[6], tl[6];
	int i;
	rld_rank2a(e, ik->x[!is_back] - 1, ik->x[!is_back] - 1 + ik->x[2], tk, tl);
	for (i = 0; i < 6; ++i) {
		ok[i].x[!is_back] = e->cnt[i] + tk[i];
		ok[i].x[2] = (tl[i] -= tk[i]);
	}
	ok[0].x[is_back] = ik->x[is_back];
	ok[4].x[is_back] = ok[0].x[is_back] + tl[0];
	ok[3].x[is_back] = ok[4].x[is_back] + tl[4];
	ok[2].x[is_back] = ok[3].x[is_back] + tl[3];
	ok[1].x[is_back] = ok[2].x[is_back] + tl[2];
	ok[5].x[is_back] = ok[1].x[is_back] + tl[1];
	return 0;
}

int fm6_extend0(const rld_t *e, const fmintv_t *ik, fmintv_t *ok0, int is_back)
{ // FIXME: this can be accelerated a little by using rld_rank1a() when ik.x[2]==1
	uint64_t tk[6], tl[6];
	rld_rank2a(e, ik->x[!is_back] - 1, ik->x[!is_back] - 1 + ik->x[2], tk, tl);
	ok0->x[!is_back] = tk[0];
	ok0->x[is_back] = ik->x[is_back];
	ok0->x[2] = tl[0] - tk[0];
	return 0;
}

uint64_t fm6_retrieve(const rld_t *e, uint64_t x, kstring_t *s, fmintv_t *k2, int *contained)
{
	uint64_t k = x, ok[6];
	fmintv_t ok2[6];
	s->l = 0; *contained = 0;
	while (1) {
		int c = rld_rank1a(e, k, ok);
		k = e->cnt[c] + ok[c] - 1;
		if (c == 0) break;
		if (s->l > 0) {
			if (k2->x[2] == 1) k2->x[0] = k;
			else {
				fm6_extend(e, k2, ok2, 1);
				*k2 = ok2[c];
			}
		} else fm6_set_intv(e, c, *k2);
		kputc(c, s);
	}
	if (k2->x[2] != 1) {
		fm6_extend(e, k2, ok2, 1);
		if (ok2[0].x[2] != k2->x[2]) *contained |= 1; // left contained
		*k2 = ok2[0];
	} else k2->x[0] = k;
	fm6_extend(e, k2, ok2, 0);
	if (ok2[0].x[2] != k2->x[2]) *contained |= 2; // right contained
	*k2 = ok2[0];
	return k;
}

void fm_reverse_fmivec(fmintv_v *p)
{
	if (p->n > 1) {
		int j;
		for (j = 0; j < p->n>>1; ++j) {
			fmintv_t tmp = p->a[p->n - 1 - j];
			p->a[p->n - 1 - j] = p->a[j];
			p->a[j] = tmp;
		}
	}
}
