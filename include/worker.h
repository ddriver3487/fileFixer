//
// Created by xavier on 1/27/23.
//

#pragma once

#include <filesystem>
#include <fmt/core.h>
#include <iostream>
#include <unistd.h>
#include <map>
#include <utility>
#include <vector>
#include <xxh3.h>
#include <set>
#include <fstream>
#include <algorithm>
#include "ring.h"
#include "file.h"

namespace FileFixer {
    class Worker{
    public:
        explicit Worker() = default;

        /**
         * Manage input until an existing directory is given and change CWD to it.
         */
        void InputPathHandler(std::filesystem::path& path) {
//            fmt::print("Enter Directory path: ");
//            std::cin >> inputPath;
            inputPath = path;
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
        void ProcessFiles(FileFixer::Ring* fxRing) {
            for (const auto& entry : std::filesystem::recursive_directory_iterator(inputPath)) {
                if (entry.is_regular_file()) {
                    //Open file
                    FileFixer::File file(entry.path());
                    files.emplace(file.size, std::make_pair(file.fd, file.path));
                } else {
                    continue;
                }
            }

        if (!files.empty()) {
            for (auto& [size, fileInfo] : files) {
                //Check for duplicate file size. No need to return the value. If needed, use find() instead.
                if (files.count(size)) {
                    bufferAndPath.emplace_back(fileBuffer(size, fxRing), fileInfo.second);
                } else {
                    // If file size is unique save path to set.
                    uniqueFiles.emplace(fileInfo.second);
                }
            }
        }
        //TODO: add hash function here.

        for (auto& [buffer, path] : bufferAndPath) {
            hashes.emplace(hashFile(buffer, buffer.size()), path);
        }
        //Hashes should be unique so add them to set.
        for (const auto& hash: hashes) {
            uniqueFiles.emplace(hash.second);
        }
    }

    void PrintContainerCount() {
            fmt::print("\nUnique Files Counted: {}\n", uniqueFiles.size());
        }

    private:
        std::filesystem::path inputPath {};
        std::multimap<size_t, std::pair<int, std::filesystem::path>> files {};
        std::vector<std::pair<std::vector<char>, std::filesystem::path>> bufferAndPath;
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

       static std::vector<char> fileBuffer(unsigned long fileSize, FileFixer::Ring* ring) {
            unsigned long blockSize {1024};
            unsigned long bytesRead {0};
            unsigned long bytesWritten {0};
            unsigned long bytesRemaining {fileSize};
            unsigned long thisOffset {0};

            std::vector<char> buffer;
            buffer.reserve(fileSize);

            //Read until finished.
            while (bytesRead < fileSize) {
                //Queue up as many reads as possible.
                while (bytesRemaining) {
                    if (bytesRemaining > blockSize) {
                        blockSize = blockSize;
                    } else {
                        blockSize = bytesRemaining;
                    }

                    auto sqe { FileFixer::Ring::GetSQE() };

                    //SQ is full.
                    if (sqe.get() == nullptr) {
                        break;
                    }

                    auto data { ring->GetIoData() };
                    //TODO:Check fd in prepQueue
                    if (data.has_value()) {
                        FileFixer::Ring::PrepQueue(sqe.get(), data.value(), blockSize);

                        if (data.value()->type == ioType::read) {
                            bytesRead += blockSize;
                            thisOffset += blockSize;
                            bytesRemaining -= blockSize;
                        }
                    } else {
                        continue;
                    }

                }

                ring->Submit();

                //Find at least one completion.
                while (bytesRead <= fileSize) {
                    auto completion = Completion(ring->Expose()).Peek();
                    auto sqe{FileFixer::Ring::GetSQE()};

                    //SQ is full.
                    if (sqe.get() == nullptr) {
                        break;
                    }

                    if (completion.has_value()) {
                        if (completion->cqe->res == -EAGAIN) {
                            //Try again
                            FileFixer::Ring::PrepQueue(sqe.get(), completion->data, blockSize);
                        } else if (completion->cqe->res != completion->data->iov.iov_len) {
                            //Short read/write. Adjust and requeue.
                            completion->data->iov.iov_base = completion->data->buffer.data() + completion->cqe->res;
                            completion->data->iov.iov_len -= completion->cqe->res;

                            FileFixer::Ring::PrepQueue(sqe.get(), completion->data, blockSize);
                        }
                        //Bytes have been read. Store in buffer
                        if (completion->data->type == ioType::read) {
                            std::copy((char *) completion->data->iov.iov_base,
                                      (char *) completion->data->iov.iov_base + completion->data->iov.iov_len,
                                      std::back_inserter(buffer));
                        }

                        completion->Processed();
                        completion->data->free = true;
                        bytesWritten += completion->data->firstLength;
                    }
                }

                ring->Submit();
            }

           return buffer;
        }

        static XXH64_hash_t hashFile(std::vector<char>& buffer, unsigned long fileSize) {
            return XXH64(buffer.data(), fileSize, 0);
        }
    };
}









