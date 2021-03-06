#include "mpc.h"
#include "mzd_additional.h"
#include "simd.h"

#if 0
void mpc_clear(mzd_t** res, unsigned sc) {
  for (unsigned int i = 0; i < sc; i++) {
    mzd_local_clear(res[i]);
  }
}
#endif

void mpc_shift_right(mzd_t* const* res, mzd_t* const* val, unsigned count, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i)
    mzd_shift_right(res[i], val[i], count);
}

void mpc_shift_left(mzd_t* const* res, mzd_t* const* val, unsigned count, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i)
    mzd_shift_left(res[i], val[i], count);
}

void mpc_and_const(mzd_t* const* res, mzd_t* const* first, mzd_t const* second, unsigned sc) {
  for (unsigned i = 0; i < sc; i++) {
    mzd_and(res[i], first[i], second);
  }
}

void mpc_xor(mzd_t* const* res, mzd_t* const* first, mzd_t* const* second, unsigned sc) {
  for (unsigned i = 0; i < sc; i++) {
    mzd_xor(res[i], first[i], second[i]);
  }
}

#ifdef WITH_OPT
#ifdef WITH_SSE2
__attribute__((target("sse2"))) void mpc_and_sse(__m128i* res, __m128i const* first,
                                                 __m128i const* second, __m128i const* r,
                                                 view_t const* view, unsigned viewshift) {
  for (unsigned m = 0; m < SC_PROOF; ++m) {
    const unsigned j = (m + 1) % SC_PROOF;

    __m128i* sm = __builtin_assume_aligned(FIRST_ROW(view->s[m]), 16);

    __m128i tmp1 = _mm_xor_si128(second[m], second[j]);
    __m128i tmp2 = _mm_and_si128(first[j], second[m]);
    tmp1         = _mm_and_si128(tmp1, first[m]);
    tmp1         = _mm_xor_si128(tmp1, tmp2);

    tmp2   = _mm_xor_si128(r[m], r[j]);
    res[m] = tmp1 = _mm_xor_si128(tmp1, tmp2);

    tmp1 = mm128_shift_right(tmp1, viewshift);
    *sm  = _mm_xor_si128(tmp1, *sm);
  }
}
#endif

#ifdef WITH_AVX2
__attribute__((target("avx2"))) void mpc_and_avx(__m256i* res, __m256i const* first,
                                                 __m256i const* second, __m256i const* r,
                                                 view_t const* view, unsigned viewshift) {
  for (unsigned m = 0; m < SC_PROOF; ++m) {
    const unsigned j = (m + 1) % SC_PROOF;

    __m256i* sm = __builtin_assume_aligned(FIRST_ROW(view->s[m]), 32);

    __m256i tmp1 = _mm256_xor_si256(second[m], second[j]);
    __m256i tmp2 = _mm256_and_si256(first[j], second[m]);
    tmp1         = _mm256_and_si256(tmp1, first[m]);
    tmp1         = _mm256_xor_si256(tmp1, tmp2);

    tmp2   = _mm256_xor_si256(r[m], r[j]);
    res[m] = tmp1 = _mm256_xor_si256(tmp1, tmp2);

    tmp1 = mm256_shift_right(tmp1, viewshift);
    *sm  = _mm256_xor_si256(tmp1, *sm);
  }
}
#endif
#endif

void mpc_and(mzd_t* const* res, mzd_t* const* first, mzd_t* const* second, mzd_t* const* r,
             view_t* view, unsigned viewshift, mzd_t* const* buffer) {
  mzd_t* b = buffer[0];

  for (unsigned m = 0; m < SC_PROOF; ++m) {
    const unsigned j = (m + 1) % SC_PROOF;

    mzd_and(res[m], first[m], second[m]);

    mzd_and(b, first[j], second[m]);
    mzd_xor(res[m], res[m], b);

    mzd_and(b, first[m], second[j]);
    mzd_xor(res[m], res[m], b);

    mzd_xor(res[m], res[m], r[m]);
    mzd_xor(res[m], res[m], r[j]);
  }

  mpc_shift_right(buffer, res, viewshift, SC_PROOF);
  mpc_xor(view->s, view->s, buffer, SC_PROOF);
}

