#pragma once
#include <iostream>
#include <algorithm>
#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

class Hash {
public:
    enum Algorithm {
        MD2, MD4, MD5, // 移除非标准的 MD3 和 MD6
        SHA1,
        SHA224, SHA256, SHA384, SHA512,
        SHA3_224, SHA3_256, SHA3_384, SHA3_512,
        SHAKE128, SHAKE256
    };

    static std::string hash_file(Algorithm algo, const fs::path& file_path, size_t shake_length = 0) {
        switch (algo) {
            case MD2: return MD2::hash_file(file_path);
            case MD4: return MD4::hash_file(file_path);
            case MD5: return MD5::hash_file(file_path);
            case SHA1: return SHA1::hash_file(file_path);
            case SHA224: return SHA2::hash_file(SHA2::SHA224, file_path);
            case SHA256: return SHA2::hash_file(SHA2::SHA256, file_path);
            case SHA384: return SHA2::hash_file(SHA2::SHA384, file_path);
            case SHA512: return SHA2::hash_file(SHA2::SHA512, file_path);
            case SHA3_224: return SHA3::hash_file(SHA3::SHA3_224, file_path);
            case SHA3_256: return SHA3::hash_file(SHA3::SHA3_256, file_path);
            case SHA3_384: return SHA3::hash_file(SHA3::SHA3_384, file_path);
            case SHA3_512: return SHA3::hash_file(SHA3::SHA3_512, file_path);
            case SHAKE128: return SHA3::shake128(file_path, shake_length);
            case SHAKE256: return SHA3::shake256(file_path, shake_length);
            default: throw std::invalid_argument("Unsupported hash algorithm");
        }
    }

private:
    // ==================== MD2 ====================
    class MD2 {
    private:
        static constexpr std::array<uint8_t, 272> S = {
            0x29, 0x2E, 0x43, 0xC9, 0xA2, 0xD8, 0x7C, 0x01, 0x3D, 0x36, 0x54, 0xA1, 0xEC, 0xF0, 0x06, 0x13,
            0x62, 0xA7, 0x05, 0xF3, 0xC0, 0xC7, 0x73, 0x8C, 0x98, 0x93, 0x2B, 0xD9, 0xBC, 0x4C, 0x82, 0xCA,
            0x1E, 0x9B, 0x57, 0x3C, 0xFD, 0xD4, 0xE0, 0x16, 0x67, 0x42, 0x6F, 0x18, 0x8A, 0x17, 0xE5, 0x12,
            0xBE, 0x4E, 0xC4, 0xD6, 0xDA, 0x9E, 0xDE, 0x49, 0xA0, 0xFB, 0xF5, 0x8E, 0xBB, 0x2F, 0xEE, 0x7A,
            0xA9, 0x68, 0x79, 0x91, 0x15, 0xB2, 0x07, 0x3F, 0x94, 0xC2, 0x10, 0x89, 0x0B, 0x22, 0x5F, 0x21,
            0x80, 0x7F, 0x5D, 0x9A, 0x5A, 0x90, 0x32, 0x27, 0x35, 0x3E, 0xCC, 0xE7, 0xBF, 0xF7, 0x97, 0x03,
            0xFF, 0x19, 0x30, 0xB3, 0x48, 0xA5, 0xB5, 0xD1, 0xD7, 0x5E, 0x92, 0x2A, 0xAC, 0x56, 0xAA, 0xC6,
            0x4F, 0xB8, 0x38, 0xD2, 0x96, 0xA4, 0x7D, 0xB6, 0x76, 0xFC, 0x6B, 0xE2, 0x9C, 0x74, 0x04, 0xF1,
            0x45, 0x9D, 0x70, 0x59, 0x64, 0x71, 0x87, 0x20, 0x86, 0x5B, 0xCF, 0x65, 0xE6, 0x2D, 0xA8, 0x02,
            0x1B, 0x60, 0x25, 0xAD, 0xAE, 0xB0, 0xB9, 0xF6, 0x1C, 0x46, 0x61, 0x69, 0x34, 0x40, 0x7E, 0x0F,
            0x55, 0x47, 0xA3, 0x23, 0xDD, 0x51, 0xAF, 0x3A, 0x11, 0x17, 0x7B, 0x79, 0x77, 0x1F, 0xDB, 0x3B,
            0xBA, 0xCE, 0xE4, 0x8B, 0xED, 0x5C, 0x1D, 0x28, 0x78, 0x26, 0x1A, 0xDC, 0x4D, 0x24, 0x72, 0x8F,
            0xEA, 0x4A, 0x6A, 0x31, 0xC8, 0x0C, 0x37, 0x0E, 0x52, 0x6C, 0x2C, 0x84, 0x09, 0xBD, 0x44, 0x8D,
            0x08, 0x15, 0x6E, 0xEB, 0xF4, 0x50, 0xC1, 0xE3, 0x75, 0xEF, 0x34, 0x95, 0x0A, 0xAB, 0x49, 0x81,
            0xFE, 0xDF, 0x53, 0xE9, 0xF2, 0x6D, 0x13, 0x58, 0x99, 0x0D, 0xCD, 0xE1, 0xF8, 0x41, 0x66, 0xE8,
            0xB7, 0xC5, 0x88, 0xB4, 0x9F, 0x14, 0x85, 0x4B, 0xBB, 0xA6, 0xD3, 0x2B, 0x39, 0x7C, 0xF9, 0x63,
            0xC3, 0x27, 0x6F, 0x33, 0x67, 0x8E, 0x76, 0xFA, 0x83, 0xB1, 0x5F, 0x69, 0x3B, 0x4C, 0x00, 0xFB
        };

