#include "caida.h"

#include <iostream>

struct PacketHeader
{
    uint32_t second;
    uint32_t microSecond;
    uint32_t dataLen; // 当前包的数据帧长度
    uint32_t flowLen; // 离线数据长度，网络中实际数据帧的长度
};
struct IPv4Header
{
    // 20字节
    uint8_t version;
    uint8_t tos;
    uint16_t length;
    uint16_t id;
    uint16_t flag;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checkSum;
    uint32_t sourceIP;
    uint32_t destIP;
};
struct IPv6Header
{
    // 40字节
    uint32_t version_class_label;
    uint16_t payload_length;
    uint8_t next_header;
    uint8_t hop_limit;
    uint64_t source_ipv6_high;
    uint64_t source_ipv6_low;
    uint64_t dest_ipv6_high;
    uint64_t dest_ipv6_low;
};

CAIDA::CAIDA(const std::string &filePath)
    : file(filePath, std::ios::binary)
{
    if (!this->file.is_open())
    {
        std::cerr << "Error: Could not open file " << filePath << std::endl;
        return;
    }
    constexpr std::size_t pcapHeaderSize = 24 + 96;
    this->file.seekg(pcapHeaderSize, std::ios::beg);
}

Tuple CAIDA::read()
{
    if (this->file.eof())
    {
        return Tuple();
    }

    PacketHeader packetHeader;
    this->file.read(reinterpret_cast<char *>(&packetHeader), sizeof(PacketHeader));
    uint8_t version = 0;
    this->file.read(reinterpret_cast<char *>(&version), sizeof(version));
    this->file.seekg(-sizeof(version), std::ios::cur);
    const auto pos = this->file.tellg();
    version = version >> 4;

    Tuple tuple;
    if (version == 4)
    {
        IPv4Header ipv4Header;
        this->file.read(reinterpret_cast<char *>(&ipv4Header), sizeof(ipv4Header));
        // 其实这里应该做一步大小端判断，先判断设备是大端还是小端，必要时进行字节序转换
        CAIDA::byteOrderSwap(&ipv4Header.sourceIP, sizeof(ipv4Header.sourceIP));
        CAIDA::byteOrderSwap(&ipv4Header.destIP, sizeof(ipv4Header.destIP));
        tuple.version = 4;
        tuple.srcAddr = CAIDA::binaryToString(ipv4Header.sourceIP);
        tuple.dstAddr = CAIDA::binaryToString(ipv4Header.destIP);
        tuple.protocol = ipv4Header.protocol;
    }
    else if (version == 6)
    {
        IPv6Header ipv6Header;
        this->file.read(reinterpret_cast<char *>(&ipv6Header), sizeof(ipv6Header));
        tuple.version = 6;
        tuple.srcAddr = CAIDA::binaryToString(ipv6Header.source_ipv6_high, ipv6Header.source_ipv6_low);
        tuple.dstAddr = CAIDA::binaryToString(ipv6Header.dest_ipv6_high, ipv6Header.dest_ipv6_low);
        tuple.protocol = ipv6Header.next_header;
    }
    else
    {
        std::cerr << "Error, unknown IP version " << version << std::endl;
        return Tuple();
    }

    // 读取端口
    if (tuple.protocol == 6 || tuple.protocol == 17)
    {
        this->file.read(reinterpret_cast<char *>(&tuple.srcPort), sizeof(tuple.srcPort));
        this->file.read(reinterpret_cast<char *>(&tuple.dstPort), sizeof(tuple.dstPort));
        CAIDA::byteOrderSwap(&tuple.srcPort, sizeof(tuple.srcPort));
        CAIDA::byteOrderSwap(&tuple.dstPort, sizeof(tuple.dstPort));
    }
    else
    {
        tuple.srcPort = 0;
        tuple.dstPort = 0;
    }
    this->file.seekg(pos, std::ios::beg);
    this->file.seekg(packetHeader.dataLen + 16, std::ios::cur);
    return tuple;
}

