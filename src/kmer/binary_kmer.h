#ifndef BINARY_KMER_H_
#define BINARY_KMER_H_

#include "dna.h"

// NUM_BKMER_WORDS is the number of 64 bit words we use to encode a kmer

typedef struct {
  uint64_t b[NUM_BKMER_WORDS];
} BinaryKmer;

#define BKMER_BYTES (NUM_BKMER_WORDS * sizeof(uint64_t))

// BinaryKmer that is all zeros
extern const BinaryKmer zero_bkmer;

#define BINARY_KMER_ZERO_MACRO {.b = {0}}

// Hash functions
#ifdef CITY_HASH
  // Use Google's CityHash
  #include "city.h"
  #define binary_kmer_hash(bkmer,rehash) (CityHash32((char*)bkmer.b, BKMER_BYTES) ^ rehash)
#else
  // Use Bob Jenkin's lookup3
  #include "lookup3.h"
  #define binary_kmer_hash(bkmer,rehash) hashlittle(bkmer.b, BKMER_BYTES, rehash)
#endif


// Since kmer_size is always odd, top word always has <= 62 bits used
// Number of bases store in all but the top word
#define BKMER_LOWER_BASES              ((NUM_BKMER_WORDS-1)*32)
#define BKMER_TOP_BASES(ksize)         ((ksize)&31)
#define BKMER_TOP_BITS(ksize)          (BKMER_TOP_BASES(ksize) * 2)
#define BKMER_TOP_BP_BYTEOFFSET(ksize) (BKMER_TOP_BITS(ksize) - 2)

#define binary_kmer_init(x)          memset((x)->b, 0, BKMER_BYTES)
#define binary_kmers_cmp(x,y)        memcmp((x).b,(x).b,BKMER_BYTES)

#define binary_kmer_first_nuc(bkmer,ksize) \
        (((bkmer).b[0] >> BKMER_TOP_BP_BYTEOFFSET(ksize)) & 0x3)
#define binary_kmer_last_nuc(bkmer)  ((bkmer).b[NUM_BKMER_WORDS - 1] & 0x3)

#define binary_kmer_set_first_nuc(bkmer,nuc,ksize)                             \
        ((bkmer)->b[0] = ((bkmer)->b[0] &                                      \
                          (~(uint64_t)0 >> (64-BKMER_TOP_BP_BYTEOFFSET(ksize)))) |\
                         (((uint64_t)(nuc)) << BKMER_TOP_BP_BYTEOFFSET(ksize)))

#define binary_kmer_set_last_nuc(bkmer,nuc) \
        ((bkmer)->b[NUM_BKMER_WORDS - 1] \
           = ((bkmer)->b[NUM_BKMER_WORDS - 1] & 0xfffffffffffffffcUL) | (nuc))

#if NUM_BKMER_WORDS == 1
  #define binary_kmers_are_equal(x,y) ((x).b[0] == (y).b[0])
  #define binary_kmer_is_zero(x)      ((x).b[0] == 0UL)
  #define binary_kmer_less_than(x,y)  ((x).b[0] < (y).b[0])
#elif NUM_BKMER_WORDS == 2
  #define binary_kmers_are_equal(x,y) ((x).b[0]==(y).b[0] && (x).b[1]==(y).b[1])
  #define binary_kmer_is_zero(x)      (((x).b[0] | (x).b[1]) == 0UL)
  #define binary_kmer_less_than(x,y) \
          ((x).b[0] < (y).b[0] || ((x).b[0] == (y).b[0] && (x).b[1] < (y).b[1]))
#else /* NUM_BKMER_WORDS > 2 */
  #define binary_kmers_are_equal(x,y) (binary_kmers_cmp(&(x),&(y)) == 0)
  #define binary_kmer_is_zero(x)      binary_kmers_are_equal((x), zero_bkmer)
  boolean binary_kmer_less_than(BinaryKmer left, BinaryKmer right);
#endif

#define binary_kmer_oversized(bk,k)  ((bk).b[0] & ~(uint64_t)0<<BKMER_TOP_BITS(k))

//
// Functions
//

// Shift towards most significant position
BinaryKmer binary_kmer_right_shift_one_base(const BinaryKmer bkmer);

// Shift towards least significant position
BinaryKmer binary_kmer_left_shift_one_base(const BinaryKmer bkmer,
                                           size_t kmer_size);

BinaryKmer binary_kmer_left_shift_add(const BinaryKmer bkmer, size_t kmer_size,
                                      Nucleotide nuc);

BinaryKmer binary_kmer_right_shift_add(const BinaryKmer bkmer, size_t kmer_size,
                                       Nucleotide nuc);

// Reverse complement a binary kmer from kmer into revcmp_kmer
BinaryKmer binary_kmer_reverse_complement(const BinaryKmer bkmer, size_t kmer_size);

// Get a random binary kmer -- useful for testing
BinaryKmer binary_kmer_random(size_t kmer_size);

// BinaryKmer <-> String functions
char* binary_kmer_to_str(const BinaryKmer kmer, size_t kmer_size, char *seq);
BinaryKmer binary_kmer_from_str(const char *seq, size_t kmer_size);

void binary_kmer_to_hex(const BinaryKmer bkmer, size_t kmer_size, char *seq);

void binary_nuc_from_str(Nucleotide *bases, const char *str, size_t len);
void binary_nuc_to_str(const Nucleotide *bases, char *str, size_t len);

#endif /* BINARY_KMER_H_ */