    public:
        static std::string hash_file(const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Can't open file: " + file_path.string());

            std::array<uint8_t, 48> state = {0};
            std::array<uint8_t, 16> checksum = {0};
            uint64_t total_bytes = 0;

            while (file) {
                std::array<uint8_t, 16> block = {0};
                file.read(reinterpret_cast<char*>(block.data()), 16);
                size_t bytes_read = file.gcount();
                total_bytes += bytes_read;

                if (bytes_read < 16) {
                    size_t padding = 16 - bytes_read;
                    for (size_t i = bytes_read; i < 16; i++) {
                        block[i] = static_cast<uint8_t>(padding);
                    }
                    process_block(block.data(), state, checksum);
                    break;
                }
                process_block(block.data(), state, checksum);
            }

            process_block(checksum.data(), state, checksum);

            std::ostringstream result;
            for (int i = 0; i < 16; i++) {
                result << std::hex << std::setw(2) << std::setfill('0')
                       << static_cast<int>(state[i]);
            }
            return result.str();
        }

    private:
        static void process_block(const uint8_t* block, std::array<uint8_t, 48>& state, std::array<uint8_t, 16>& checksum) {
            for (int j = 0; j < 16; j++) {
                state[16 + j] = block[j];
                state[32 + j] = state[16 + j] ^ state[j];
            }

            uint8_t t = 0;
            for (int round = 0; round < 18; round++) {
                for (int j = 0; j < 48; j++) {
                    t = state[j] = state[j] ^ S[t];
                }
                t = (t + round) & 0xFF;
            }

            uint8_t c = 0;
            for (int j = 0; j < 16; j++) {
                c = checksum[j] ^ S[block[j] ^ c];
                checksum[j] = c;
            }
        }
    };

    // ==================== MD4 ====================
    class MD4 {
    private:
        static constexpr std::array<uint32_t, 64> SHIFTS = {
            3, 7, 11, 19, 3, 5, 9, 13, 3, 9, 11, 15,
            3, 7, 11, 19, 3, 5, 9, 13, 3, 9, 11, 15,
            3, 7, 11, 19, 3, 5, 9, 13, 3, 9, 11, 15,
            3, 7, 11, 19, 3, 5, 9, 13, 3, 9, 11, 15,
            3, 7, 11, 19, 3, 5, 9, 13, 3, 9, 11, 15
        };

