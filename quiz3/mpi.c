#include <assert.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* mpi: Multi-Precision Integers */
typedef struct {
    uint32_t *data;
    size_t capacity;
} mpi_t[1];

typedef size_t mp_bitcnt_t;

void mpi_init(mpi_t rop)
{
    rop->capacity = 0;
    rop->data = NULL;
}

void mpi_clear(mpi_t rop)
{
    free(rop->data);
}

void mpi_enlarge(mpi_t rop, size_t capacity)
{
    if (capacity > rop->capacity) {
        size_t min = rop->capacity;

        rop->capacity = capacity;

        rop->data = realloc(rop->data, capacity * 4);

        if (!rop->data && capacity != 0) {
            fprintf(stderr, "Out of memory (%zu words requested)\n", capacity);
            abort();
        }

        for (size_t n = min; n < capacity; ++n)
            rop->data[n] = 0;
    }
}

void mpi_compact(mpi_t rop)
{
    size_t capacity = rop->capacity;

    if (rop->capacity == 0)
        return;

    for (size_t i = rop->capacity - 1; i != (size_t) -1; --i) {
        if (rop->data[i])
            break;
        capacity--;
    }

    assert(capacity != (size_t) -1);

    rop->data = realloc(rop->data, capacity * 4);
    rop->capacity = capacity;

    if (!rop->data && capacity != 0) {
        fprintf(stderr, "Out of memory (%zu words requested)\n", capacity);
        abort();
    }
}

/* ceiling division without needing floating-point operations. */
static size_t ceil_div(size_t n, size_t d)
{
    return (n + d - 1) / d;
}

#define INTMAX 0x7fffffff






void mpi_set(mpi_t rop, const mpi_t op)
{
    mpi_enlarge(rop, op->capacity);

    for (size_t n = 0; n < op->capacity; ++n)
        rop->data[n] = op->data[n];

    for (size_t n = op->capacity; n < rop->capacity; ++n)
        rop->data[n] = 0;
}

/* Naive multiplication */
static void mpi_mul_naive(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    size_t capacity = op1->capacity + op2->capacity;

    mpi_t tmp;
    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    for (size_t n = 0; n < tmp->capacity; ++n)
        tmp->data[n] = 0;

    for (size_t n = 0; n < op1->capacity; ++n) {
        for (size_t m = 0; m < op2->capacity; ++m) {
            uint64_t r = (uint64_t) op1->data[n] * op2->data[m];
            uint64_t c = 0;
            for (size_t k = n + m; c || r; ++k) {
                //if (k >= tmp->capacity)
                //    mpi_enlarge(tmp, tmp->capacity + 1);
                tmp->data[k] += (r & INTMAX) + c;
                r >>= 31;
                c = tmp->data[k] >> 31;
                //tmp->data[k] &= INTMAX;
            }
        }
    }

    mpi_set(rop, tmp);

    mpi_compact(rop);

    mpi_clear(tmp);
}

void mpi_set_u32(mpi_t rop, uint32_t op)
{
    size_t capacity = ceil_div(32, 31);

    mpi_enlarge(rop, capacity);

    for (size_t n = 0; n < capacity; ++n) {
        rop->data[n] = op & INTMAX;
        op >>= 31;
    }

    for (size_t n = capacity; n < rop->capacity; ++n)
        rop->data[n] = 0;
}

void mpi_add_u32(mpi_t rop, const mpi_t op1, uint32_t op2)
{
    size_t capacity =
        op1->capacity > ceil_div(32, 31) ? op1->capacity : ceil_div(32, 31);

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 + op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = op2 & INTMAX;
        op2 >>= 31;
        rop->data[n] = r1 + r2 + c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = 0 + c;
    }
}

void mpi_mul_u32(mpi_t rop, const mpi_t op1, uint32_t op2)
{
    size_t capacity = op1->capacity + 1;

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 * op2 */
    for (size_t n = 0; n < op1->capacity; ++n) {
        assert(op1->data[n] <= (UINT64_MAX - c) / op2);
        uint64_t r = (uint64_t) op1->data[n] * op2 + c;
        rop->data[n] = r & INTMAX;
        c = r >> 31;
    }

    while (c != 0) {
        mpi_enlarge(rop, capacity + 1);
        rop->data[capacity] = c & INTMAX;
        c >>= 31;
    }
}

