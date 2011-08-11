#ifndef FERMI_H
#define FERMI_H

#include <stdint.h>

#define FERMI_VERSION "0.0-dev (r112)"

typedef struct {
	uint64_t x[3]; // 0: start of the interval, backward; 1: forward; 2: size of the interval
} fmintv_t;

struct __rld_t; // defined in rld.h

#ifndef KSTRING_T
#define KSTRING_T kstring_t
typedef struct __kstring_t { // implemented in kstring.h
	uint32_t l, m;
	char *s;
} kstring_t;
#endif

// complement of a nucleotide
#define fm6_comp(a) ((a) >= 1 && (a) <= 4? 5 - (a) : (a))

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * Backward search for a generic FM-Index
	 *
	 * @param e       FM-Index
	 * @param len     length of the input string
	 * @param str     input string
	 * @param sa_beg  the start of the final SA interval
	 * @param sa_end  the end of the interval
	 * 
	 * @return        equal to (*sa_end - *sa_end - 1); zero if no match
	 */
	uint64_t fm_backward_search(const struct __rld_t *e, int len, const uint8_t *str, uint64_t *sa_beg, uint64_t *sa_end);

	/**
	 * Retrieve the x-th string from a generic FM-Index
	 *
	 * @param e  FM-Index
	 * @param x  string to retrieve (x >= 0)
	 * @param s  output string
	 */
	void fm_retrieve(const struct __rld_t *e, uint64_t x, kstring_t *s);

	/** Similar to {@link #fm_retrieve()} but working for DNA FM-Index only */
	void fm6_retrieve(const struct __rld_t *e, uint64_t x, kstring_t *s);

	/**
	 * Extend a string in either forward or backward direction
	 *
	 * @param e        DNA FM-Index
	 * @param ik       input SA interval
	 * @param ok       output SA intervals; one for each symbol between 0 and 5
	 * @param is_back  true is backward (right-to-left); otherwise forward (left-to-right)
	 */
	int fm6_extend(const struct __rld_t *e, const fmintv_t *ik, fmintv_t ok[6], int is_back);

	/**
	 * Find the exact match in the left-to-right direction.
	 *
	 * Matching continues if sequences in the index have minimum {min} exact matches.
	 *
	 * @param e    DNA FM-Index
	 * @param min  minimum end-to-end overlaps between sequences in the index
	 * @param len  length of the input string (DNA sequence)
	 * @param seq  input string
	 *
	 * @return     length of the maximal match
	 */
	int fm6_search_forward_overlap(const struct __rld_t *e, int min, int len, const uint8_t *seq);

	/**
	 * Merge two generic FM-Indexes with intermediate data stored in an array
	 *
	 * When {fn} is NULL, the output FM-Index is constructed in memory. {e0} and {e1} are not
	 * modified. When {fn} is a file name, the output FM-Index will be initially written to {fn}.
	 * Before the index is completely written, {e0} and {e1} will be deallocated and working space
	 * freed. And then the output index will be read back from disk and indexed for computing ranks.
	 * This procedure saves memory. {@link #fm_merge_tree()} does similar things, but it uses a
	 * B-tree rather than an array to keep intermediate data (the gap array).
	 *
	 * @param e0   first FM-Index
	 * @param e1   second FM-Index
	 * @param fn   output FM-Index file; NULL if doing everything in memory
	 *
	 * @return     output FM-Index
	 */
	struct __rld_t *fm_merge_array(struct __rld_t *e0, struct __rld_t *e1, const char *fn);

	/**
	 * Merge two generic FM-Indexes with intermediate data stored in a B-tree
	 * 
	 * This routine gives identical output to {@link @fm_merge_array()}. The difference is it
	 * uses a B-tree to store the gap array. This routine is slower, but may have a smaller
	 * memory footprint if the gap array can be well run-length compressed. Nonetheless, it
	 * may use more memory when the gap array cannot be compressed, for example, when {e0}
	 * and {e1} are identical.
	 */
	struct __rld_t *fm_merge_tree(struct __rld_t *e0, struct __rld_t *e1, const char *fn);

#ifdef __cplusplus
}
#endif

#endif