    public:
        static std::string hash_file(const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Can't open file: " + file_path.string());

            std::array<uint32_t, 4> state = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
            std::array<uint8_t, 64> buffer = {0};
            uint64_t total_bytes = 0;

            while (file) {
                file.read(reinterpret_cast<char*>(buffer.data()), 64);
                size_t bytes_read = file.gcount();
                total_bytes += bytes_read;

                if (bytes_read < 64) {
                    buffer[bytes_read] = 0x80;
                    for (size_t i = bytes_read + 1; i < 56; i++) {
                        buffer[i] = 0;
                    }

                    uint64_t bit_length = total_bytes * 8;
                    for (int i = 0; i < 8; i++) {
                        buffer[56 + i] = static_cast<uint8_t>(bit_length >> (i * 8));
                    }

                    process_block(buffer.data(), state);
                    break;
                }
                process_block(buffer.data(), state);
            }

            std::ostringstream result;
            for (uint32_t val : state) {
                for (int i = 0; i < 4; i++) {
                    result << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<int>((val >> (i * 8)) & 0xFF);
                }
            }
            return result.str();
        }

    private:
        static uint32_t rotl32(uint32_t x, uint32_t n) {
            return (x << n) | (x >> (32 - n));
        }

        static void process_block(const uint8_t* block, std::array<uint32_t, 4>& state) {
            std::array<uint32_t, 16> X;
            for (int i = 0; i < 16; i++) {
                X[i] = (block[i*4+3] << 24) | (block[i*4+2] << 16) | (block[i*4+1] << 8) | block[i*4];
            }

            uint32_t A = state[0], B = state[1], C = state[2], D = state[3];

            // Round 1
            for (int i = 0; i < 16; i++) {
                uint32_t F = (B & C) | ((~B) & D);
                uint32_t g = i;
                uint32_t T = A + F + X[g];
                A = D;
                D = C;
                C = B;
                B = rotl32(T, SHIFTS[i]);
            }

            // Round 2
            for (int i = 16; i < 32; i++) {
                uint32_t F = (B & C) | (B & D) | (C & D);
                uint32_t g = (i % 4) * 4 + (i / 4);
                uint32_t T = A + F + X[g] + 0x5A827999;
                A = D;
                D = C;
                C = B;
                B = rotl32(T, SHIFTS[i]);
            }

            // Round 3
            for (int i = 32; i < 48; i++) {
                uint32_t F = B ^ C ^ D;
                uint32_t g = (i % 4 == 0) ? 0 : (i % 4 == 1) ? 8 : (i % 4 == 2) ? 4 : 12;
                g += i / 4;
                uint32_t T = A + F + X[g] + 0x6ED9EBA1;
                A = D;
                D = C;
                C = B;
                B = rotl32(T, SHIFTS[i]);
            }

            state[0] += A;
            state[1] += B;
            state[2] += C;
            state[3] += D;
        }
    };

    // ==================== MD5 ====================
    class MD5 {
    private:
        static constexpr std::array<uint32_t, 64> T = {
            0xD76AA478, 0xE8C7B756, 0x242070DB, 0xC1BDCEEE,
            0xF57C0FAF, 0x4787C62A, 0xA8304613, 0xFD469501,
            0x698098D8, 0x8B44F7AF, 0xFFFF5BB1, 0x895CD7BE,
            0x6B901122, 0xFD987193, 0xA679438E, 0x49B40821,
            0xF61E2562, 0xC040B340, 0x265E5A51, 0xE9B6C7AA,
            0xD62F105D, 0x02441453, 0xD8A1E681, 0xE7D3FBC8,
            0x21E1CDE6, 0xC33707D6, 0xF4D50D87, 0x455A14ED,
            0xA9E3E905, 0xFCEFA3F8, 0x676F02D9, 0x8D2A4C8A,
            0xFFFA3942, 0x8771F681, 0x6D9D6122, 0xFDE5380C,
            0xA4BEEA44, 0x4BDECFA9, 0xF6BB4B60, 0xBEBFBC70,
            0x289B7EC6, 0xEAA127FA, 0xD4EF3085, 0x04881D05,
            0xD9D4D039, 0xE6DB99E5, 0x1FA27CF8, 0xC4AC5665,
            0xF4292244, 0x432AFF97, 0xAB9423A7, 0xFC93A039,
            0x655B59C3, 0x8F0CCC92, 0xFFEFF47D, 0x85845DD1,
            0x6FA87E4F, 0xFE2CE6E0, 0xA3014314, 0x4E0811A1,
            0xF7537E82, 0xBD3AF235, 0x2AD7D2BB, 0xEB86D391
        };

