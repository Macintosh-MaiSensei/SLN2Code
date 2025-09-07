#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>

namespace fs = std::filesystem;

class SHA3 {
private:
    static constexpr std::array<uint64_t, 24> RC = {
        0x0000000000000001, 0x0000000000008082, 0x800000000000808a,
        0x8000000080008000, 0x000000000000808b, 0x0000000080000001,
        0x8000000080008081, 0x8000000000008009, 0x000000000000008a,
        0x0000000000000088, 0x0000000080008009, 0x000000008000000a,
        0x000000008000808b, 0x800000000000008b, 0x8000000000008089,
        0x8000000000008003, 0x8000000000008002, 0x8000000000000080,
        0x000000000000800a, 0x800000008000000a, 0x8000000080008081,
        0x8000000000008080, 0x0000000080000001, 0x8000000080008008
    };

    static constexpr std::array<int, 25> RHO = {
        0, 1, 62, 28, 27, 36, 44, 6, 55, 20, 3, 10, 43, 25, 39, 41, 45, 15, 21, 8, 18, 2, 61, 56, 14
    };

    static constexpr std::array<int, 25> PI = {
        0, 10, 20, 5, 15, 16, 1, 11, 21, 6, 7, 17, 2, 12, 22, 23, 8, 18, 3, 13, 14, 24, 9, 19, 4
    };

    size_t digest_size;
    size_t rate;
    size_t rate_bytes;
    size_t capacity;
    std::array<uint64_t, 25> state;
    std::vector<uint8_t> buffer;

    static uint64_t rotl64(uint64_t x, int n) {
        return (x << n) | (x >> (64 - n));
    }

    void theta() {
        std::array<uint64_t, 5> C;
        for (int x = 0; x < 5; x++) {
            C[x] = state[x] ^ state[x + 5] ^ state[x + 10] ^ state[x + 15] ^ state[x + 20];
        }
        
        std::array<uint64_t, 5> D;
        for (int x = 0; x < 5; x++) {
            D[x] = C[(x + 4) % 5] ^ rotl64(C[(x + 1) % 5], 1);
        }
        
        for (int x = 0; x < 5; x++) {
            for (int y = 0; y < 5; y++) {
                state[x + 5 * y] ^= D[x];
            }
        }
    }

    void rho_pi() {
        std::array<uint64_t, 25> temp;
        for (int i = 0; i < 25; i++) {
            temp[i] = rotl64(state[PI[i]], RHO[PI[i]]);
        }
        state = temp;
    }

    void chi() {
        std::array<uint64_t, 5> A;
        for (int y = 0; y < 5; y++) {
            for (int x = 0; x < 5; x++) {
                A[x] = state[x + 5 * y];
            }
            for (int x = 0; x < 5; x++) {
                state[x + 5 * y] = A[x] ^ ((~A[(x + 1) % 5]) & A[(x + 2) % 5]);
            }
        }
    }

    void iota(int round) {
        state[0] ^= RC[round];
    }

    void keccak_f() {
        for (int round = 0; round < 24; round++) {
            theta();
            rho_pi();
            chi();
            iota(round);
        }
    }

    void absorb_block(const uint8_t* block) {
        for (size_t i = 0; i < rate_bytes; i += 8) {
            uint64_t word = 0;
            for (int j = 0; j < 8; j++) {
                word |= static_cast<uint64_t>(block[i + j]) << (8 * j);
            }
            state[i / 8] ^= word;
        }
        keccak_f();
    }

    void pad_and_absorb() {
        size_t block_size = rate_bytes;
        size_t n = buffer.size();
        
        if (n == block_size - 1) {
            buffer.push_back(0x86);
            absorb_block(buffer.data());
        } else {
            buffer.push_back(0x01);
            while (buffer.size() < block_size - 1) {
                buffer.push_back(0x00);
            }
            buffer.push_back(0x80);
            absorb_block(buffer.data());
        }
        buffer.clear();
    }