std::vector<Tuple> CAIDA::read(std::size_t number)
{
    std::vector<Tuple> tuples;
    for (std::size_t i = 0; i < number; i++)
    {
        Tuple tuple = this->read();
        if (tuple.isValid())
        {
            tuples.emplace_back(std::move(tuple));
        }
    }
    return tuples;
}

void CAIDA::skip(std::size_t offset)
{
    this->file.seekg(0, std::ios::beg);
    for (std::size_t i = 0; i < offset && !this->file.eof(); i++)
    {
        this->read();
    }
}

std::string CAIDA::binaryToString(uint32_t ip) noexcept
{
    return std::to_string(ip >> 24) + "." +
           std::to_string(ip << 8 >> 24) + "." +
           std::to_string(ip << 16 >> 24) + "." +
           std::to_string(ip << 24 >> 24);
}

std::string CAIDA::binaryToString(uint64_t ipv6_high, uint64_t ipv6_low) noexcept
{
    uint16_t group[8] = {0};
    memcpy(group + 4, reinterpret_cast<uint8_t *>(&ipv6_high), 8);
    memcpy(group, reinterpret_cast<uint8_t *>(&ipv6_low), 8);
    std::string ip[8];

    for (int i = 7; i > -1; --i)
    {
        uint8_t high_byte = group[i] >> 8;
        uint8_t low_byte = group[i] << 8 >> 8;
        uint8_t nums[4] = {0};
        nums[3] = high_byte >> 4;
        nums[2] = high_byte & 0x0F;
        nums[1] = low_byte >> 4;
        nums[0] = low_byte & 0x0F;
        for (int j = 3; j > -1; --j)
        {
            if (nums[j] < 10)
            {
                ip[i] += nums[j] + '0';
            }
            else
            {
                ip[i] += nums[j] + 'a' - 10;
            }
        }
        if (group[i] != 0)
        {
            ip[i] = ip[i].substr(ip[i].find_first_not_of('0'));
        }
    }

    std::string str;
    int end = 0;
    for (int i = 7; i > -1; --i)
    {
        if (group[i] == 0)
        {
            end = i;
            for (int j = i; j > -1; --j)
            {
                end--;
                if (group[j] != 0)
                {
                    str += ":";
                    break;
                }
            }
            if (end == -1)
            {
                str += ":";
                break;
            }
        }
        else
        {
            str += ip[i] + ":";
        }
    }
    for (int i = end; i > -1; --i)
    {
        str += ip[i] + ":";
    }
    if (group[0] != 0)
    {
        str = str.substr(0, str.size() - 1);
    }
    return str;
}

void CAIDA::byteOrderSwap(void *data, size_t size) noexcept
{
    uint8_t *p = (uint8_t *)data;
    for (size_t i = 0; i < size / 2; i++)
    {
        uint8_t temp = p[i];
        p[i] = p[size - i - 1];
        p[size - i - 1] = temp;
    }
}

std::string Tuple::toString() const
{
    std::string str;
    if (version == 4)
    {
        str += "IPv4 ";
    }
    else if (version == 6)
    {
        str += "IPv6 ";
    }
    else
    {
        str += "UnknownIPversion ";
    }
    str += srcAddr + " " + dstAddr + " ";
    if (protocol == 1)
    {
        str += "ICMP";
    }
    else if (protocol == 6)
    {
        str += std::to_string(srcPort) + " " + std::to_string(dstPort) + " ";
        str += "TCP";
    }
    else if (protocol == 17)
    {
        str += std::to_string(srcPort) + " " + std::to_string(dstPort) + " ";
        str += "UDP";
    }
    else if (protocol == 50)
    {
        str += "ESP";
    }
    else if (protocol == 58)
    {
        str += "ICMPv6";
    }
    else
    {
        str += "UnknownProtocol";
    }
    return str;
}

bool Tuple::isValid() const
{
    return this->version != 0;
}