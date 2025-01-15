#include "distribute.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <unordered_map>

void flowSizeDist(const std::string &filePath, const std::string &outputPath, std::size_t number)
{
    // 检查输出路径是否为文件夹
    if (!std::filesystem::is_directory(outputPath))
    {
        std::cerr << "Output path is not a directory." << std::endl;
        return;
    }

    CAIDA io(filePath);
    std::vector<Tuple> tuples = io.read(number);
    std::cout << "Read " << tuples.size() << " tuples." << std::endl;

    // 统计每个流的大小<流ID，流大小>
    std::unordered_map<std::string, std::size_t> flowSizeCount;
    for (const auto &tuple : tuples)
    {
        flowSizeCount[tuple.toString()]++;
    }
    tuples.clear();

    // 记录下流的数量
    const std::size_t flowCount = flowSizeCount.size();
    std::cout << "Flow number = " << flowCount << std::endl;

    // 统计流大小分布<流大小，流数量>
    std::unordered_map<std::size_t, std::size_t> flowSizeDist;
    for (const auto &pair : flowSizeCount)
    {
        flowSizeDist[pair.second]++;
    }
    flowSizeCount.clear();

    std::set<std::pair<std::size_t, std::size_t>> sortedDist;
    for (const auto &pair : flowSizeDist)
    {
        sortedDist.insert(pair);
    }
    flowSizeDist.clear();

    std::string dir = outputPath + "/";

    // 写入概率密度
    std::ofstream outDensFile(dir + "flowSizeDensity.csv");
    if(!outDensFile.is_open())
    {
        std::cerr << "Failed to open output file." << std::endl;
        return;
    }
    for (const auto &pair : sortedDist)
    {
        const std::size_t flowSize = pair.first;
        const double density = static_cast<double>(pair.second) / static_cast<double>(flowCount);
        outDensFile << flowSize << "," << density << std::endl;
    }
    outDensFile.close();

    // 写入概率分布
    std::ofstream outDistFile(dir + "flowSizeDistribution.csv");
    if(!outDistFile.is_open())
    {
        std::cerr << "Failed to open output file." << std::endl;
        return;
    }
    size_t count = 0;
    for (const auto &pair : sortedDist)
    {
        count += pair.second;
        const std::size_t flowSize = pair.first;
        const double probability = static_cast<double>(count) / static_cast<double>(flowCount);
        outDistFile << flowSize << "," << probability << std::endl;
    }
    outDistFile.close();
}