#ifdef WITH_OPT
#ifdef WITH_SSE2
__attribute__((target("sse2"))) void mpc_and_verify_sse(__m128i* res, __m128i const* first,
                                                        __m128i const* second, __m128i const* r,
                                                        view_t const* view, __m128i const mask,
                                                        unsigned viewshift) {
  for (unsigned m = 0; m < (SC_VERIFY - 1); ++m) {
    const unsigned j = (m + 1);

    __m128i* sm = __builtin_assume_aligned(FIRST_ROW(view->s[m]), 16);

    __m128i tmp1 = _mm_xor_si128(second[m], second[j]);
    __m128i tmp2 = _mm_and_si128(first[j], second[m]);
    tmp1         = _mm_and_si128(tmp1, first[m]);
    tmp1         = _mm_xor_si128(tmp1, tmp2);

    tmp2   = _mm_xor_si128(r[m], r[j]);
    res[m] = tmp1 = _mm_xor_si128(tmp1, tmp2);

    tmp1 = mm128_shift_right(tmp1, viewshift);
    *sm  = _mm_xor_si128(tmp1, *sm);
  }

  __m128i const* s1  = __builtin_assume_aligned(CONST_FIRST_ROW(view->s[SC_VERIFY - 1]), 16);
  __m128i rsc        = mm128_shift_left(*s1, viewshift);
  res[SC_VERIFY - 1] = _mm_and_si128(rsc, mask);
}
#endif

#ifdef WITH_AVX2
__attribute__((target("avx2"))) void mpc_and_verify_avx(__m256i* res, __m256i const* first,
                                                        __m256i const* second, __m256i const* r,
                                                        view_t const* view, __m256i const mask,
                                                        unsigned viewshift) {
  for (unsigned m = 0; m < (SC_VERIFY - 1); ++m) {
    const unsigned j = (m + 1);

    __m256i* sm = __builtin_assume_aligned(FIRST_ROW(view->s[m]), 32);

    __m256i tmp1 = _mm256_xor_si256(second[m], second[j]);
    __m256i tmp2 = _mm256_and_si256(first[j], second[m]);
    tmp1         = _mm256_and_si256(tmp1, first[m]);
    tmp1         = _mm256_xor_si256(tmp1, tmp2);

    tmp2   = _mm256_xor_si256(r[m], r[j]);
    res[m] = tmp1 = _mm256_xor_si256(tmp1, tmp2);

    tmp1 = mm256_shift_right(tmp1, viewshift);
    *sm  = _mm256_xor_si256(tmp1, *sm);
  }

  __m256i const* s1  = __builtin_assume_aligned(CONST_FIRST_ROW(view->s[SC_VERIFY - 1]), 32);
  __m256i rsc        = mm256_shift_left(*s1, viewshift);
  res[SC_VERIFY - 1] = _mm256_and_si256(rsc, mask);
}
#endif
#endif

void mpc_and_verify(mzd_t* const* res, mzd_t* const* first, mzd_t* const* second, mzd_t* const* r,
                    view_t const* view, mzd_t const* mask, unsigned viewshift,
                    mzd_t* const* buffer) {
  mzd_t* b = buffer[0];

  for (unsigned m = 0; m < (SC_VERIFY - 1); ++m) {
    const unsigned j = m + 1;

    mzd_and(res[m], first[m], second[m]);

    mzd_and(b, first[j], second[m]);
    mzd_xor(res[m], res[m], b);

    mzd_and(b, first[m], second[j]);
    mzd_xor(res[m], res[m], b);

    mzd_xor(res[m], res[m], r[m]);
    mzd_xor(res[m], res[m], r[j]);
  }

  for (unsigned m = 0; m < (SC_VERIFY - 1); ++m) {
    mzd_shift_right(b, res[m], viewshift);
    mzd_xor(view->s[m], view->s[m], b);
  }

  mzd_shift_left(res[SC_VERIFY - 1], view->s[SC_VERIFY - 1], viewshift);
  mzd_and(res[SC_VERIFY - 1], res[SC_VERIFY - 1], mask);
}

#if 0
int mpc_and_bit(BIT* a, BIT* b, BIT* r, view_t* views, int* i, unsigned bp, unsigned sc) {
  BIT* wp = (BIT*)malloc(sc * sizeof(BIT));
  for (unsigned m = 0; m < sc; ++m) {
    unsigned j = (m + 1) % 3;
    wp[m]      = (a[m] & b[m]) ^ (a[j] & b[m]) ^ (a[m] & b[j]) ^ r[m] ^ r[j];
  }
  for (unsigned m = 0; m < sc; ++m) {
    a[m] = wp[m];
  }
  mpc_write_bit(views[*i].s, bp, a, sc);
  free(wp);
  return 0;
}

