//
// Created by xavier on 2/1/23.
//

#pragma once

#include <filesystem>
#include <vector>

namespace FileFixer {
    class File {
        explicit File(std::filesystem::path filePath);

        //Move constructor
        File(File &&other) noexcept ;

        [[nodiscard]]
        int fd() const;

        [[nodiscard]]
        size_t size() const;

        [[nodiscard]]
        const std::filesystem::path& path() const;

        ~File();
    private:
        std::filesystem::path path_;
        int fd_{};
        size_t size_{};

    };
}
