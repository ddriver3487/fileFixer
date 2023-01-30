//
// Created by xavier on 1/27/23.
//

#include <fstream>
#include "../include/fixer.h"

void FileFixer::Fixer::ValidateUserInput() {
    fmt::print("Enter Directory Fixer: ");
    std::cin >> inputPath;

    if (inputPath.string().front() == '~') {
        prependHomeDir(inputPath);
    }

    //Handle user input until Directory path is valid.
    try {
        while (true) {
            if (!exists(inputPath)) {
                fmt::print("{} does not exist. Try again: ", inputPath.string());
                std::cin >> inputPath;
            } else if (!is_directory(inputPath) || is_regular_file(inputPath)) {
                fmt::print("Fixer must be to a Directory: ");
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
                "path(): {}\n "
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
 * Iterate a directory and its subdirectories, if any, to find unique <file size, path> pairs. Match duplicate file
 * sizes and hash to find reduce files further..
 * @return a set with unique file paths.
 */
void FileFixer::Fixer::ProcessFiles() {

    for (const auto& entry : std::filesystem::recursive_directory_iterator(inputPath)) {
        if (!entry.is_directory()) {
            auto fileSize{entry.file_size()};
            auto &filePath{entry.path()};
            files.emplace(fileSize, filePath);
        } else {
            continue;
        }
    }

    if (!files.empty()) {
        for (const auto& file: files) {
            //Check for duplicate file size. No need to return the value ( find() ).
            if (files.count(file.first)) {
                auto hash {hashFile(file.first, file.second)};
                hashes.emplace(hash, file.second);
            } else {
                // If file size is unique save to set.
                uniqueFiles.emplace(file.second);
            }
        }
    }

    //Hashes should be unique so add them to
    for (const auto& hash: hashes) {
        uniqueFiles.emplace(hash.second);
    }
}

void FileFixer::Fixer::prependHomeDir(std::filesystem::path& path) {
    //TODO: check for OS version
    std::string homeDir = getenv("HOME");
    std::string stringPath {path.string()};
    stringPath.replace(stringPath.begin(), stringPath.begin() + 1, homeDir);
    inputPath = stringPath;
}

std::vector<char> FileFixer::Fixer::fileToBuffer(const std::filesystem::path& path) {
    std::ifstream source(path, std::ios::in | std::ios::binary);
    std::vector<char> buffer((std::istreambuf_iterator<char>(source)), std::istreambuf_iterator<char>());
    source.close();
    return buffer;

}


XXH64_hash_t FileFixer::Fixer::hashFile(unsigned long fileSize, const std::filesystem::path& path) {
    //Prepare variables for hash function
    auto size{fileSize};
    auto seed{0};
    auto buffer{fileToBuffer(path)};

    return XXH64(buffer.data(), size, seed);
}

void FileFixer::Fixer::PrintContainer() {
    fmt::print("{}\n", uniqueFiles.size());
}

