/* -*- Mode: C++; c-basic-offset: 4; indent-tabs-mode: nil -*- */
#ifndef _FASTRANK_POPCOUNT_H_
#define _FASTRANK_POPCOUNT_H_

#include <sys/types.h>
#include <stdio.h>
#include <stdint.h>

#define L8 0x0101010101010101ULL // Every lowest 8th bit set: 00000001...
#define G2 0xAAAAAAAAAAAAAAAAULL // Every highest 2nd bit: 101010...
#define G4 0x3333333333333333ULL // 00110011 ... used to group the sum of 4 bits.
#define G8 0x0F0F0F0F0F0F0F0FULL // 0000111100001111...group the sum of 8
#define H8 0x8080808080808080ULL // 1000 0000 1000000010000000...
#define L9 0x0040201008040201ULL // 00000 0000 1 00000000 1 000000001 除第一个为9个0,其余为8个0+1个1
#define H9 (L9 << 8) //010000 0000 10000 0000 100000000100000000
#define L16 0x0001000100010001ULL //0000 0000 0000 0001 0000000000000001 每16位最低一位是1
#define H16 0x8000800080008000ULL //1000 0000 0000 0000 1000000000000000 每16位最高一位是1

#define ONES_STEP_4 ( 0x1111111111111111ULL ) //0001 0001 0001 00010001 ->ONES_STEP_4 每4位最低一位是1
#define ONES_STEP_8 ( 0x0101010101010101ULL ) //0000 0001 0000 0001 0000000100000001 -> 每8位最低一位是1
#define ONES_STEP_9 ( 1ULL << 0 | 1ULL << 9 | 1ULL << 18 | 1ULL << 27 | 1ULL << 36 | 1ULL << 45 | 1ULL << 54 )
                    //0000000001 0000 0000 1 0000 0000 1 ->每9位最低一位为1（最前面多了一个0）
#define ONES_STEP_16 ( 1ULL << 0 | 1ULL << 16 | 1ULL << 32 | 1ULL << 48 ) //0000 0000 0000 0001-> 每16位最低一位是1
#define MSBS_STEP_4 ( 0x8ULL * ONES_STEP_4 ) //1000 1000 1000 10001000 ->每4位最高一位是1
#define MSBS_STEP_8 ( 0x80ULL * ONES_STEP_8 ) //1000 0000 1000000010000000 ->每8位最高一位是1
#define MSBS_STEP_9 ( 0x100ULL * ONES_STEP_9 ) //1 0000 0000 1 0000 0000 100000000 ->每9位最高一位是1
#define MSBS_STEP_16 ( 0x8000ULL * ONES_STEP_16 ) //1000 0000 0000 0000 1000000000000000 ->每16位最高一位是1
#define INCR_STEP_8 ( 0x80ULL << 56 | 0x40ULL << 48 | 0x20ULL << 40 | 0x10ULL << 32 | 0x8ULL << 24 | 0x4ULL << 16 | 0x2ULL << 8 | 0x1 )
                    //100000000100000000100000000100000000 1 0000 0000 1 0000 0000 1 0000 0000 1 ->每9位最低一位是1
#define ONES_STEP_32 ( 0x0000000100000001ULL ) //0000 0000 0000 0000 0000 0000 0000 0001 -> 每32位最低一位是1
#define MSBS_STEP_32 ( 0x8000000080000000ULL ) //1000 0000 0000 0000 0000 0000 0000 0000 -> 每32位最高一位是1
	
#define COMPARE_STEP_8(x,y) ( ( ( ( ( (x) | MSBS_STEP_8 ) - ( (y) & ~MSBS_STEP_8 ) ) ^ (x) ^ ~(y) ) & MSBS_STEP_8 ) >> 7 )
                    //每隔8位比较大小，比如COMPARE_STEP_8(1111113,4562413218),x大为0,y大为1
                    /*
                        00000000 00000000 00010000 11110100 01001001
                        00000001 00001111 11110000 11100110 10100010
                        00000001 00000001 00000000 00000000 00000001

                    */
#define LEQ_STEP_8(x,y) ( ( ( ( ( (y) | MSBS_STEP_8 ) - ( (x) & ~MSBS_STEP_8 ) ) ^ (x) ^ (y) ) & MSBS_STEP_8 ) >> 7 )
                    //每隔8位比较大小，比如 LEQ_STEP_8(1111113,4562413218),x大为0,y大为1,但结尾默认是1
                    /*
                        00000000 00000000 00000000 00010000 11110100 01001001
                        00000000 00000001 00001111 11110000 11100110 10100010
                        00000001 00000001 00000001 00000001 00000000 00000001

                    */
