#include <array>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

class SHA2 {
protected:
    enum class Algorithm {
        SHA224, SHA256, SHA384, SHA512, SHA512_224, SHA512_256
    };

    Algorithm algo_;
    size_t block_size_;
    size_t digest_size_;

    std::array<uint32_t, 8> state32_ = {0};
    
    std::array<uint64_t, 8> state64_ = {0};
    
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
        return rotr64(x, 1) ^ rotr64(x, 8) ^ (x >> 7);
    }
    
    static uint64_t gamma1_64(uint64_t x) {
        return rotr64(x, 19) ^ rotr64(x, 61) ^ (x >> 6);
    }
    
    void init_state() {
        switch (algo_) {
            case Algorithm::SHA224:
                state32_ = {
                    0xc1059ed8, 0x367cd507, 0x3070dd17, 0xf70e5939,
                    0xffc00b31, 0x68581511, 0x64f98fa7, 0xbefa4fa4
                };
                break;
                
            case Algorithm::SHA256:
                state32_ = {
                    0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                    0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
                };
                break;
                
            case Algorithm::SHA384:
                state64_ = {
                    0xcbbb9d5dc1059ed8, 0x629a292a367cd507, 
                    0x9159015a3070dd17, 0x152fecd8f70e5939,
                    0x67332667ffc00b31, 0x8eb44a8768581511, 
                    0xdb0c2e0d64f98fa7, 0x47b5481dbefa4fa4
                };
                break;
                
            case Algorithm::SHA512:
                state64_ = {
                    0x6a09e667f3bcc908, 0xbb67ae8584caa73b, 
                    0x3c6ef372fe94f82b, 0xa54ff53a5f1d36f1,
                    0x510e527fade682d1, 0x9b05688c2b3e6c1f, 
                    0x1f83d9abfb41bd6b, 0x5be0cd19137e2179
                };
                break;
                
            case Algorithm::SHA512_224:
                state64_ = {
                    0x8C3D37C819544DA2, 0x73E1996689DCD4D6,
                    0x1DFAB7AE32FF9C82, 0x679DD514582F9FCF,
                    0x0F6D2B697BD44DA8, 0x77E36F7304C48942,
                    0x3F9D85A86A1D36C8, 0x1112E6AD91D692A1
                };
                break;
                
            case Algorithm::SHA512_256:
                state64_ = {
                    0x22312194FC2BF72C, 0x9F555FA3C84C64C2,
                    0x2393B86B6F53B151, 0x963877195940EABD,
                    0x96283EE2A88EFFE3, 0xBE5E1E2553863992,
                    0x2B0199FC2C85B8AA, 0x0EB72DDC81C52CA2
                };
                break;
        }
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

        auto a = state32_[0];
        auto b = state32_[1];
        auto c = state32_[2];
        auto d = state32_[3];
        auto e = state32_[4];
        auto f = state32_[5];
        auto g = state32_[6];
        auto h = state32_[7];

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

        state32_[0] += a;
        state32_[1] += b;
        state32_[2] += c;
        state32_[3] += d;
        state32_[4] += e;
        state32_[5] += f;
        state32_[6] += g;
        state32_[7] += h;
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

        auto a = state64_[0];
        auto b = state64_[1];
        auto c = state64_[2];
        auto d = state64_[3];
        auto e = state64_[4];
        auto f = state64_[5];
        auto g = state64_[6];
        auto h = state64_[7];

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

        state64_[0] += a;
        state64_[1] += b;
        state64_[2] += c;
        state64_[3] += d;
        state64_[4] += e;
        state64_[5] += f;
        state64_[6] += g;
        state64_[7] += h;
    }
    
    void process_file(const fs::path& file_path) {
        std::ifstream file(file_path, std::ios::binary);
        if (!file) {
            throw std::runtime_error("Can't open the file: " + file_path.string());
        }
        
        const bool is_32bit = (algo_ == Algorithm::SHA224 || algo_ == Algorithm::SHA256);
        const size_t buffer_size = is_32bit ? 64 : 128;
        const size_t length_bytes = is_32bit ? 8 : 16;
        const size_t padding_start = is_32bit ? 56 : 112;
        
        std::vector<uint8_t> buffer(buffer_size, 0);
        uint64_t total_bytes = 0;
        bool processed_last_block = false;

        while (!processed_last_block) {
            file.read(reinterpret_cast<char*>(buffer.data()), buffer_size);
            size_t bytes_read = file.gcount();
            total_bytes += bytes_read;

            if (bytes_read < buffer_size) {
                if (bytes_read < buffer_size) {
                    buffer[bytes_read] = 0x80;
                }

                if (bytes_read >= padding_start) {
                    for (size_t i = bytes_read + 1; i < buffer_size; i++) {
                        buffer[i] = 0;
                    }
                    
                    if (is_32bit) transform32(buffer.data());
                    else transform64(buffer.data());

                    buffer.assign(buffer_size, 0);
                    
                    if (is_32bit) {
                        uint64_t bit_length = total_bytes * 8;
                        for (int i = 0; i < 8; i++) {
                            buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                        }
                    } else {
                        uint64_t bit_length_high = total_bytes >> 61;
                        uint64_t bit_length_low = total_bytes << 3;
                        for (int i = 0; i < 8; i++) {
                            buffer[127 - i] = static_cast<uint8_t>(bit_length_low >> (i * 8));
                            buffer[119 - i] = static_cast<uint8_t>(bit_length_high >> (i * 8));
                        }
                    }
                    
                    if (is_32bit) transform32(buffer.data());
                    else transform64(buffer.data());
                } else {
                    for (size_t i = bytes_read + 1; i < padding_start; i++) {
                        buffer[i] = 0;
                    }
                    
                    if (is_32bit) {
                        uint64_t bit_length = total_bytes * 8;
                        for (int i = 0; i < 8; i++) {
                            buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                        }
                    } else {
                        uint64_t bit_length_high = total_bytes >> 61;
                        uint64_t bit_length_low = total_bytes << 3;
                        for (int i = 0; i < 8; i++) {
                            buffer[127 - i] = static_cast<uint8_t>(bit_length_low >> (i * 8));
                            buffer[119 - i] = static_cast<uint8_t>(bit_length_high >> (i * 8));
                        }
                    }
                    
                    if (is_32bit) transform32(buffer.data());
                    else transform64(buffer.data());
                }
                processed_last_block = true;
            } else {
                if (is_32bit) transform32(buffer.data());
                else transform64(buffer.data());
            }
        }
    }
    
    std::string get_digest() {
        std::ostringstream result;
        result << std::hex << std::setfill('0');
        
        switch (algo_) {
            case Algorithm::SHA224:
                for (size_t i = 0; i < 7; i++) {
                    result << std::setw(8) << state32_[i];
                }
                break;
                
            case Algorithm::SHA256:
                for (uint32_t val : state32_) {
                    result << std::setw(8) << val;
                }
                break;
                
            case Algorithm::SHA384:
                for (size_t i = 0; i < 6; i++) {
                    result << std::setw(16) << state64_[i];
                }
                break;
                
            case Algorithm::SHA512:
                for (uint64_t val : state64_) {
                    result << std::setw(16) << val;
                }
                break;
                
            case Algorithm::SHA512_224:
                result << std::setw(16) << (state64_[0] >> 32);
                result << std::setw(8) << (state64_[0] & 0xFFFFFFFF);
                for (size_t i = 1; i < 3; i++) {
                    result << std::setw(16) << state64_[i];
                }
                result << std::setw(8) << (state64_[3] >> 32);
                break;
                
            case Algorithm::SHA512_256:
                for (size_t i = 0; i < 4; i++) {
                    result << std::setw(16) << state64_[i];
                }
                break;
        }

        return result.str();
    }
    
