#include "sha256.h"
#include <fstream>
#include <iomanip>
#include <sstream>
#include <array>
#include <cstdint>
#include <filesystem>

namespace fs = std::filesystem;

// 实现辅助函数
uint32_t SHA256::rotr(uint32_t x, uint32_t n) {
    return (x >> n) | (x << (32 - n));
}

uint32_t SHA256::ch(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (~x & z);
}

uint32_t SHA256::maj(uint32_t x, uint32_t y, uint32_t z) {
    return (x & y) ^ (x & z) ^ (y & z);
}

uint32_t SHA256::sigma0(uint32_t x) {
    return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22);
}

uint32_t SHA256::sigma1(uint32_t x) {
    return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25);
}

uint32_t SHA256::gamma0(uint32_t x) {
    return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3);
}

uint32_t SHA256::gamma1(uint32_t x) {
    return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10);
}

void SHA256::transform(const uint8_t* data) {
    std::array<uint32_t, 64> W = {0};

    // 将数据分成16个32位字（大端序）
    for (int i = 0; i < 16; i++) {
        W[i] = (static_cast<uint32_t>(data[i*4]) << 24) |
               (static_cast<uint32_t>(data[i*4+1]) << 16) |
               (static_cast<uint32_t>(data[i*4+2]) << 8) |
               (static_cast<uint32_t>(data[i*4+3]));
    }

    // 扩展消息
    for (int i = 16; i < 64; i++) {
        W[i] = gamma1(W[i-2]) + W[i-7] + gamma0(W[i-15]) + W[i-16];
    }

    auto a = state[0];
    auto b = state[1];
    auto c = state[2];
    auto d = state[3];
    auto e = state[4];
    auto f = state[5];
    auto g = state[6];
    auto h = state[7];

    // 主循环
    for (int i = 0; i < 64; i++) {
        uint32_t T1 = h + sigma1(e) + ch(e, f, g) + K[i] + W[i];
        uint32_t T2 = sigma0(a) + maj(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + T1;
        d = c;
        c = b;
        b = a;
        a = T1 + T2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

std::string SHA256::hash_file(const fs::path& file_path) {
    std::ifstream file(file_path, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Can't open the file: " + file_path.string());
    }

    SHA256 sha;
    std::array<uint8_t, 64> buffer = {0};
    uint64_t total_bytes = 0;
    bool processed_last_block = false;

    while (!processed_last_block) {
        file.read(reinterpret_cast<char*>(buffer.data()), 64);
        size_t bytes_read = file.gcount();
        total_bytes += bytes_read;

        if (bytes_read < 64) {
            // 处理最后一块数据
            buffer[bytes_read] = 0x80; // 填充起始位

            // 如果当前块空间不足，先处理这个块再创建新块
            if (bytes_read >= 56) {
                // 填充剩余部分为0
                for (size_t i = bytes_read + 1; i < 64; i++) {
                    buffer[i] = 0;
                }
                sha.transform(buffer.data());

                // 创建新块用于存放长度
                buffer.fill(0);
                // 在最后8字节写入位长度（大端序）
                uint64_t bit_length = total_bytes * 8;
                for (int i = 0; i < 8; i++) {
                    buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                }
                sha.transform(buffer.data());
            } else {
                // 在当前块填充0和长度
                for (size_t i = bytes_read + 1; i < 56; i++) {
                    buffer[i] = 0;
                }
                // 在最后8字节写入位长度（大端序）
                uint64_t bit_length = total_bytes * 8;
                for (int i = 0; i < 8; i++) {
                    buffer[63 - i] = static_cast<uint8_t>(bit_length >> (i * 8));
                }
                sha.transform(buffer.data());
            }
            processed_last_block = true;
        } else {
            // 处理完整数据块
            sha.transform(buffer.data());
        }
    }

    // 转换为十六进制字符串
    std::ostringstream result;
    result << std::hex << std::setfill('0');
    for (uint32_t val : sha.state) {
        result << std::setw(8) << val;
    }

    return result.str();
}