#define UCOMPARE_STEP_9(x,y) ( ( ( ( ( ( (x) | MSBS_STEP_9 ) - ( (y) & ~MSBS_STEP_9 ) ) | ( x ^ y ) ) ^ ( x | ~y ) ) & MSBS_STEP_9 ) >> 8 )
                    //每隔9位比较大小，比如 UCOMPARE_STEP_9(1111113,4562413218),x大为0,y大为1
                    /*
                        00000000 0 00000010 0 00111101 0 00100100 1
                        00010000 1 11111110 0 00111001 1 01010001 0
                        00000000 1 00000000 1 00000000 0 00000000 1                    
                    */
#define UCOMPARE_STEP_16(x,y) ( ( ( ( ( ( (x) | MSBS_STEP_16 ) - ( (y) & ~MSBS_STEP_16 ) ) | ( x ^ y ) ) ^ ( x | ~y ) ) & MSBS_STEP_16 ) >> 15 )
#define ULEQ_STEP_9(x,y) ( ( ( ( ( ( (y) | MSBS_STEP_9 ) - ( (x) & ~MSBS_STEP_9 ) ) | ( x ^ y ) ) ^ ( x & ~y ) ) & MSBS_STEP_9 ) >> 8 )
#define ULEQ_STEP_16(x,y) ( ( ( ( ( ( (y) | MSBS_STEP_16 ) - ( (x) & ~MSBS_STEP_16 ) ) | ( x ^ y ) ) ^ ( x & ~y ) ) & MSBS_STEP_16 ) >> 15 )
                    //每隔16位比较大小，比如 ULEQ_STEP_16(1111113,4562413218),x大为0,y大为1,但结尾默认是1
                    /*
                        00000000 00000000 00000000 00010000 11110100 01001001
                        00000000 00000001 00001111 11110000 11100110 10100010
                        00000000 00000001 00000000 00000001 00000000 00000000

                    */
#define ZCOMPARE_STEP_8(x) ( ( ( x | ( ( x | MSBS_STEP_8 ) - ONES_STEP_8 ) ) & MSBS_STEP_8 ) >> 7 )
                    //每隔8位判断是否有位为1，比如ZCOMPARE_STEP_8(4562413218),若当前8bit不全为0则置1
                    /*
                        00000000 00000001 00001111 11110000 11100110 10100010
                        00000000 00000001 00000001 00000001 00000001 00000001

                    */
// Population count of a 64 bit integer in SWAR (SIMD within a register) style
// From Sebastiano Vigna, "Broadword Implementation of Rank/Select Queries"
// http://sux.dsi.unimi.it/paper.pdf p4
// This variant uses multiplication for the last summation instead of
// continuing the shift/mask/addition chain.

inline int suxpopcount(uint64_t x) {
    // Step 1:  00 - 00 = 0;  01 - 00 = 01; 10 - 01 = 01; 11 - 01 = 10;
    x = x - ((x & G2) >> 1);
    // step 2:  add 2 groups of 2.
    x = (x & G4) + ((x >> 2) & G4);
    // 2 groups of 4.
    x = (x + (x >> 4)) & G8;
    // Using a multiply to collect the 8 groups of 8 together.
    x = x * L8 >> 56;
    return x;
}
/*
    suxpopcount的作用是每8位计算这8位里有多少个1,例如suxpopcount(2413218):
    2413218: 00000000 000100100 11010010 10100010
     result: 00000000 000000010 00000100 00000011
*/

// Default to using the GCC builtin popcount.  On architectures
// with -march popcnt, this compiles to a single popcnt instruction.
#ifndef popcount
#define popcount __builtin_popcountll //__builtin_popcount是GCC的内建函数，它可以精确的计算1的个数
//#define popcount suxpopcount
#endif

#define popcountsize 64ULL
#define popcountmask (popcountsize - 1)

inline uint64_t popcountLinear(uint64_t *bits, uint64_t x, uint64_t nbits) { //计算bits从x开始的nbits里有多少个1
    if (nbits == 0) { return 0; }
    uint64_t lastword = (nbits - 1) / popcountsize; //一共有多少个完整的64bit块
    uint64_t p = 0;

    __builtin_prefetch(bits + x + 7, 0); //数据预取，参数1为地址，指向要预取得的数据，参数2表示读（若为1表示写）
    for (uint64_t i = 0; i < lastword; i++) { /* tested;  manually unrolling doesn't help, at least in C */
        //__builtin_prefetch(bits + x + i + 3, 0);
        p += popcount(bits[x+i]); // 实际上是通过__builtin_popcountll实现的
    }

    // 'nbits' may or may not fall on a multiple of 64 boundary,
    // so we may need to zero out the right side of the last word
    // (accomplished by shifting it right, since we're just popcounting)
    uint64_t lastshifted = bits[x+lastword] >> (63 - ((nbits - 1) & popcountmask));
    p += popcount(lastshifted); ////加上最后一个不完整块里的1数量
    return p;
}

