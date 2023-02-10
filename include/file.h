//
// Created by xavier on 2/1/23.
//

#pragma once

#include <filesystem>
#include <fcntl.h> //open
#include <csignal> //close
#include <vector>

namespace FileFixer {
    class File {
    public:
        std::filesystem::path path;
        int fd{};
        size_t size{};

        explicit File(std::filesystem::path filePath): path(std::move(filePath)) {
            fd = open(path.c_str(), O_RDONLY, O_LARGEFILE, O_NOATIME, O_NONBLOCK);

            if (fd < 0) {
                throw std::runtime_error("Can not open source file.");
            }

            size = file_size(path);

        }

        ~File() {
            if (fd) {
                close(fd);
            }
        }
    };
}

