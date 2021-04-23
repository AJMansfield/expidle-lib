
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <stddef.h>
#include <stdalign.h>
#include <math.h>
#include <immintrin.h>

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#  define GNUC_PREREQ(minMajor, minMinor) \
         ((__GNUC__ << 16) + __GNUC_MINOR__ >= ((minMajor) << 16) + (minMinor))
#else
#  define GNUC_PREREQ 0
#endif

#if GNUC_PREREQ(4, 0)
#  define OFFSETOF(type, member) ((int)__builtin_offsetof(type, member))
#else
#  define OFFSETOF(type, member) ((int)(intptr_t)&(((type *)(void*)0)->member) )
#endif

#define min(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a < _b ? _a : _b; })
#define max(a,b) \
   ({ __typeof__ (a) _a = (a); \
       __typeof__ (b) _b = (b); \
     _a > _b ? _a : _b; })  

#define COUNT 128
#define VEC_LEN 8
#define VEC_CNT COUNT / VEC_LEN 

#define HUGENUM_X_MAX 1e6
#define HUGENUM_X_MIN 6.0
#define HUGENUM_E_MAX 255
#define HUGENUM_E_MIN 0

typedef struct {
    double x;
    int_fast8_t e;
    _Bool p;
    _Bool z;
} hugenum;

typedef struct {
    __m512d x; // mantissa
    __m256i e; // exponent
    __mmask8 p; // positive
    __mmask8 z; // exists
} hugenum8;

hugenum8 mk_hugenum8(hugenum buf[], size_t count) {
    hugenum8 r;
    int n = min(count, VEC_LEN);
    for (int i = 0; i < VEC_LEN; ++i) {
        if (i < count) {
            ((double*) &r.x)[i] = buf[i].x;
            ((int*) &r.e)[i] = buf[i].e;
            r.p = (r.p & ~(1<<i)) | (buf[i].p << i);
            r.z = (r.z & ~(1<<i)) | (buf[i].z << i);
        } else {
            ((double*) &r.x)[i] = 0.0;
            ((int*) &r.e)[i] = 0;
            r.p |= 1 << i;
            r.z &= ~(1 << i);
        }
    }

    return r;
}
void un_hugenum8(hugenum buf[VEC_LEN], hugenum8 r) {
    for (size_t i = 0; i < VEC_LEN; ++i) {
        buf[i].x = ((double*) &r.x)[i];
        buf[i].e = ((int*) &r.e)[i];
        buf[i].p = (r.p & (1<<i))>>i;
        buf[i].z = (r.z & (1<<i))>>i;
    }
}

size_t load_vec(hugenum8 data[]){
    hugenum buf[VEC_LEN];
    size_t cnt = 0;
    size_t idx = 0;
    int scan_result;

    do {
        // printf("col %d\n", idx);
        scan_result = scanf("%lg,%d,%d\n",
            &buf[idx % VEC_LEN].x,
            &buf[idx % VEC_LEN].e,
            &buf[idx % VEC_LEN].p
        );
        buf[idx % VEC_LEN].z = 1;
        if (scan_result < 0 || scan_result == EOF) break;
        if (++idx % VEC_LEN == 0) {
            printf("full row\n");
            data[cnt++] = mk_hugenum8(buf, 8);
        }
    } while (1);
    if (idx % VEC_LEN != 0) {
        printf("partial row %d\n", idx % VEC_LEN);
        data[cnt++] = mk_hugenum8(buf, idx % VEC_LEN);
    }
    return cnt;
}

void print_vec(hugenum8 data[], size_t count){
    hugenum buf[VEC_LEN];
    
    for (size_t i = 0; i < count; ++i) {
        un_hugenum8(buf, data[i]);
        for (size_t j = 0; j < VEC_LEN; ++j) {
            if (buf[j].z){
                printf("%g,%d,%d\n",
                    buf[j].x,
                    buf[j].e,
                    buf[j].p
                );
            }
        }
    }
}
void print_hn(hugenum8 data){
    hugenum buf[VEC_LEN];
    un_hugenum8(buf, data);
    for (size_t j = 0; j < VEC_LEN; ++j) {
        if (buf[j].z){
            printf("%2g ",
                buf[j].x //,
                // buf[j].e,
                // buf[j].p
            );
        } else {
            printf(" ---- ");
        }
    }
    printf("\n");
}
void print_pd(char* f, __m512d data){
    for (size_t j = 0; j < VEC_LEN; ++j) {
        printf(f, ((double*) &data)[j]);
    }
    printf("\n");
}


__mmask8 mask_blend_mask(__mmask8 k, __mmask8 a, __mmask8 b){
    return (~k & a) | (k & b);
}
hugenum8 mask_blend_hugenum8(__mmask8 k, hugenum8 a, hugenum8 b){
    hugenum8 r;
    r.x = _mm512_mask_blend_pd(k, a.x, b.x);
    r.e = _mm256_mask_blend_epi32(k, a.e, b.e);
    r.p = mask_blend_mask(k, a.p, b.p);
    r.z = mask_blend_mask(k, a.z, b.z);
    return r;
}

__mmask8 mag_less(hugenum8 q1, hugenum8 q2)
{
    __mmask8 eeq = _mm256_cmp_epi32_mask(q1.e, q2.e, _MM_CMPINT_EQ);
    __mmask8 elt = _mm256_cmp_epi32_mask(q1.e, q2.e, _MM_CMPINT_LT);
    __mmask8 xlt = _mm512_cmp_pd_mask(q1.x, q2.x, _CMP_LT_OQ);
    __mmask8 q1nq2 = (q1.z & ~q2.z); // q1 is the only one that exists
    __mmask8 q2nq1 = (q2.z & ~q1.z); // q2 is the only one that exists

    return ~q1nq2 & (
        q2nq1 |
        (~eeq & elt) |
        (eeq & xlt)
    );
}