int mpc_and_bit_verify(BIT* a, BIT* b, BIT* r, view_t* views, int* i, unsigned bp, unsigned sc) {
  BIT* wp = (BIT*)malloc(sc * sizeof(BIT));
  for (unsigned m = 0; m < sc - 1; m++) {
    unsigned j = m + 1;
    wp[m]      = (a[m] & b[m]) ^ (a[j] & b[m]) ^ (a[m] & b[j]) ^ r[m] ^ r[j];
  }
  for (unsigned m = 0; m < sc - 1; m++) {
    a[m] = wp[m];
    if (a[m] != mzd_read_bit(views[*i].s[m], 0, bp)) {
      free(wp);
      return -1;
    }
  }
  a[sc - 1] = mzd_read_bit(views[*i].s[sc - 1], 0, bp);
  free(wp);
  return 0;
}

void mpc_xor_bit(BIT* a, BIT* b, unsigned sc) {
  for (unsigned i = 0; i < sc; i++) {
    a[i] ^= b[i];
  }
}

void mpc_read_bit(BIT* out, mzd_t** vec, rci_t n, unsigned sc) {
  for (unsigned i = 0; i < sc; i++)
    out[i]        = mzd_read_bit(vec[i], 0, n);
}

void mpc_write_bit(mzd_t** vec, rci_t n, BIT* bit, unsigned sc) {
  for (unsigned i = 0; i < sc; i++)
    mzd_write_bit(vec[i], 0, n, bit[i]);
}
#endif

mzd_t** mpc_add(mzd_t** result, mzd_t** first, mzd_t** second, unsigned sc) {
  for (unsigned i = 0; i < sc; i++) {
    mzd_xor(result[i], first[i], second[i]);
  }
  return result;
}

mzd_t** mpc_const_add(mzd_t** result, mzd_t** first, mzd_t const* second, unsigned sc, unsigned c) {
  if (c == 0)
    mzd_xor(result[0], first[0], second);
  else if (c == sc)
    mzd_xor(result[sc - 1], first[sc - 1], second);
  return result;
}

mzd_t** mpc_const_mat_mul(mzd_t** result, mzd_t const* matrix, mzd_t** vector, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i) {
    mzd_mul_v(result[i], vector[i], matrix);
  }
  return result;
}

void mpc_const_addmat_mul_l(mzd_t** result, mzd_t const* matrix, mzd_t** vector, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i) {
    mzd_addmul_vl(result[i], vector[i], matrix);
  }
}

mzd_t** mpc_const_mat_mul_l(mzd_t** result, mzd_t const* matrix, mzd_t** vector, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i) {
    mzd_mul_vl(result[i], vector[i], matrix);
  }
  return result;
}

void mpc_copy(mzd_t** out, mzd_t* const* in, unsigned sc) {
  for (unsigned i = 0; i < sc; ++i) {
    mzd_local_copy(out[i], in[i]);
  }
}

mzd_t* mpc_reconstruct_from_share(mzd_t* dst, mzd_t** shared_vec) {
  if (!dst) {
    dst = mzd_local_init_ex(shared_vec[0]->nrows, shared_vec[0]->ncols, false);
  }
  mzd_xor(dst, shared_vec[0], shared_vec[1]);
  return mzd_xor(dst, dst, shared_vec[2]);
}

void mpc_print(mzd_t** shared_vec) {
  mzd_t* r = mpc_reconstruct_from_share(NULL, shared_vec);
  mzd_print(r);
  mzd_local_free(r);
}

void mpc_free(mzd_t** vec, unsigned sc) {
  (void)sc;
  mzd_local_free_multiple(vec);
  free(vec);
}

mzd_t** mpc_init_empty_share_vector(rci_t n, unsigned sc) {
  mzd_t** s = malloc(sc * sizeof(mzd_t*));
  mzd_local_init_multiple(s, sc, 1, n);
  return s;
}

mzd_t** mpc_init_random_vector(rci_t n, unsigned sc) {
  mzd_t** s = malloc(sc * sizeof(mzd_t*));
  mzd_local_init_multiple_ex(s, sc, 1, n, false);
  for (unsigned i = 0; i < sc; ++i) {
    mzd_randomize_ssl(s[i]);
  }
  return s;
}

mzd_t** mpc_init_plain_share_vector(mzd_t const* v) {
  mzd_t** s = malloc(3 * sizeof(mzd_t*));
  mzd_local_init_multiple_ex(s, 3, 1, v->ncols, false);
  for (unsigned i = 0; i < 3; ++i) {
    mzd_local_copy(s[i], v);
  }

  return s;
}

mzd_t** mpc_init_share_vector(mzd_t const* v) {
  mzd_t** s = malloc(3 * sizeof(mzd_t*));
  mzd_local_init_multiple_ex(s, 3, 1, v->ncols, false);

  mzd_randomize_ssl(s[0]);
  mzd_randomize_ssl(s[1]);

  mzd_xor(s[2], s[0], s[1]);
  mzd_xor(s[2], s[2], v);

  return s;
}