int mpi_set_str(mpi_t rop, const char *str, int base)
{
    assert(base == 10); /* only decimal integers */

    size_t len = strlen(str);

    mpi_set_u32(rop, 0U);

    for (size_t i = 0; i < len; ++i) {
        mpi_mul_u32(rop, rop, 10U);
        assert(str[i] >= '0' && str[i] <= '9');
        mpi_add_u32(rop, rop, (uint32_t) (str[i] - '0'));
    }

    return 0;
}

int mpi_cmp_u32(const mpi_t op1, uint32_t op2)
{
    size_t capacity = op1->capacity;

    if (capacity < ceil_div(32, 31))
        capacity = ceil_div(32, 31);

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = 0;

        switch (n) {
        case 0:
            r2 = (op2) & INTMAX;
            break;
        case 1:
            r2 = (op2 >> 31) & INTMAX;
            break;
        }

        if (r1 < r2)
            return -1;

        if (r1 > r2)
            return +1;
    }

    return 0;
}

size_t mpi_sizeinbase(const mpi_t op, int base)
{
    assert(base == 2); /* Only binary */

    /* find right-most non-zero word */
    for (size_t i = op->capacity - 1; i != (size_t) -1; --i) {
        if (op->data[i] != 0) {
            /* find right-most non-zero bit */
            for (int b = 30; b >= 0; --b) {
                if ((op->data[i] & (1U << b)) != 0)
                    return 31 * i + b + 1;
            }
        }
    }

    return 0;
}

/* Retrieve a 32-bit word from the MPI, shifted by lshift bits. */
uint32_t mpi_get_word_lshift_u32(const mpi_t op, size_t n, size_t lshift)
{
    uint32_t r = 0;

    assert(lshift < 31);

    if (n < op->capacity + 0)
        r |= (op->data[n + 0] << lshift) & INTMAX;

    if (n < op->capacity + 1 && n > 0) {
        r |= op->data[n - 1] >> (31 - lshift);
    }

    return r;
}

/* Left-shift (multiply) a multi-precision integer by 2^op2 */
void mpi_mul_2exp(mpi_t rop, const mpi_t op1, mp_bitcnt_t op2)
{
    size_t words = ceil_div(op2, 31);
    size_t word_shift = op2 / 31;
    size_t bit_shift = op2 % 31;

    size_t capacity = op1->capacity + words;

    mpi_t tmp;

    mpi_init(tmp);

    mpi_enlarge(tmp, capacity);

    for (size_t i = 0; i < tmp->capacity; ++i) {
        tmp->data[i] = i >= word_shift ? mpi_get_word_lshift_u32(
                                             op1, i - word_shift, bit_shift)
                                       : 0;
    }

    mpi_set(rop, tmp);

    mpi_clear(tmp);

    mpi_compact(rop);
}

int mpi_testbit(const mpi_t op, mp_bitcnt_t bit_index)
{
    size_t word = bit_index / 31;
    size_t bit = bit_index % 31;

    uint32_t r = word < op->capacity ? op->data[word] : 0;

    return (r >> bit) & 1;
}


void mpi_setbit(mpi_t rop, mp_bitcnt_t bit_index)
{
    size_t word = bit_index / 31;
    size_t bit = bit_index % 31;

    mpi_enlarge(rop, word + 1);

    uint32_t mask = 1U << bit;
    rop->data[word] |= mask;
}

void mpi_sub(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    mpi_enlarge(rop, capacity);

    uint32_t c = 0;

    /* op1 - op2 */
    for (size_t n = 0; n < rop->capacity; ++n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = (n < op2->capacity) ? op2->data[n] : 0;
        rop->data[n] = r1 - r2 - c;
        c = rop->data[n] >> 31;
        rop->data[n] &= INTMAX;
    }

    if (c != 0) {
        fprintf(stderr, "Negative numbers not supported\n");
        abort();
    }

    mpi_compact(rop);
}

