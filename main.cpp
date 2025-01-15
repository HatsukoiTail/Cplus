#include <iostream>

#include "distribute.h"

int main()
{
    const std::string CAIDA_PATH("C:/Users/Administrator/Desktop/DataSet/Packet/Orignal/1.pcap");
    const std::string OUTPUT_PATH("C:/Users/Administrator/Desktop/Project/CAIDA/");
    constexpr std::size_t PACKET_NUMBER = 6e6;

    flowSizeDist(CAIDA_PATH, OUTPUT_PATH, PACKET_NUMBER);
    return 0;
}