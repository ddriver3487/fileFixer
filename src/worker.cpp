//
// Created by xavier on 1/27/23.
//

#include <fstream>
#include "../include/worker.h"
#include "../include/file.h"
#include "../include/ring/Ring.h"

void FileFixer::Worker::ValidateUserInput() {
    fmt::print("Enter Directory Worker: ");
    std::cin >> inputPath;

    if (inputPath.string().front() == '~') {
        prependHomeDir(inputPath);
    }

    //Handle user input until Directory path_ is valid.
    try {
        while (true) {
            if (!exists(inputPath)) {
                fmt::print("{} does not exist. Try again: ", inputPath.string());
                std::cin >> inputPath;
            } else if (!is_directory(inputPath) || is_regular_file(inputPath)) {
                fmt::print("Worker must be to a Directory: ");
                std::cin >> inputPath;
            } else {
                break;
            }
        }
        inputPath = std::filesystem::canonical(inputPath);
        std::filesystem::current_path(inputPath);
        fmt::print("Current Dir: {}\n", std::filesystem::current_path().string());
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
 * Iterate a directory and its subdirectories, if any, to find unique <file size_, path_> pairs. Match duplicate file
 * sizes and hash to find reduce files further..
 * @return a set with unique file paths.
 */
void FileFixer::Worker::ProcessFiles() {

    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputPath)) {
        if (!entry.is_directory()) {
            auto fileSize{entry.file_size()};
            auto& filePath{entry.path()};
            files.emplace(fileSize, filePath);
        } else {
            continue;
        }
    }

    if (!files.empty()) {
        for (const auto& file: files) {
            //Check for duplicate file size_. No need to return the value ( find() ).
            if (files.count(file.first)) {
                auto hash {hashFile(file.first, file.second)};
                hashes.emplace(hash, file.second);
            } else {
                // If file size_ is unique save to set.
                uniqueFiles.emplace(file.second);
            }
        }
    }

    //Hashes should be unique so add them to
    for (const auto& hash: hashes) {
        uniqueFiles.emplace(hash.second);
    }
}

void FileFixer::Worker::prependHomeDir(std::filesystem::path& path) {
    //TODO: check for OS version
    std::string homeDir = getenv("HOME");
    std::string stringPath {path.string()};
    stringPath.replace(stringPath.begin(), stringPath.begin() + 1, homeDir);
    inputPath = stringPath;
}

std::vector<char> FileFixer::Worker::fileToBuffer(const std::filesystem::path& path) {
    FileFixer::Ring::Ring ring(4096, 0);

    std::ifstream source(path, std::ios::in | std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    return buffer;

}


XXH64_hash_t FileFixer::Worker::hashFile(unsigned long fileSize, const std::filesystem::path& path) {
    //Prepare variables for hash function
    auto size{fileSize};
    auto seed{0};
    auto buffer{fileToBuffer(path)};

    return XXH64(buffer.data(), size, seed);
}

void FileFixer::Worker::PrintContainerCount() {
    fmt::print("{}\n", uniqueFiles.size());
}

