//
// Created by xavier on 2/1/23.
//

#include "../include/file.h"

#include <fcntl.h> //open
#include <csignal> //close
#include <vector>

FileFixer::File::File(std::filesystem::path filePath) : path_(std::move(filePath)) {
    fd_ = open(path_.c_str(), O_RDONLY);

    if (fd_ < 0) {
        throw std::runtime_error("Can not open source file.");
    }

    size_ = file_size(path_);

}

int FileFixer::File::fd() const { return fd_; }

size_t FileFixer::File::size() const { return size_; }

const std::filesystem::path &FileFixer::File::path() const { return path_; }

FileFixer::File::~File() {
    if (fd_) {
        close(fd_);
    }
}

FileFixer::File::File(FileFixer::File &&other)  noexcept
    : path_(std::move(other.path_)), size_(other.size_) {
}