int mpi_cmp(const mpi_t op1, const mpi_t op2)
{
    size_t capacity =
        op1->capacity > op2->capacity ? op1->capacity : op2->capacity;

    if (capacity == 0)
        return 0;

    for (size_t n = capacity - 1; n != (size_t) -1; --n) {
        uint32_t r1 = (n < op1->capacity) ? op1->data[n] : 0;
        uint32_t r2 = (n < op2->capacity) ? op2->data[n] : 0;

        if (r1 < r2)
            return -1;

        if (r1 > r2)
            return +1;
    }

    return 0;
}

void mpi_fdiv_qr(mpi_t q, mpi_t r, const mpi_t n, const mpi_t d)
{
    mpi_t n0, d0;
    mpi_init(n0);
    mpi_init(d0);
    mpi_set(n0, n);
    mpi_set(d0, d);                                                                                  
    if (mpi_cmp_u32(d0, 0) == 0) {
        fprintf(stderr, "Division by zero\n");
        abort();
    }

    mpi_set_u32(q, 0);
    mpi_set_u32(r, 0);

    size_t start = mpi_sizeinbase(n0, 2) - 1;

    for (size_t i = start; i != (size_t) -1; --i) {
        mpi_mul_2exp(r, r, 1);
        if (mpi_testbit(n0, i) != 0)
            mpi_setbit(r, 0);
        if (mpi_cmp(r, d0) >= 0) {
            mpi_sub(r, r, d0);
            mpi_setbit(q, i);
        }
    }

    mpi_clear(n0);
    mpi_clear(d0);
}

void mpi_gcd(mpi_t rop, const mpi_t op1, const mpi_t op2)
{
    if (mpi_cmp_u32(op1, 0) == 0 || mpi_cmp_u32(op2, 0) == 0) {
        fprintf(stderr, "Both operands are zero, GCD undefined\n");
        return;
    }
        
    mpi_t opa, opb, q, r;
    mpi_init(opa); // Dividend
    mpi_init(opb); // Divisor        
    mpi_init(q);
    mpi_init(r);
    mpi_set(opa,op1);
    mpi_set(opb,op2);
    while(mpi_cmp_u32(opb, 0) != 0){
        mpi_fdiv_qr(q, r, opa, opb);   
        mpi_set(opa,opb);
        mpi_set(opb,r);
    }
    mpi_set(rop,opa);

    mpi_clear(q);
    mpi_clear(r);
    mpi_clear(opa);
    mpi_clear(opb);
    
}

int main(){
    printf("mpi_mul_naive\n");
    {
        mpi_t rop, op1, op2;
        mpi_init(rop);
        mpi_init(op1);
        mpi_init(op2);
        mpi_set_str(op1, "10000", 10);
        mpi_set_str(op2, "10000", 10);
        mpi_mul_naive(rop,op1,op2);
        assert(mpi_cmp_u32(rop, 100000000 ) == 0 );
        mpi_clear(rop);
        mpi_clear(op1);
        mpi_clear(op2);    
    }
    printf("mpi_fdiv_qr\n");
    {
        mpi_t n, d, q, r;
        mpi_init(n);
        mpi_init(d);
        mpi_init(q);
        mpi_init(r);

        mpi_set_str(n, "10", 10);
        mpi_set_str(d, "4", 10);
        mpi_fdiv_qr(q, r, n, d);
        assert(mpi_cmp_u32(q, 2) == 0);
        assert(mpi_cmp_u32(r, 2) == 0);

        mpi_clear(n);
        mpi_clear(d);
        mpi_clear(q);
        mpi_clear(r);
    }
    printf("GCD test\n");
    {
        mpi_t a, b, r;
        mpi_init(a);
        mpi_init(b);
        mpi_init(r);

        mpi_set_str(a, "50", 10);
        mpi_set_str(b, "30", 10);

        mpi_gcd(r, a, b);
        assert(mpi_cmp_u32(r, 10) == 0);

        mpi_clear(a);
        mpi_clear(b);
        mpi_clear(r);
    }

}
