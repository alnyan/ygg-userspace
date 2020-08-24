#include <stdint.h>
#include <string.h>
#include <stdio.h>

static const uint32_t k[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

struct sha256_state {
    size_t len;
    size_t bitlen;
    uint8_t data[64];
    uint32_t h[8];
};

static inline uint32_t _rol(uint32_t a, uint32_t b) {
    return (a << b) | (a >> (32 - b));
}

static inline uint32_t _ror(uint32_t a, uint32_t b) {
    return (a >> b) | (a << (32 - b));
}

void sha256_init(struct sha256_state *st) {
    st->len = 0;
    st->bitlen = 0;
    st->h[0] = 0x6a09e667;
    st->h[1] = 0xbb67ae85;
    st->h[2] = 0x3c6ef372;
    st->h[3] = 0xa54ff53a;
    st->h[4] = 0x510e527f;
    st->h[5] = 0x9b05688c;
    st->h[6] = 0x1f83d9ab;
    st->h[7] = 0x5be0cd19;
}

void sha256_transform(struct sha256_state *st, const uint8_t *data) {
    uint32_t a, b, c, d, e, f, g, h;
    uint32_t s0, s1;
    uint32_t w[64];
    size_t i;

    for (i = 0; i < 16; ++i) {
        w[i] = (data[i << 2] << 24) | (data[(i << 2) + 1] << 16) |
               (data[(i << 2) + 2] << 8) | data[(i << 2) + 3];
    }
    for (i = 16; i < 64; ++i) {
        s0 = _ror(w[i - 15], 7) ^ _ror(w[i - 15], 18) ^ (w[i - 15] >> 3);
        s1 = _ror(w[i - 2], 17) ^ _ror(w[i - 2], 19) ^ (w[i - 2] >> 10);
        w[i] = w[i - 16] + s0 + w[i - 7] + s1;
    }

    a = st->h[0];
    b = st->h[1];
    c = st->h[2];
    d = st->h[3];
    e = st->h[4];
    f = st->h[5];
    g = st->h[6];
    h = st->h[7];

    for (i = 0; i < 64; ++i) {
        uint32_t s1 = _ror(e, 6) ^ _ror(e, 11) ^ _ror(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + s1 + ch + k[i] + w[i];
        uint32_t s0 = _ror(a, 2) ^ _ror(a, 13) ^ _ror(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = s0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    st->h[0] += a;
    st->h[1] += b;
    st->h[2] += c;
    st->h[3] += d;
    st->h[4] += e;
    st->h[5] += f;
    st->h[6] += g;
    st->h[7] += h;
}

void sha256_update(struct sha256_state *st, const uint8_t *data, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        st->data[st->len++] = data[i];
        if (st->len == 64) {
            sha256_transform(st, st->data);
            st->bitlen += 512;
            st->len = 0;
        }
    }
}

void sha256_final(struct sha256_state *st, uint8_t hash[]) {
    size_t i;

    i = st->len;

    // Pad the last chunk
    if (st->len < 56) {
        st->data[i++] = 0x80;
        while (i < 56) {
            st->data[i++] = 0x00;
        }
    } else {
        st->data[i++] = 0x80;
        while (i < 64) {
            st->data[i++] = 0x00;
        }
        sha256_transform(st, st->data);
        memset(st->data, 0, 56);
    }

    // Append total message length
    st->bitlen += st->len * 8;
    st->data[63] = st->bitlen & 0xFF;
    st->data[62] = (st->bitlen >> 8) & 0xFF;
    st->data[61] = (st->bitlen >> 16) & 0xFF;
    st->data[60] = (st->bitlen >> 24) & 0xFF;
    st->data[59] = (st->bitlen >> 32) & 0xFF;
    st->data[58] = (st->bitlen >> 40) & 0xFF;
    st->data[57] = (st->bitlen >> 48) & 0xFF;
    st->data[56] = (st->bitlen >> 56) & 0xFF;
    sha256_transform(st, st->data);

    // Reverse the bytes for proper endianness
    for (i = 0; i < 4; ++i) {
        hash[i     ] = (st->h[0] >> (24 - i * 8)) & 0xFF;
        hash[i +  4] = (st->h[1] >> (24 - i * 8)) & 0xFF;
        hash[i +  8] = (st->h[2] >> (24 - i * 8)) & 0xFF;
        hash[i + 12] = (st->h[3] >> (24 - i * 8)) & 0xFF;
        hash[i + 16] = (st->h[4] >> (24 - i * 8)) & 0xFF;
        hash[i + 20] = (st->h[5] >> (24 - i * 8)) & 0xFF;
        hash[i + 24] = (st->h[6] >> (24 - i * 8)) & 0xFF;
        hash[i + 28] = (st->h[7] >> (24 - i * 8)) & 0xFF;
    }
}

static int do_sha256sum(const char *in) {
    struct sha256_state st;
    uint8_t hash[32];
    char buf[256];
    size_t bread;
    FILE *fp;

    if (!strcmp(in, "-")) {
        fp = stdin;
    } else {
        fp = fopen(in, "rb");
        if (!fp) {
            perror(in);
            return -1;
        }
    }

    sha256_init(&st);

    while ((bread = fread(buf, 1, sizeof(buf), fp)) > 0) {
        sha256_update(&st, (uint8_t *) buf, bread);
    }

    sha256_final(&st, hash);
    for (size_t i = 0; i < 32; ++i) {
        printf("%02hhx", hash[i]);
    }
    printf(" %s\n", in);

    return 0;
}

int main(int argc, char **argv) {
    if (argc == 2) {
        return do_sha256sum(argv[1]);
    } else {
        return do_sha256sum("-");
    }

    return 0;
}