    void squeeze(std::vector<uint8_t>& output, size_t output_len) {
        size_t bytes_extracted = 0;
        while (bytes_extracted < output_len) {
            size_t to_copy = std::min(rate_bytes, output_len - bytes_extracted);
            for (size_t i = 0; i < to_copy; i++) {
                uint8_t byte = static_cast<uint8_t>(state[i / 8] >> (8 * (i % 8)));
                output.push_back(byte);
            }
            bytes_extracted += to_copy;
            if (bytes_extracted < output_len) {
                keccak_f();
            }
        }
    }

public:
    SHA3(size_t digest_bits, size_t capacity_bits) 
        : digest_size(digest_bits / 8), 
          rate(1600 - capacity_bits),
          rate_bytes(rate / 8),
          capacity(capacity_bits) {
        reset();
    }

    void reset() {
        state.fill(0);
        buffer.clear();
    }

    void update(const uint8_t* data, size_t len) {
        size_t offset = 0;
        if (!buffer.empty()) {
            size_t to_append = std::min(rate_bytes - buffer.size(), len);
            buffer.insert(buffer.end(), data, data + to_append);
            offset = to_append;
            if (buffer.size() == rate_bytes) {
                absorb_block(buffer.data());
                buffer.clear();
            }
        }

        while (offset + rate_bytes <= len) {
            absorb_block(data + offset);
            offset += rate_bytes;
        }

        if (offset < len) {
            buffer.insert(buffer.end(), data + offset, data + len);
        }
    }

    std::vector<uint8_t> finalize() {
        pad_and_absorb();
        std::vector<uint8_t> result;
        result.reserve(digest_size);
        squeeze(result, digest_size);
        reset();
        return result;
    }

    std::vector<uint8_t> shake(size_t output_len) {
        pad_and_absorb();
        std::vector<uint8_t> result;
        squeeze(result, output_len);
        reset();
        return result;
    }

    static std::string to_hex(const std::vector<uint8_t>& bytes) {
        std::ostringstream oss;
        oss << std::hex << std::setfill('0');
        for (uint8_t b : bytes) {
            oss << std::setw(2) << static_cast<int>(b);
        }
        return oss.str();
    }

    static std::string sha3_224(const fs::path& file_path) {
        return hash_file(224, 448, file_path);
    }

    static std::string sha3_256(const fs::path& file_path) {
        return hash_file(256, 512, file_path);
    }

    static std::string sha3_384(const fs::path& file_path) {
        return hash_file(384, 768, file_path);
    }

    static std::string sha3_512(const fs::path& file_path) {
        return hash_file(512, 1024, file_path);
    }

    static std::string shake128(const fs::path& file_path, size_t output_len) {
        return hash_shake(128, 256, file_path, output_len);
    }

    static std::string shake256(const fs::path& file_path, size_t output_len) {
        return hash_shake(256, 512, file_path, output_len);
    }

private:
    static std::string hash_file(size_t digest_bits, size_t capacity_bits, const fs::path& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + file_path.string());
        }

        SHA3 hasher(digest_bits, capacity_bits);
        std::array<uint8_t, 4096> buf;
        while (file) {
            file.read(reinterpret_cast<char*>(buf.data()), buf.size());
            size_t bytes_read = file.gcount();
            if (bytes_read > 0) {
                hasher.update(buf.data(), bytes_read);
            }
        }
        return to_hex(hasher.finalize());
    }

    static std::string hash_shake(size_t rate_bits, size_t capacity_bits, const fs::path& file_path, size_t output_len) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Cannot open file: " + file_path.string());
        }

        SHA3 hasher(0, capacity_bits);
        std::array<uint8_t, 4096> buf;
        while (file) {
            file.read(reinterpret_cast<char*>(buf.data()), buf.size());
            size_t bytes_read = file.gcount();
            if (bytes_read > 0) {
                hasher.update(buf.data(), bytes_read);
            }
        }
        return to_hex(hasher.shake(output_len));
    }
};