#ifndef CAIDA_H
#define CAIDA_H

#include <cstdint>
#include <fstream>
#include <string>
#include <vector>

struct Tuple;

class CAIDA
{
public:
    CAIDA(const std::string &filePath);
    Tuple read();
    std::vector<Tuple> read(std::size_t number);

public:
    void skip(std::size_t offset);

private:
    static void byteOrderSwap(void *data, size_t size) noexcept;
    static std::string binaryToString(uint32_t ipv4) noexcept;
    static std::string binaryToString(uint64_t ipv6_high, uint64_t ipv6_low) noexcept;

private:
    std::ifstream file;
};

struct Tuple
{
    uint8_t version;
    std::string srcAddr;
    std::string dstAddr;
    uint16_t srcPort;
    uint16_t dstPort;
    uint8_t protocol;

    std::string toString() const;
    bool isValid() const;
};

#endif /* CAIDA_H */