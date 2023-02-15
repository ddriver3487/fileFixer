//
// Created by xavier on 2/1/23.
//

#pragma once

#include <filesystem>
#include <fcntl.h> //open
#include <csignal> //close
#include <vector>
#include <fmt/core.h>
#include <linux/fs.h>
#include <sys/stat.h>
#include <sys/ioctl.h>

namespace FileFixer {
    class File {
    public:
        int fd;
        size_t size;
        size_t remaining{};
        std::vector<char> buffer{};

        explicit File(const std::filesystem::path& path) {
            if (path.empty()) {
                throw std::runtime_error(fmt::format("File {} is empty.\n", path.string()));
            } else {

                fd = open(path.c_str(), O_RDONLY);
                if (fd < 0) {
                    throw std::runtime_error(fmt::format("Could not open {}.\n Error: {}.\n",
                                                         path.string(), strerror(errno)));
                }

                struct stat st{};

                if (fstat(fd, &st) < 0) {

                }

                if (S_ISREG(st.st_mode)) {
                    size = st.st_size;
                } else if (S_ISBLK(st.st_mode)) {
                    unsigned long long bytes;

                    if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
                        size = bytes;
                }
            }

            buffer = std::vector<char>(size, 0);
        }

        void UpdateOffset(size_t length) {
            offset += length;
        }

        [[nodiscard]] size_t CurrentOffset() const { return offset; }

        ~File() {
            close(fd);
        }

    private:
        size_t offset{};
    };
}
