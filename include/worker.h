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
#include <fstream>
#include "ring.h"
#include "file.h"

namespace FileFixer {
    class Worker{
    public:
        explicit Worker(FileFixer::Ring* ring) : ring(ring) { };

        /**
         * Manage input until an existing directory is given and change CWD to it.
         */
        void InputPathHandler() {
            fmt::print("Enter Directory path: ");
            std::cin >> inputPath;

            //Fix home shorthand
            if (inputPath.string().front() == '~') {
                prependHomeDir(inputPath);
            }

            try {
                while (true) {
                    if (!exists(inputPath)) {
                        fmt::print("{} does not exist. Try again: ", inputPath.string());
                        std::cin >> inputPath;
                    } else if (!is_directory(inputPath) || is_regular_file(inputPath)) {
                        fmt::print("Path must be to a directory: ");
                        std::cin >> inputPath;
                    } else {
                        break;
                    }
                }
                inputPath = std::filesystem::canonical(inputPath);
                std::filesystem::current_path(inputPath);
                fmt::print("CWD: {}\n", std::filesystem::current_path().string());
            }
            catch (std::filesystem::filesystem_error const& error) {
                throw std::runtime_error(fmt::format(fmt::runtime(
                        "what(): {}\n "
                        "path_(): {}\n "
                        "code().value(): {}\n"
                        "code().message(): {}\n "
                        "code().category(): {}\n"),
                        error.what(),
                        error.path1().string(),
                        error.code().message(),
                        error.code().category().name()));
            }
        }

        /**
         * Iterate a directory and its subdirectories, if any, to find unique <file size, path> pairs. Match
         * duplicate file sizes and hash to reduce files further.
         * @return a set with unique file paths.
         */
        void ProcessFiles() {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(inputPath)) {
            if (!entry.is_directory()) {
                FileFixer::File file(entry.path());
                ring->AddFD(file.fd);
                //TODO:: continue implementing registered files and buffers. Use Expose function to access ring.
                files.emplace(file.size, file.path);
            } else {
                continue;
            }
        }

        if (!files.empty()) {
            for (auto& [size, path] : files) {
                //Check for duplicate file size. No need to return the value. If needed, use find() instead.
                if (files.count(size)) {
                    auto hash {hashFile(size, path)};
                    hashes.emplace(hash, path);
                } else {
                    // If file size is unique save to set.
                    uniqueFiles.emplace(path);
                }
            }
        }

        //Hashes should be unique so add them to
        for (const auto& hash: hashes) {
            uniqueFiles.emplace(hash.second);
        }
    }

    void PrintContainerCount() {
            fmt::print("\nUnique Files Counted: {}\n", uniqueFiles.size());
        }

    private:
        FileFixer::Ring* ring;
        std::filesystem::path inputPath {};
        std::multimap<size_t, std::filesystem::path > files {};
        std::set<std::filesystem::path> uniqueFiles{};
        std::map<XXH64_hash_t, std::filesystem::path > hashes{};

        // Convert '~' to "/home/user"
        void prependHomeDir(std::filesystem::path& path) {
            //TODO: check for OS version
            std::string homeDir = getenv("HOME");
            std::string stringPath {path.string()};
            stringPath.replace(stringPath.begin(), stringPath.begin() + 1, homeDir);
            inputPath = stringPath;
        }

        static XXH64_hash_t hashFile(unsigned long fileSize, const std::filesystem::path& path) {
            //Prepare variables for hash function
            auto size{fileSize};
            auto seed{0};
            auto buffer{fileToBuffer(path)};

            return XXH64(buffer.data(), size, seed);
        }

        static std::vector<char> fileToBuffer(const std::filesystem::path& path) {

            std::ifstream source(path, std::ios::in | std::ios::binary);
                std::vector<char> buffer((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
                return buffer;
        }
    };
}
