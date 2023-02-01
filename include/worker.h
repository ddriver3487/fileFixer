//
// Created by xavier on 1/27/23.
//

#pragma once

#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <vector>
#include <xxh3.h>
#include <set>

namespace FileFixer {
    class Worker{
    public:
        Worker() = default;

        void ValidateUserInput();
        void ProcessFiles();
        void PrintContainerCount();

    private:

        std::filesystem::path inputPath {};
        std::multimap<size_t, std::filesystem::path > files {};
        std::set<std::filesystem::path> uniqueFiles{};
        std::map<XXH64_hash_t, std::filesystem::path > hashes{};
        // Convert '~' to "/home/user"
        void prependHomeDir(std::filesystem::path& path);
        static XXH64_hash_t hashFile(unsigned long fileSize, const std::filesystem::path& path);
        static std::vector<char> fileToBuffer(const std::filesystem::path& path);
    };
}
