#ifndef __HUGENUM_H__
#define __HUGENUM_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

typedef double hugenum_x_t;
#define HUGENUM_X_MAX 1e6
#define HUGENUM_X_MIN 6

typedef uint_fast8_t hugenum_e_t;
#define HUGENUM_E_MAX UINT_FAST8_MAX
#define HUGENUM_E_MIN 0

typedef struct {
    hugenum_x_t x; // the tetration "mantissa"
    hugenum_e_t e; // number of ee's; the tetration coefficient
    bool p; // "is positive"
    /* the numerical value of an instance of this struct is:
     * hugenum(x, 0, p) = p?+1:-1 * abs(x)
     * hugenum(x, e, p) = p?+1:-1 * 10**(x, e-1, True)
     * hugenum(x=inf, e=-1, p) = p?+1:-1 * inf
     * Values larger than this type can represent 
     */

} hugenum;

bool hugenum_iszero(hugenum q);
bool hugenum_isnonzero(hugenum q);
bool hugenum_isnan(hugenum q);
bool hugenum_isinf(hugenum q);
bool hugenum_isfinite(hugenum q);

int hugenum_sign(hugenum q);

hugenum hugenum_from_scalar(double p);
double hugenum_to_scalar(hugenum q);

hugenum hugenum_absolute(hugenum q);
hugenum hugenum_add(hugenum q1, hugenum q2);
hugenum hugenum_subtract(hugenum q1, hugenum q2);
hugenum hugenum_multiply(hugenum q1, hugenum q2);
hugenum hugenum_divide(hugenum q1, hugenum q2);
hugenum hugenum_multiply_scalar(hugenum q, double s);
hugenum hugenum_divide_scalar(hugenum q, double s);
hugenum hugenum_log(hugenum q);
hugenum hugenum_log10(hugenum q);
hugenum hugenum_exp(hugenum q);
hugenum hugenum_exp10(hugenum q);
hugenum hugenum_power(hugenum q, hugenum p);
hugenum hugenum_power_scalar(hugenum q, double p);
hugenum hugenum_negative(hugenum q);
hugenum hugenum_copysign(hugenum q1, hugenum q2);
hugenum hugenum_normalized(hugenum q);

bool hugenum_equal(hugenum q1, hugenum q2);
bool hugenum_not_equal(hugenum q1, hugenum q2);
bool hugenum_less(hugenum q1, hugenum q2);
bool hugenum_less_equal(hugenum q1, hugenum q2);

// hugenum hugenum_from_string(char* s);
// int hugenum_snprint(char* s, size_t n, hugenum q);
// TODO add printf hook: https://www.gnu.org/software/libc/manual/html_node/Customizing-Printf.html

#ifdef __cplusplus
}
#endif

#endif