// 从0位置开始计算第k个1的位置在哪里，找到返回位置，没找到返回-1
inline int select64_naive(uint64_t x, int k) {
    int count = -1;
    for (int i = 63; i >= 0; i--) {
        count++;
        if (x & (1ULL << i)) {
            k--;
            if (k == 0) {
                return count;
            }
        }
    }
    return -1;
}
//从0位置开始计算第k个1的位置在哪里，找到返回位置，没找到返回大于63的值
/*
    例如
    113 = 56个0 + 01110001
    select64_popcount_search(113,1) = 57
    select64_popcount_search(113,4) = 63
    select64_popcount_search(113,10)= 69
*/
inline int select64_popcount_search(uint64_t x, int k) {
    int loc = -1;
    // if (k > popcount(x)) { return -1; }

    for (int testbits = 32; testbits > 0; testbits >>= 1) {
        int lcount = popcount(x >> testbits);
        if (k > lcount) {
            x &= ((1ULL << testbits)-1);
            loc += testbits;
            k -= lcount;
        } else {
            x >>= testbits;
        }
    }
    return loc+k;
}

inline int select64_broadword(uint64_t x, int k) {
    uint64_t word = x;
    int residual = k;
    register uint64_t byte_sums;
    
    byte_sums = word - ( ( word & 0xa * ONES_STEP_4 ) >> 1 );
    byte_sums = ( byte_sums & 3 * ONES_STEP_4 ) + ( ( byte_sums >> 2 ) & 3 * ONES_STEP_4 );
    byte_sums = ( byte_sums + ( byte_sums >> 4 ) ) & 0x0f * ONES_STEP_8;
    byte_sums *= ONES_STEP_8;
    
    // Phase 2: compare each byte sum with the residual
    const uint64_t residual_step_8 = residual * ONES_STEP_8;
    const int place = ( LEQ_STEP_8( byte_sums, residual_step_8 ) * ONES_STEP_8 >> 53 ) & ~0x7;
    
    // Phase 3: Locate the relevant byte and make 8 copies with incremental masks
    const int byte_rank = residual - ( ( ( byte_sums << 8 ) >> place ) & 0xFF );
    
    const uint64_t spread_bits = ( word >> place & 0xFF ) * ONES_STEP_8 & INCR_STEP_8;
    const uint64_t bit_sums = ZCOMPARE_STEP_8( spread_bits ) * ONES_STEP_8;
    
    // Compute the inside-byte location and return the sum
    const uint64_t byte_rank_step_8 = byte_rank * ONES_STEP_8;
    
    return place + ( LEQ_STEP_8( bit_sums, byte_rank_step_8 ) * ONES_STEP_8 >> 56 );   
}
//从0位置开始计算第k个1的位置在哪里，找到返回位置，没找到返回大于63的值
inline int select64(uint64_t x, int k) {
    return select64_popcount_search(x, k);
}

// x is the starting offset of the 512 bits;
// k is the thing we're selecting for.
//计算地址为bits的uint64_t数组内，从第x开始的512位，第k个1在哪
inline int select512(uint64_t *bits, int x, int k) {
    __asm__ __volatile__ (
                          "prefetchnta (%0)\n"
                          : : "r" (&bits[x]) );
    int i = 0;
    int pop = popcount(bits[x+i]);
    while (k > pop && i < 7) {
        k -= pop;
        i++;
        pop = popcount(bits[x+i]);
    }
    if (i == 7 && popcount(bits[x+i]) < k) {
        return -1;
    }
    // We're now certain that the bit we want is stored in bv[x+i]
    return i*64 + select64(bits[x+i], k);
}

// brute-force linear select
// x is the starting offset of the bits in bv;
// k is the thing we're selecting for (starting from bv[x]).
// bvlen is the total length of bv
// 计算地址为bits的uint64_t数组内，从第x开始的length位，第k个1在哪
inline uint64_t selectLinear(uint64_t* bits, uint64_t length, uint64_t x, uint64_t k) {
    if (k > (length - x) * 64)
        return -1;
    uint64_t i = 0;
    uint64_t pop = popcount(bits[x+i]);
    while (k > pop && i < (length - 1)) {
        k -= pop;
        i++;
        pop = popcount(bits[x+i]);
    }
    if ((i == length - 1) && (pop < k)) {
        return -1;
    }
    // We're now certain that the bit we want is stored in bits[x+i]
    return i*64 + select64(bits[x+i], k);
}


#endif /* _FASTRANK_POPCOUNT_H_ */