void order_by_mag(hugenum8 *q1, hugenum8 *q2) {
    __mmask8 swp = mag_less(*q1, *q2);
    hugenum8 temp = *q2;
    *q2 = mask_blend_hugenum8(swp, *q2, *q1);
    *q1 = mask_blend_hugenum8(swp, *q1, temp);
}

hugenum8 normalize(hugenum8 q) {
    const __m512d X_MAX = _mm512_set1_pd(HUGENUM_X_MAX);
    const __m512d X_MIN = _mm512_set1_pd(HUGENUM_X_MIN);
    const __m256i E_MAX = _mm256_set1_epi32(HUGENUM_E_MAX);
    const __m256i E_MIN = _mm256_set1_epi32(HUGENUM_E_MIN);
    const __m256i ONE = _mm256_set1_epi32(1);

    // for (int i = 0; i < 20; i++) {
    do {
        __mmask8 xgt = _mm512_mask_cmp_pd_mask(q.z, q.x, X_MAX, _CMP_GE_OQ);
        // printf("dc = %x\n", xgt);
        if (xgt == 0) break;
        q.x = _mm512_mask_log10_pd(q.x, xgt, q.x);
        q.e = _mm256_mask_add_epi32(q.e, xgt, q.e, ONE);
    } while (1);
    // for (int i = 0; i < 20; i++) {
    do {
        __mmask8 xlt = _mm512_mask_cmp_pd_mask(q.z, q.x, X_MIN, _CMP_LT_OQ);
        __mmask8 egt = _mm256_mask_cmp_epi32_mask(q.z, q.e, E_MIN, _MM_CMPINT_NLE);
        __mmask8 cond = xlt & egt;
        // printf("xlt, egt = %x, %x\n", xlt, egt);
        if (cond == 0) break;
        q.x = _mm512_mask_exp10_pd(q.x, cond, q.x);
        q.e = _mm256_mask_sub_epi32(q.e, cond, q.e, ONE);
    } while (1);
    
    return q;
}

hugenum8 add(hugenum8 q1, hugenum8 q2) {
    const __m512d ONE = _mm512_set1_pd(1.0);
    const __m512d ZERO = _mm512_set1_pd(0.0);
    const __m512d NEG_ONE = _mm512_set1_pd(-1.0);
    const __m256i E_ZERO = _mm256_set1_epi32(0);
    const __m256i E_ONE = _mm256_set1_epi32(1);
    
    order_by_mag(&q1,&q2);

    // print_hn(q1);
    // print_hn(q2);

    __mmask8 z = q1.z & q2.z;

    // printf("z = %02x\n", z);
    
    __mmask8 q1e0 = _mm256_mask_cmp_epi32_mask(z, q1.e, E_ZERO, _MM_CMPINT_EQ);
    __mmask8 q1e1 = _mm256_mask_cmp_epi32_mask(z, q1.e, E_ONE, _MM_CMPINT_EQ);
    __mmask8 q2e0 = _mm256_mask_cmp_epi32_mask(z, q2.e, E_ZERO, _MM_CMPINT_EQ);

    // printf("q1e0 = %02x, q1e1 = %02x, q2e0 = %02x,\n", q1e0, q1e1, q2e0);

    __m512d sign = _mm512_mask_blend_pd(q1.p ^ q2.p, ONE, NEG_ONE);

    // print_pd("%2g ", sign);

    __m512d dx = ZERO; // q1e > 1 case

    // print_pd("%2g ", dx);

    dx = _mm512_mask_mul_pd(dx, z & q1e0, sign, q2.x); // q1.e = 0 case

    // print_pd("%2g ", dx);

    __m512d q2x = _mm512_mask_log10_pd(q2.x, z & q2e0 & q1e1, q2.x);
    dx = _mm512_mask_sub_pd(dx, z & q1e1, q2x, q1.x); // q1.e = 1 case
    dx = _mm512_mask_exp10_pd(dx, z & q1e1, dx);
    dx = _mm512_mask_fmadd_pd(dx, z & q1e1, sign, ONE);
    dx = _mm512_mask_log10_pd(dx, z & q1e1, dx);

    // print_pd("%2g ", dx);
    
    __mmask8 nanz = _mm512_mask_fpclass_pd_mask(z, dx, 0x81); // SNAN and QNAN
    
    // printf("nans = %02x\n", nanz);

    q1.x = _mm512_mask_add_pd(q1.x, ~nanz, q1.x, dx);

    // print_pd("%2g ", q1.x);

    return q1;
}

int main(int argc, char** argv) {

    hugenum8 data[VEC_CNT];

    size_t n = load_vec(data);

    for (size_t i = 0; i < n; ++i){
        data[i] = normalize(data[i]);
    }
    
    print_vec(data, n);

    // print_hn(data[0]);
    // order_by_mag(&data[0], &data[1]);
    // print_hn(data[0]);
    // printf("\n");

    data[2] = add(data[0], data[1]);
    ++n;

    print_vec(data, n);


    // printf("%d,%d\nx=%d,%d\ne=%d,%d\np=%d,%d\nz=%d,%d",
    //     sizeof(hugenum8), _Alignof(hugenum8),
    //     OFFSETOF(hugenum8, x), sizeof(__m512d),
    //     OFFSETOF(hugenum8, e), sizeof(__m256i),
    //     OFFSETOF(hugenum8, p), sizeof(__mmask8),
    //     OFFSETOF(hugenum8, z), sizeof(__mmask8)
    //     );

    return 0;
}



