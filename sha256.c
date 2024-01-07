

// fct de rotatie spre dreapta
uint32_t rightRotate(uint32_t value, int shift) {
    return (value >> shift) | (value << (32 - shift));
}

// fct de transformare - pt fctia ce descrie alg. sha256
void sha256Transform(uint32_t state[8], const unsigned char block[SHA256_BLOCK_SIZE]) {

    // SHA-256 constante
    static const uint32_t k[] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
    };

    uint32_t W[64]; //W - blocul pt mesaj

    for (int t = 0; t < 16; t++) {
        W[t] = (block[t * 4] << 24) | (block[t * 4 + 1] << 16) | (block[t * 4 + 2] << 8) | block[t * 4 + 3];
    } // copiere in blocul W

    for (int t = 16; t < 64; t++) {
        uint32_t s0 = rightRotate(W[t - 15], 7) ^ rightRotate(W[t - 15], 18) ^ (W[t - 15] >> 3);
        uint32_t s1 = rightRotate(W[t - 2], 17) ^ rightRotate(W[t - 2], 19) ^ (W[t - 2] >> 10);
        W[t] = W[t - 16] + s0 + W[t - 7] + s1;
    } // extinderea dimensiunii blocului

    // initializare cu valorile de hash
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    // compresie
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

    // update-ul valorilor de hash
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

// Algoritmul sha256 - functia de transformare a parolei
void sha256(const unsigned char *message, size_t length, unsigned char hash[32]) {
    uint32_t state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    }; //valori initiale de hash

    for (size_t i = 0; i < length / SHA256_BLOCK_SIZE; i++) {
        sha256Transform(state, message + i * SHA256_BLOCK_SIZE);
    } ///transformarea fiecaruki bloc


    unsigned char block[SHA256_BLOCK_SIZE];
    size_t lastBlock = length % SHA256_BLOCK_SIZE;
    size_t bitLength = length * 8;

    memcpy(block, message + length - lastBlock, lastBlock); // copierea mesajului

    block[lastBlock++] = 0x80; //add unui bit de 1

    if (lastBlock > SHA256_BLOCK_SIZE - 8) {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - lastBlock);
        sha256Transform(state, block);
        memset(block, 0, SHA256_BLOCK_SIZE - 8);
    } else {
        memset(block + lastBlock, 0, SHA256_BLOCK_SIZE - 8 - lastBlock);
    } // umplere cu 0 pana la utimiii 8 bytes

    for (int i = 0; i < 8; i++) {
        block[SHA256_BLOCK_SIZE - 8 + i] = (bitLength >> ((7 - i) * 8)) & 0xFF;
    }

    sha256Transform(state, block); // transformarea finala

    // copierea hash-ului final in output
    for (int i = 0; i < 32; i++) {
        hash[i] = (state[i / 4] >> ((3 - i % 4) * 8)) & 0xFF;
    }
}

void signup() {

}

int main() {
    signup();
}
