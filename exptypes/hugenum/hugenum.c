#include "hugenum.h"
#include "math.h"
#include "assert.h"
#include "errno.h"
#include "stddef.h"
#include "stdint.h"
#include "stdio.h"
#include "stdbool.h"
#include "tgmath.h"

#define LOG10_E 0.4342944819032518276511289189166050822943970058036665661144537831658646492088707747292249493384317483187061067447663037336416792

void _swap(hugenum *q1, hugenum *q2 );
void _order_by_magnitude(hugenum *q1, hugenum *q2);
void _order_by_value(hugenum *q1, hugenum *q2);

void
_print(hugenum q)
{
    fprintf(stderr, "%g,%d,%s\n", q.x, q.e, q.p?"+":"-");
}

inline void
_normalize_up(hugenum_x_t *x, hugenum_e_t *e)
{
    while ( *x >= HUGENUM_X_MAX && *e < UINT_FAST8_MAX ) {
        *x = log10(*x);
        ++*e;
    }
}

inline void
_normalize_down(hugenum_x_t *x, hugenum_e_t *e)
{
    while ( *x < HUGENUM_X_MIN && *e > HUGENUM_E_MIN ) {
        *x = pow(10, *x);
        --*e;
    }
}

inline void
_normalize(hugenum_x_t *x, hugenum_e_t *e)
{
    while ( *x >= HUGENUM_X_MAX && *e < HUGENUM_E_MAX ) {
        *x = log10(*x);
        ++*e;
    }
    while ( *x < HUGENUM_X_MIN && *e > HUGENUM_E_MIN ) {
        *x = pow(10, *x);
        --*e;
    }
}


hugenum
hugenum_at_e(hugenum q, hugenum_e_t e)
{
    hugenum r = q;
    for (; r.e > e; --r.e) {
        r.x = pow(10, r.x);
    }
    for (; r.e < e; ++r.e) {
        r.x = log10(r.x);
    }
    // if (isnan(r.x)) {
    //     r.e = 0;
    //     r.x = 0;
    //     errno = 0;
    // }
}

hugenum
hugenum_normalized(hugenum q)
{
    _normalize(&q.x, &q.e);
    return q;
}

bool
hugenum_iszero(hugenum q)
{
    return q.e == HUGENUM_E_MIN && q.x == 0;
}

bool
hugenum_isnonzero(hugenum q)
{
    return ! hugenum_iszero(q);
}

bool
hugenum_isnan(hugenum q)
{
    return isnan(q.x);
}

bool
hugenum_isinf(hugenum q)
{
    return q.e >= HUGENUM_E_MAX && isinf(q.x);
}

bool
hugenum_isfinite(hugenum q)
{
    return q.e < HUGENUM_E_MAX;
}

int
hugenum_sign(hugenum q)
{
    return q.p ? +1 : -1;
}

hugenum
hugenum_from_scalar(double p)
{
    hugenum_x_t x = (hugenum_x_t) p;
    hugenum r = {fabs(x), 0, x >= 0};
    _normalize(&r.x, &r.e);
    return r;
}

double
hugenum_to_scalar(hugenum q)
{
    hugenum_x_t r = q.x;
    for (hugenum_e_t e = q.e; e > 0; --e) {
        r = pow(10, r);
    }
    return (double)(hugenum_sign(q) * r);
}

hugenum
hugenum_absolute(hugenum q)
{
    return (hugenum) {
        fabs(q.x),
        q.e,
        true
    };
}

hugenum
hugenum_add(hugenum q1, hugenum q2)
{
    _order_by_magnitude(&q1, &q2); // q1 is now the larger of the two

    int s =  q1.p ^ q2.p ? -1 : +1;
    
    hugenum_x_t dx;
    if (q1.e == 0) {
        dx = s * q2.x;
    } else if (q1.e == 1) {
        hugenum_x_t q2x = (q2.e == 1 ? q2.x : log10(q2.x)); 
        dx = log10(1 + s * pow(10, q2x - q1.x)); // using log-add-exp formula
    } else {
        dx = 0;
    }

    if (isnan(dx) && ! isinf(dx)) {
        dx = 0;
    }

    hugenum r = {
        q1.x + dx,
        q1.e,
        q1.p
    };
    _normalize(&r.x, &r.e);

    return r;
}