public:
    SHA2(Algorithm algo) : algo_(algo) {
        switch (algo) {
            case Algorithm::SHA224:
                block_size_ = 64;
                digest_size_ = 28;
                break;
            case Algorithm::SHA256:
                block_size_ = 64;
                digest_size_ = 32;
                break;
            case Algorithm::SHA384:
                block_size_ = 128;
                digest_size_ = 48;
                break;
            case Algorithm::SHA512:
                block_size_ = 128;
                digest_size_ = 64;
                break;
            case Algorithm::SHA512_224:
                block_size_ = 128;
                digest_size_ = 28;
                break;
            case Algorithm::SHA512_256:
                block_size_ = 128;
                digest_size_ = 32;
                break;
        }
        init_state();
    }
    
    static std::string hash_file(Algorithm algo, const fs::path& file_path) {
        SHA2 hasher(algo);
        hasher.process_file(file_path);
        return hasher.get_digest();
    }
    
    static std::string sha224(const fs::path& file_path) {
        return hash_file(Algorithm::SHA224, file_path);
    }
    
    static std::string sha256(const fs::path& file_path) {
        return hash_file(Algorithm::SHA256, file_path);
    }
    
    static std::string sha384(const fs::path& file_path) {
        return hash_file(Algorithm::SHA384, file_path);
    }
    
    static std::string sha512(const fs::path& file_path) {
        return hash_file(Algorithm::SHA512, file_path);
    }
    
    static std::string sha512_224(const fs::path& file_path) {
        return hash_file(Algorithm::SHA512_224, file_path);
    }
    
    static std::string sha512_256(const fs::path& file_path) {
        return hash_file(Algorithm::SHA512_256, file_path);
    }
};