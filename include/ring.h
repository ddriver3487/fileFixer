//
// Created by xavier on 2/1/23.
//

#pragma once

#include <cstdint>

#include <liburing.h>
#include <sys/utsname.h>
#include <sys/uio.h> //iovec
#include <vector>
#include <fmt/core.h>
#include <optional>

namespace FileFixer {
    enum class ioType {
        read,
        write,
        none
    };

    struct ioData {
        int fd { };
        ioType type { ioType::none };
        size_t startingOffset   { };
        size_t currentOffset    { };
        size_t firstLength      { };
        iovec  iov              { };
    };

    class Completion {
    public:
        Completion() : ring(nullptr), data(nullptr), cqe(nullptr) { };
        Completion(io_uring* ring, io_uring_cqe* cqe) : ring(ring), cqe(cqe) { };

        [[nodiscard]] constexpr bool Safe() const { return cqe != nullptr; }

        [[nodiscard]] constexpr bool Type() const { return  data->type == ioType::read; }

        void Consumed() {
            io_uring_cqe_seen(ring, cqe);
            cqe = nullptr;
        }

        // Return an IO completion, if one is readily available. Returns with a cqe_ptr filled in on success.
        std::optional<Completion> Get() {
            Completion completion;
            int returnCode = io_uring_peek_cqe(ring, &completion.cqe);

            if (returnCode != 0) {
                //return empty Completion
                return { };
            } else {
                data = (ioData *)(cqe->user_data);
                return completion;
            }
        }

    private:
        io_uring* ring;
        ioData* data{};
        io_uring_cqe* cqe;
    };

    class Ring {
    public:

        // ============ Ring Construction and Destruction ============ //

        /**
         * Initialize a io_uring instance.
         * @param entries sets the SQ and CQ queue depth.
         * @param flags Multiple flags can be set with | (pipe) separator.
         */
        explicit Ring(unsigned entries, unsigned flags = 0) {
            int started { io_uring_queue_init(entries, &ring, flags) };

            if (started < 0) {
                throw std::runtime_error(fmt::format("Ring failed to start. Error: {}\n", strerror(-started)));
            }
        }

        //Disable all copying or moving
        Ring(const Ring &) = delete;
        Ring(Ring &&) = delete;
        Ring &operator=(const Ring &) = delete;
        Ring &operator=(Ring &&) = delete;
        
        ~Ring() { io_uring_queue_exit(&ring); }

        // ============ Ring Operations ============ //

        // Print list of io_uring operations supported by the kernel.
        void Probe() {
            std::vector<std::string> uringOps { "IORING_OP_NOP",
                                                "IORING_OP_READV",
                                                "IORING_OP_WRITEV",
                                                "IORING_OP_FSYNC",
                                                "IORING_OP_READ_FIXED",
                                                "IORING_OP_WRITE_FIXED",
                                                "IORING_OP_POLL_ADD",
                                                "IORING_OP_POLL_REMOVE",
                                                "IORING_OP_SYNC_FILE_RANGE",
                                                "IORING_OP_SENDMSG",
                                                "IORING_OP_RECVMSG",
                                                "IORING_OP_TIMEOUT",
                                                "IORING_OP_TIMEOUT_REMOVE",
                                                "IORING_OP_ACCEPT",
                                                "IORING_OP_ASYNC_CANCEL",
                                                "IORING_OP_LINK_TIMEOUT",
                                                "IORING_OP_CONNECT",
                                                "IORING_OP_FALLOCATE",
                                                "IORING_OP_OPENAT",
                                                "IORING_OP_CLOSE",
                                                "IORING_OP_FILES_UPDATE",
                                                "IORING_OP_STATX",
                                                "IORING_OP_READ",
                                                "IORING_OP_WRITE",
                                                "IORING_OP_FADVISE",
                                                "IORING_OP_MADVISE",
                                                "IORING_OP_SEND",
                                                "IORING_OP_RECV",
                                                "IORING_OP_OPENAT2",
                                                "IORING_OP_EPOLL_CTL",
                                                "IORING_OP_SPLICE",
                                                "IORING_OP_PROVIDE_BUFFERS",
                                                "IORING_OP_REMOVE_BUFFERS"
            };

            utsname system {};
            uname(&system);
            io_uring_probe *probe = io_uring_get_probe_ring(&ring);

            fmt::print("You are running kernel version: {}\n"
                       "Supported io_uring operations include: \n", system.release);

            for (auto opCode = 0; opCode < IORING_OP_LAST; opCode++) {
                if (io_uring_opcode_supported(probe, opCode)) {
                    fmt::print("{}\n", uringOps[opCode]);
                }
            }
            free(probe);
        }

        static void PrepQueue(io_uring_sqe* sqe, ioData *data, int fd, size_t size, size_t offset) {
            if (data->type == ioType::write) {
                prepWriteData(fd, data);

                io_uring_prep_writev(sqe, fd, &data->iov, 1, offset);
            } else {
                prepReadData(fd, size, offset, data);

                io_uring_prep_readv(sqe, fd, &data->iov, 1, offset);
            }

            io_uring_sqe_set_data(sqe, data);
        }

        static void Submit() {
            int submitted { io_uring_submit(&ring) };

            if (submitted < 0) {
                throw std::runtime_error(fmt::format("Failed to submit to ring. Error: {}\n", strerror(-submitted)));
            }
        }

        static auto GetIOData() {
            return std::make_unique_for_overwrite<ioData>();

        }

        static std::unique_ptr<io_uring_sqe> GetSQE() {
            return std::make_unique_for_overwrite<io_uring_sqe>();

        }

    private:
        static io_uring ring;
        std::vector<int> fds{};
        static void prepReadData(int fd, size_t size, size_t offset, ioData* data) {
            data->fd = fd;
            data->type = ioType::read;
            data->startingOffset = offset;
            data->currentOffset  = offset;
            data->iov.iov_base   = data + 1;
            data->iov.iov_len    = size;
            data->firstLength    = size;
        }

        static void prepWriteData(int fd, ioData *data) {
            data->fd = fd;
            data->type = ioType::write;
            data->currentOffset = data->startingOffset;
            data->iov.iov_base = data + 1;
            data->iov.iov_len = data->firstLength;
        }

    };
}