hugenum
hugenum_subtract(hugenum q1, hugenum q2)
{
   return hugenum_add(q1, hugenum_negative(q2));
}

hugenum
hugenum_multiply(hugenum q1, hugenum q2)
{
    _order_by_magnitude(&q1, &q2); // q1 is now the larger of the two

    int s =  q1.p ^ q2.p ? -1 : +1;
    
    hugenum_x_t dx, px;

    if (q1.e == 0) {
        px = q2.x;
        dx = 0;
    } else if (q1.e == 1) {
        px = 1;
        dx = (q2.e == 1 ? q2.x : log10(q2.x));
    } else if (q1.e == 2) {
        hugenum_x_t q2x = ( q2.e == 2 ? q2.x : log10(q2.x) );
        px = 1;
        dx = ( q2.e == 0 ? 0 : log10(1 + s * pow(10, q2x - q1.x)) ); // log-add-exp optimization
    } else {
        px = 1;
        dx = 0;
    }

    if (isnan(dx) && ! isinf(dx)) {
        dx = 0;
    }

    hugenum r = {
        px * q1.x + dx,
        q1.e,
        ! q1.p ^ q2.p
    };
    _normalize(&r.x, &r.e);

    return r;
}

// Logarithm implementation; above is (theoretically) more performant, but this one is a lot simpler to understand:
/*
hugenum
hugenum_multiply(hugenum q1, hugenum q2)
{
    hugenum r;
    q1 = hugenum_log10(hugenum_absolute(q1));
    q2 = hugenum_log10(hugenum_absolute(q2));
    r = hugenum_add(q1, q2);
    r = hugenum_exp10(r);
    r.p = ! q1.p ^ q2.p;
    _normalize(&r.x, &r.e);
    return r;
}
*/

// TODO special-case things rather than using logs
hugenum
hugenum_divide(hugenum q1, hugenum q2)
{
    hugenum r;
    q1 = hugenum_log10(hugenum_absolute(q1));
    q2 = hugenum_log10(hugenum_absolute(q2));
    r = hugenum_subtract(q1, q2);
    r = hugenum_exp10(r);
    r.p = ! q1.p ^ q2.p;
    _normalize(&r.x, &r.e);
    return r;
}

hugenum
hugenum_multiply_scalar(hugenum q, double x) {
    int s =  q.p ^ (x>=0) ? -1 : +1;
    
    hugenum_x_t dx, px;

    if (q.e == 0) {
        px = fabs(x);
        dx = 0;
    } else if (q.e == 1) {
        px = 1;
        dx = log10(x);
    } else {
        px = 1;
        dx = 0;
    }

    if (isnan(dx) && ! isinf(dx)) {
        dx = 0;
    }

    hugenum r = {
        px * q.x + dx,
        q.e,
        ! q.p ^ (x>=0)
    };
    _normalize(&r.x, &r.e);

    return r;
}
hugenum
hugenum_divide_scalar(hugenum q, double x) {
    int s =  q.p ^ (x>=0) ? -1 : +1;
    
    hugenum_x_t dx, px;

    if (q.e == 0) {
        px = fabs(x);
        dx = 0;
    } else if (q.e == 1) {
        px = 1;
        dx = -log10(x);
    } else {
        px = 1;
        dx = 0;
    }

    if (isnan(dx) && ! isinf(dx)) {
        dx = 0;
    }

    hugenum r = {
        px * q.x + dx,
        q.e,
        ! q.p ^ (x>=0)
    };
    _normalize(&r.x, &r.e);

    return r;
}