        static constexpr std::array<uint32_t, 64> SHIFTS = {
            7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22, 7, 12, 17, 22,
            5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20, 5, 9, 14, 20,
            4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23, 4, 11, 16, 23,
            6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21, 6, 10, 15, 21
        };

    public:
        static std::string hash_file(const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Can't open file: " + file_path.string());

            std::array<uint32_t, 4> state = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476};
            std::array<uint8_t, 64> buffer = {0};
            uint64_t total_bytes = 0;

            while (file) {
                file.read(reinterpret_cast<char*>(buffer.data()), 64);
                size_t bytes_read = file.gcount();
                total_bytes += bytes_read;

                if (bytes_read < 64) {
                    buffer[bytes_read] = 0x80;
                    for (size_t i = bytes_read + 1; i < 56; i++) {
                        buffer[i] = 0;
                    }

                    uint64_t bit_length = total_bytes * 8;
                    for (int i = 0; i < 8; i++) {
                        buffer[56 + i] = static_cast<uint8_t>(bit_length >> (i * 8));
                    }

                    process_block(buffer.data(), state);
                    break;
                }
                process_block(buffer.data(), state);
            }

            std::ostringstream result;
            for (uint32_t val : state) {
                for (int i = 0; i < 4; i++) {
                    result << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<int>((val >> (i * 8)) & 0xFF);
                }
            }
            return result.str();
        }

    private:
        static uint32_t rotl32(uint32_t x, uint32_t n) {
            return (x << n) | (x >> (32 - n));
        }

