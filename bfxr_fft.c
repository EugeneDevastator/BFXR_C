#include "bfxr_fft.h"
#include <math.h>

// Iterative Cooley-Tukey FFT, in-place, n must be power of 2
void bfxr_fft(complex float* buf, int n) {
    // Bit-reversal permutation
    for (int i = 1, j = 0; i < n; i++) {
        int bit = n >> 1;
        for (; j & bit; bit >>= 1)
            j ^= bit;
        j ^= bit;
        if (i < j) {
            complex float tmp = buf[i];
            buf[i] = buf[j];
            buf[j] = tmp;
        }
    }

    // FFT stages
    for (int len = 2; len <= n; len <<= 1) {
        float ang = -2 * M_PI / len;
        complex float wlen = cexpf(ang * I);
        for (int i = 0; i < n; i += len) {
            complex float w = 1;
            for (int j = 0; j < len / 2; j++) {
                complex float u = buf[i + j];
                complex float v = buf[i + j + len / 2] * w;
                buf[i + j] = u + v;
                buf[i + j + len / 2] = u - v;
                w *= wlen;
            }
        }
    }
}