hugenum
hugenum_log(hugenum q)
{
    if (!q.p) {
        errno = EDOM;
        return (hugenum) {NAN, 0, false};
    } else if (q.e > 0) {
        hugenum r = {q.x*LOG10_E, q.e-1, true};
        _normalize_up(&r.x, &r.e);
        return r;
    } else {
        hugenum_x_t l = log(q.x);
        return (hugenum) {fabs(l), 0, l>=0};
    }
}

hugenum
hugenum_log10(hugenum q)
{
    if (!q.p) {
        errno = EDOM;
        return (hugenum) {NAN, 0, false};
    } else if (q.e > 0) {
        return (hugenum) {q.x, q.e-1, true};
    } else {
        hugenum_x_t l = log10(q.x);
        return (hugenum) {fabs(l), 0, l>=0};
    }
}

hugenum
hugenum_exp(hugenum q)
{
    if (q.p) {
        hugenum r = {q.x/LOG10_E, q.e+1, true};
        _normalize_down(&r.x, &r.e);
        return r;
    } else {
        return (hugenum) {
            exp(hugenum_to_scalar(q)), // if the hugenum is large enough to overflow and be +inf as a scalar, then exp(q) will be 0 anyway
            0,
            true
        };
    }
}

hugenum
hugenum_exp10(hugenum q)
{
    if (q.p) {
        return (hugenum) {q.x, q.e+1, true};
    } else {
        return (hugenum) {
            pow(10, hugenum_to_scalar(q)), // if the hugenum is large enough to overflow and be +inf as a scalar, then 10^q will be 0 anyway
            0,
            true
        };
    }
}

hugenum
hugenum_power(hugenum q, hugenum p)
{
    hugenum r = hugenum_exp10(hugenum_multiply(hugenum_log10(q), p));
    _normalize(&r.x, &r.e);
    return r;
}

hugenum
hugenum_power_scalar(hugenum q, double p)
{
    hugenum r = hugenum_exp10(hugenum_multiply_scalar(hugenum_log10(q), p));
    _normalize(&r.x, &r.e);
    return r;
}

hugenum
hugenum_negative(hugenum q)
{
    return (hugenum) {q.x, q.e, !q.p};
}

hugenum
hugenum_copysign(hugenum q1, hugenum q2)
{
    return (hugenum) {
        q1.x,
        q1.e,
        q2.p
    };
}

bool
hugenum_equal(hugenum q1, hugenum q2)
{
    return 
        !hugenum_isnan(q1) &&
        !hugenum_isnan(q2) &&
        q1.x == q2.x && 
        q1.e == q2.e && 
        q1.p == q2.p;
}

bool
hugenum_not_equal(hugenum q1, hugenum q2)
{
    return !hugenum_equal(q1, q2);
}

bool
_mag_less(hugenum q1, hugenum q2)
{
    return (q1.e != q2.e ? q1.e < q2.e : q1.x < q2.x);
}

bool
hugenum_less(hugenum q1, hugenum q2)
{
    return
        (!hugenum_isnan(q1) &&
         !hugenum_isnan(q2)) && (
            q1.p != q2.p ? (!q1.p && q2.p) :
            q1.p ? _mag_less(q1, q2) : !_mag_less(q2, q1));
}

bool
hugenum_less_equal(hugenum q1, hugenum q2)
{
    return hugenum_equal(q1, q2) || hugenum_less(q1, q2);
}


void 
_swap(hugenum *q1, hugenum *q2 ) {
    hugenum temp = *q1;
    *q1 = *q2;
    *q2 = temp;
}
void
_order_by_magnitude(hugenum *q1, hugenum *q2)
{
    if (_mag_less(*q1, *q2)) _swap(q1,q2);
}

void
_order_by_value(hugenum *q1, hugenum *q2)
{
    if (hugenum_less(*q1, *q2)) _swap(q1,q2);
}