        static void process_block(const uint8_t* block, std::array<uint32_t, 4>& state) {
            std::array<uint32_t, 16> X;
            for (int i = 0; i < 16; i++) {
                X[i] = (block[i*4+3] << 24) | (block[i*4+2] << 16) | (block[i*4+1] << 8) | block[i*4];
            }

            uint32_t A = state[0], B = state[1], C = state[2], D = state[3];

            for (int i = 0; i < 64; i++) {
                uint32_t F, g;
                if (i < 16) {
                    F = (B & C) | ((~B) & D);
                    g = i;
                } else if (i < 32) {
                    F = (D & B) | ((~D) & C);
                    g = (5*i + 1) % 16;
                } else if (i < 48) {
                    F = B ^ C ^ D;
                    g = (3*i + 5) % 16;
                } else {
                    F = C ^ (B | (~D));
                    g = (7*i) % 16;
                }

                F = F + A + T[i] + X[g];
                A = D;
                D = C;
                C = B;
                B = B + rotl32(F, SHIFTS[i]);
            }

            state[0] += A;
            state[1] += B;
            state[2] += C;
            state[3] += D;
        }
    };

    // ==================== SHA-1 ====================
    class SHA1 {
    private:
        static constexpr std::array<uint32_t, 4> K = {
            0x5A827999, 0x6ED9EBA1, 0x8F1BBCDC, 0xCA62C1D6
        };

    public:
        static std::string hash_file(const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Can't open file: " + file_path.string());

            std::array<uint32_t, 5> state = {0x67452301, 0xEFCDAB89, 0x98BADCFE, 0x10325476, 0xC3D2E1F0};
            std::array<uint8_t, 64> buffer = {0};
            uint64_t total_bytes = 0;

            while (file) {
                file.read(reinterpret_cast<char*>(buffer.data()), 64);
                size_t bytes_read = file.gcount();
                total_bytes += bytes_read;

                if (bytes_read < 64) {
                    buffer[bytes_read] = 0x80;
                    for (size_t i = bytes_read + 1; i < 56; i++) {
                        buffer[i] = 0;
                    }

                    uint64_t bit_length = total_bytes * 8;
                    for (int i = 0; i < 8; i++) {
                        buffer[63 - i] = static_cast<uint8_t>(bit_length >> (56 - i * 8));
                    }

                    process_block(buffer.data(), state);
                    break;
                }
                process_block(buffer.data(), state);
            }

            std::ostringstream result;
            for (uint32_t val : state) {
                for (int i = 3; i >= 0; i--) {
                    result << std::hex << std::setw(2) << std::setfill('0')
                           << static_cast<int>((val >> (i * 8)) & 0xFF);
                }
            }
            return result.str();
        }

    private:
        static uint32_t rotl32(uint32_t x, uint32_t n) {
            return (x << n) | (x >> (32 - n));
        }

        static void process_block(const uint8_t* block, std::array<uint32_t, 5>& state) {
            std::array<uint32_t, 80> W = {0};
            for (int t = 0; t < 16; t++) {
                W[t] = (block[t*4] << 24) | (block[t*4+1] << 16) | (block[t*4+2] << 8) | block[t*4+3];
            }

            for (int t = 16; t < 80; t++) {
                W[t] = rotl32(W[t-3] ^ W[t-8] ^ W[t-14] ^ W[t-16], 1);
            }

            uint32_t A = state[0], B = state[1], C = state[2], D = state[3], E = state[4];

            for (int t = 0; t < 80; t++) {
                uint32_t F;
                if (t < 20) {
                    F = (B & C) | ((~B) & D);
                } else if (t < 40) {
                    F = B ^ C ^ D;
                } else if (t < 60) {
                    F = (B & C) | (B & D) | (C & D);
                } else {
                    F = B ^ C ^ D;
                }

                uint32_t temp = rotl32(A, 5) + F + E + K[t/20] + W[t];
                E = D;
                D = C;
                C = rotl32(B, 30);
                B = A;
                A = temp;
            }

            state[0] += A;
            state[1] += B;
            state[2] += C;
            state[3] += D;
            state[4] += E;
        }
    };

    // ==================== SHA2 ====================
    class SHA2 {
    public:
        enum Algorithm {
            SHA224, SHA256, SHA384, SHA512
        };

        static std::string hash_file(Algorithm algo, const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Can't open file: " + file_path.string());

            SHA2 hasher(algo);
            std::vector<uint8_t> buffer(hasher.block_size);
            uint64_t total_bytes = 0;
            bool processed_last_block = false;

            while (!processed_last_block) {
                file.read(reinterpret_cast<char*>(buffer.data()), hasher.block_size);
                size_t bytes_read = file.gcount();
                total_bytes += bytes_read;

                if (bytes_read < hasher.block_size) {
                    if (bytes_read < hasher.block_size) {
                        buffer[bytes_read] = 0x80;
                    }

                    if (bytes_read >= hasher.padding_start) {
                        for (size_t i = bytes_read + 1; i < hasher.block_size; i++) {
                            buffer[i] = 0;
                        }

                        if (hasher.is_32bit) hasher.transform32(buffer.data());
                        else hasher.transform64(buffer.data());

                        buffer.assign(hasher.block_size, 0);

                        if (hasher.is_32bit) {
                            uint64_t bit_length = total_bytes * 8;
                            for (int i = 0; i < 8; i++) {
                                buffer[hasher.block_size - 1 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                            }
                        } else {
                            uint64_t bit_length_high = total_bytes >> 61;
                            uint64_t bit_length_low = total_bytes << 3;
                            for (int i = 0; i < 8; i++) {
                                buffer[hasher.block_size - 1 - i] = static_cast<uint8_t>(bit_length_low >> (i * 8));
                                buffer[hasher.block_size - 9 - i] = static_cast<uint8_t>(bit_length_high >> (i * 8));
                            }
                        }

                        if (hasher.is_32bit) hasher.transform32(buffer.data());
                        else hasher.transform64(buffer.data());
                    } else {
                        for (size_t i = bytes_read + 1; i < hasher.padding_start; i++) {
                            buffer[i] = 0;
                        }

                        if (hasher.is_32bit) {
                            uint64_t bit_length = total_bytes * 8;
                            for (int i = 0; i < 8; i++) {
                                buffer[hasher.block_size - 1 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                            }
                        } else {
                            uint64_t bit_length_high = total_bytes >> 61;
                            uint64_t bit_length_low = total_bytes << 3;
                            for (int i = 0; i < 8; i++) {
                                buffer[hasher.block_size - 1 - i] = static_cast<uint8_t>(bit_length_low >> (i * 8));
                                buffer[hasher.block_size - 9 - i] = static_cast<uint8_t>(bit_length_high >> (i * 8));
                            }
                        }

                        if (hasher.is_32bit) hasher.transform32(buffer.data());
                        else hasher.transform64(buffer.data());
                    }
                    processed_last_block = true;
                } else {
                    if (hasher.is_32bit) hasher.transform32(buffer.data());
                    else hasher.transform64(buffer.data());
                }
            }

            return hasher.get_digest();
        }

    private:
        Algorithm algo_;
        bool is_32bit;
        size_t block_size;
        size_t digest_size;
        size_t padding_start;

        std::array<uint32_t, 8> state32;
        std::array<uint64_t, 8> state64;

        static constexpr std::array<uint32_t, 64> K32 = {
            0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
            0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
            0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
            0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
            0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
            0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
            0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
            0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
        };

        static constexpr std::array<uint64_t, 80> K64 = {
            0x428a2f98d728ae22, 0x7137449123ef65cd, 0xb5c0fbcfec4d3b2f, 0xe9b5dba58189dbbc,
            0x3956c25bf348b538, 0x59f111f1b605d019, 0x923f82a4af194f9b, 0xab1c5ed5da6d8118,
            0xd807aa98a3030242, 0x12835b0145706fbe, 0x243185be4ee4b28c, 0x550c7dc3d5ffb4e2,
            0x72be5d74f27b896f, 0x80deb1fe3b1696b1, 0x9bdc06a725c71235, 0xc19bf174cf692694,
            0xe49b69c19ef14ad2, 0xefbe4786384f25e3, 0x0fc19dc68b8cd5b5, 0x240ca1cc77ac9c65,
            0x2de92c6f592b0275, 0x4a7484aa6ea6e483, 0x5cb0a9dcbd41fbd4, 0x76f988da831153b5,
            0x983e5152ee66dfab, 0xa831c66d2db43210, 0xb00327c898fb213f, 0xbf597fc7beef0ee4,
            0xc6e00bf33da88fc2, 0xd5a79147930aa725, 0x06ca6351e003826f, 0x142929670a0e6e70,
            0x27b70a8546d22ffc, 0x2e1b21385c26c926, 0x4d2c6dfc5ac42aed, 0x53380d139d95b3df,
            0x650a73548baf63de, 0x766a0abb3c77b2a8, 0x81c2c92e47edaee6, 0x92722c851482353b,
            0xa2bfe8a14cf10364, 0xa81a664bbc423001, 0xc24b8b70d0f89791, 0xc76c51a30654be30,
            0xd192e819d6ef5218, 0xd69906245565a910, 0xf40e35855771202a, 0x106aa07032bbd1b8,
            0x19a4c116b8d2d0c8, 0x1e376c085141ab53, 0x2748774cdf8eeb99, 0x34b0bcb5e19b48a8,
            0x391c0cb3c5c95a63, 0x4ed8aa4ae3418acb, 0x5b9cca4f7763e373, 0x682e6ff3d6b2b8a3,
            0x748f82ee5defb2fc, 0x78a5636f43172f60, 0x84c87814a1f0ab72, 0x8cc702081a6439ec,
            0x90befffa23631e28, 0xa4506cebde82bde9, 0xbef9a3f7b2c67915, 0xc67178f2e372532b,
            0xca273eceea26619c, 0xd186b8c721c0c207, 0xeada7dd6cde0eb1e, 0xf57d4f7fee6ed178,
            0x06f067aa72176fba, 0x0a637dc5a2c898a6, 0x113f9804bef90dae, 0x1b710b35131c471b,
            0x28db77f523047d84, 0x32caab7b40c72493, 0x3c9ebe0a15c9bebc, 0x431d67c49c100d4c,
            0x4cc5d4becb3e42b6, 0x597f299cfc657e2a, 0x5fcb6fab3ad6faec, 0x6c44198c4a475817
        };

        SHA2(Algorithm algo) : algo_(algo) {
            switch (algo) {
                case SHA224:
                    is_32bit = true;
                    block_size = 64;
                    digest_size = 28;
                    padding_start = 56;
                    state32 = {
                        0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
                        0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
                    };
                    break;

                case SHA256:
                    is_32bit = true;
                    block_size = 64;
                    digest_size = 32;
                    padding_start = 56;
                    state32 = {
                        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
                    };
                    break;

                case SHA384:
                    is_32bit = false;
                    block_size = 128;
                    digest_size = 48;
                    padding_start = 112;
                    state64 = {
                        0xcbbb9d5dc1059ed8, 0x629a292a367cd507,
                        0x9159015a3070dd17, 0x152fecd8f70e5939,
                        0x67332667ffc00b31, 0x8eb44a8768581511,
                        0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4
                    };
                    break;

                case SHA512:
                    is_32bit = false;
                    block_size = 128;
                    digest_size = 64;
                    padding_start = 112;
                    state64 = {
                        0x6a09e667f3bcc908, 0xbb67ae8584caa73b,
                        0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                        0x510e527fade682d1, 0x9b05688c2b3e6c1f,
                        0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
                    };
                    break;
            }
        }

        static uint32_t rotr32(uint32_t x, uint32_t n) {
            return (x >> n) | (x << (32 - n));
        }

        static uint32_t ch32(uint32_t x, uint32_t y, uint32_t z) {
            return (x & y) ^ (~x & z);
        }

        static uint32_t maj32(uint32_t x, uint32_t y, uint32_t z) {
            return (x & y) ^ (x & z) ^ (y & z);
        }

        static uint32_t sigma0_32(uint32_t x) {
            return rotr32(x, 2) ^ rotr32(x, 13) ^ rotr32(x, 22);
        }

        static uint32_t sigma1_32(uint32_t x) {
            return rotr32(x, 6) ^ rotr32(x, 11) ^ rotr32(x, 25);
        }

        static uint32_t gamma0_32(uint32_t x) {
            return rotr32(x, 7) ^ rotr32(x, 18) ^ (x >> 3);
        }

        static uint32_t gamma1_32(uint32_t x) {
            return rotr32(x, 17) ^ rotr32(x, 19) ^ (x >> 10);
        }

        static uint64_t rotr64(uint64_t x, uint32_t n) {
            return (x >> n) | (x << (64 - n));
        }

        static uint64_t ch64(uint64_t x, uint64_t y, uint64_t z) {
            return (x & y) ^ (~x & z);
        }

        static uint64_t maj64(uint64_t x, uint64_t y, uint64_t z) {
            return (x & y) ^ (x & z) ^ (y & z);
        }

        static uint64_t sigma0_64(uint64_t x) {
            return rotr64(x, 28) ^ rotr64(x, 34) ^ rotr64(x, 39);
        }

        static uint64_t sigma1_64(uint64_t x) {
            return rotr64(x, 14) ^ rotr64(x, 18) ^ rotr64(x, 41);
        }

        static uint64_t gamma0_64(uint64_t x) {
            return rotr64(x, 7) ^ rotr64(x, 18) ^ (x >> 3);
        }

        static uint64_t gamma1_64(uint64_t x) {
            return rotr64(x, 17) ^ rotr64(x, 19) ^ (x >> 10);
        }

        void transform32(const uint8_t* data) {
            std::array<uint32_t, 64> W = {0};

            for (int i = 0; i < 16; i++) {
                W[i] = (static_cast<uint32_t>(data[i*4]) << 24) |
                       (static_cast<uint32_t>(data[i*4+1]) << 16) |
                       (static_cast<uint32_t>(data[i*4+2]) << 8) |
                       (static_cast<uint32_t>(data[i*4+3]));
            }

            for (int i = 16; i < 64; i++) {
                W[i] = gamma1_32(W[i-2]) + W[i-7] + gamma0_32(W[i-15]) + W[i-16];
            }

            auto a = state32[0];
            auto b = state32[1];
            auto c = state32[2];
            auto d = state32[3];
            auto e = state32[4];
            auto f = state32[5];
            auto g = state32[6];
            auto h = state32[7];

            for (int i = 0; i < 64; i++) {
                uint32_t T1 = h + sigma1_32(e) + ch32(e, f, g) + K32[i] + W[i];
                uint32_t T2 = sigma0_32(a) + maj32(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + T1;
                d = c;
                c = b;
                b = a;
                a = T1 + T2;
            }

            state32[0] += a;
            state32[1] += b;
            state32[2] += c;
            state32[3] += d;
            state32[4] += e;
            state32[5] += f;
            state32[6] += g;
            state32[7] += h;
        }

        void transform64(const uint8_t* data) {
            std::array<uint64_t, 80> W = {0};

            for (int i = 0; i < 16; i++) {
                W[i] = (static_cast<uint64_t>(data[i*8]) << 56) |
                       (static_cast<uint64_t>(data[i*8+1]) << 48) |
                       (static_cast<uint64_t>(data[i*8+2]) << 40) |
                       (static_cast<uint64_t>(data[i*8+3]) << 32) |
                       (static_cast<uint64_t>(data[i*8+4]) << 24) |
                       (static_cast<uint64_t>(data[i*8+5]) << 16) |
                       (static_cast<uint64_t>(data[i*8+6]) << 8) |
                       (static_cast<uint64_t>(data[i*8+7]));
            }

            for (int i = 16; i < 80; i++) {
                W[i] = gamma1_64(W[i-2]) + W[i-7] + gamma0_64(W[i-15]) + W[i-16];
            }

            auto a = state64[0];
            auto b = state64[1];
            auto c = state64[2];
            auto d = state64[3];
            auto e = state64[4];
            auto f = state64[5];
            auto g = state64[6];
            auto h = state64[7];

            for (int i = 0; i < 80; i++) {
                uint64_t T1 = h + sigma1_64(e) + ch64(e, f, g) + K64[i] + W[i];
                uint64_t T2 = sigma0_64(a) + maj64(a, b, c);
                h = g;
                g = f;
                f = e;
                e = d + T1;
                d = c;
                c = b;
                b = a;
                a = T1 + T2;
            }

            state64[0] += a;
            state64[1] += b;
            state64[2] += c;
            state64[3] += d;
            state64[4] += e;
            state64[5] += f;
            state64[6] += g;
            state64[7] += h;
        }

        std::string get_digest() {
            std::ostringstream result;
            result << std::hex << std::setfill('0');

            switch (algo_) {
                case SHA224:
                    for (size_t i = 0; i < 7; i++) {
                        result << std::setw(8) << state32[i];
                    }
                    break;

                case SHA256:
                    for (uint32_t val : state32) {
                        result << std::setw(8) << val;
                    }
                    break;

                case SHA384:
                    for (size_t i = 0; i < 6; i++) {
                        result << std::setw(16) << state64[i];
                    }
                    break;

                case SHA512:
                    for (uint64_t val : state64) {
                        result << std::setw(16) << val;
                    }
                    break;
            }

            return result.str();
        }
    };

    // ==================== SHA3 ====================
    class SHA3 {
    public:
        enum Algorithm {
            SHA3_224, SHA3_256, SHA3_384, SHA3_512
        };

        static std::string hash_file(Algorithm algo, const fs::path& file_path) {
            size_t digest_bits, capacity_bits;
            switch (algo) {
                case SHA3_224: digest_bits = 224; capacity_bits = 448; break;
                case SHA3_256: digest_bits = 256; capacity_bits = 512; break;
                case SHA3_384: digest_bits = 384; capacity_bits = 768; break;
                case SHA3_512: digest_bits = 512; capacity_bits = 1024; break;
            }
            return hash_file(digest_bits, capacity_bits, file_path);
        }

        static std::string shake128(const fs::path& file_path, size_t output_len) {
            return hash_shake(128, 256, file_path, output_len);
        }

        static std::string shake256(const fs::path& file_path, size_t output_len) {
            return hash_shake(256, 512, file_path, output_len);
        }

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

        static std::string hash_file(size_t digest_bits, size_t capacity_bits, const fs::path& file_path) {
            std::ifstream file(file_path, std::ios::binary);
            if (!file) throw std::runtime_error("Cannot open file: " + file_path.string());

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
            if (!file) throw std::runtime_error("Cannot open file: " + file_path.string());

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

    private:
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

            // 标准填充：添加 0x06 对于 SHA3，0x1F 对于 SHAKE
            uint8_t suffix = (digest_size > 0) ? 0x06 : 0x1F;

            buffer.push_back(suffix);
            while (buffer.size() % block_size != block_size - 1) {
                buffer.push_back(0x00);
            }
            buffer.back() |= 0x80;

            absorb_block(buffer.data());
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
    };
};
