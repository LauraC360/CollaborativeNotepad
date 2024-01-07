#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define SHA256_BLOCK_SIZE 64

// SHA-256 functions
uint32_t rightRotate(uint32_t value, int shift);
void sha256Transform(uint32_t state[8], const unsigned char block[SHA256_BLOCK_SIZE]);
void sha256(const unsigned char *message, size_t length, unsigned char hash[32]);

int main() {
    const char *password;

    char parola[30] = "hello";

    // Hash the password using SHA-256
    unsigned char hashedPassword[32];
    sha256((const unsigned char *)parola, strlen(parola), hashedPassword);

    // Print the hashed password
    printf("Hashed Password: ");
    for (int i = 0; i < 32; i++) {
        printf("%02x", hashedPassword[i]);
    }
    printf("\n");

    return 0;
}

// Right rotate function (circular right shift)
uint32_t rightRotate(uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

// SHA-256 transformation for a single block
void sha256Transform(uint32_t state[8], const unsigned char block[SHA256_BLOCK_SIZE]) {
    // SHA-256 constants
    static const uint32_t k[] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
        // ... (omitted for brevity)
    };

    // Message schedule (W)
    uint32_t W[64];

    // Copy the block into the message schedule
    for (int t = 0; t < 16; t++) {
        W[t] = (block[t * 4] << 24) | (block[t * 4 + 1] << 16) | (block[t * 4 + 2] << 8) | block[t * 4 + 3];
    }

    // Extend the message schedule
    for (int t = 16; t < 64; t++) {
        uint32_t s0 = rightRotate(W[t - 15], 7) ^ rightRotate(W[t - 15], 18) ^ (W[t - 15] >> 3);
        uint32_t s1 = rightRotate(W[t - 2], 17) ^ rightRotate(W[t - 2], 19) ^ (W[t - 2] >> 10);
        W[t] = W[t - 16] + s0 + W[t - 7] + s1;
    }

    // Initialize working variables with the current hash value
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    // Compression function main loop
    for (int t = 0; t < 64; t++) {
        uint32_t S1 = rightRotate(e, 6) ^ rightRotate(e, 11) ^ rightRotate(e, 25);
        uint32_t ch = (e & f) ^ ((~e) & g);
        uint32_t temp1 = h + S1 + ch + k[t] + W[t];
        uint32_t S0 = rightRotate(a, 2) ^ rightRotate(a, 13) ^ rightRotate(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t temp2 = S0 + maj;

        h = g;
        g = f;
        f = e;
        e = d + temp1;
        d = c;
        c = b;
        b = a;
        a = temp1 + temp2;
    }

    // Update hash value
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// SHA-256 algorithm
void sha256(const unsigned char *message, size_t length, unsigned char hash[32]) {
    // Initial hash values (first 32 bits of the fractional parts of the square roots of the first 8 prime numbers)
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };

    // Process each block in the message
    for (size_t i = 0; i < length / SHA256_BLOCK_SIZE; i++) {
        sha256Transform(state, message + i * SHA256_BLOCK_SIZE);
    }

    // Padding and length representation
    unsigned char block[SHA256_BLOCK_SIZE];
    size_t lastBlock = length % SHA256_BLOCK_SIZE;
    size_t bitLength = length * 8;

    // Copy the remaining message content
    memcpy(block, message + length - lastBlock, lastBlock);

    // Add a single '1' bit
    block[lastBlock++] = 0x80;

    // Pad with zeros until the last 8 bytes
    if (lastBlock > SHA256_BLOCK_SIZE - 8) {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - lastBlock);
        sha256Transform(state, block);
        memset(block, 0, SHA256_BLOCK_SIZE - 8);
    } else {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - 8 - lastBlock);
    }

    // Append the original message length in bits
    for (int i = 0; i < 8; i++) {
        block[SHA256_BLOCK_SIZE - 8 + i] = (bitLength >> ((7 - i) * 8)) & 0xFF;
    }

    // Final transformation
    sha256Transform(state, block);

    // Copy the final hash to the output
    for (int i = 0; i < 32; i++) {
        hash[i] = (state[i / 4] >> ((3 - i % 4) * 8)) & 0xFF;
    }
}
