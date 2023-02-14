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
        int fd              { };
        bool free           { true };
        ioType type         { ioType::none };
        size_t firstOffset  { };
        size_t offset       { };
        size_t firstLength  { };
        iovec  iov          { };
        std::vector<char> buffer {};
    };

    class Completion {
    public:
        io_uring_cqe* cqe{};
        ioData* data{};

        explicit Completion(io_uring* ring) : ring(ring) { };
        Completion() : ring(nullptr), data(nullptr), cqe(nullptr) { };

        void Processed() {
            io_uring_cqe_seen(ring, cqe);
            cqe = nullptr;
        }

        // Return an IO completion, if one is readily available. Returns with a cqe_ptr filled in on success.
        std::optional<Completion> Peek() {
            Completion completion;
            int returnCode = io_uring_peek_cqe(ring, &completion.cqe);

            if (returnCode != 0) {
                //return empty Completion
                fmt::print("Completion Error: {}\n", strerror(returnCode));
                return { };
            } else {
                data = (ioData *)(io_uring_cqe_get_data(cqe));
                return completion;
            }
        }

    private:
        io_uring* ring;
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

            fillIoData();
        }

        //Disable all copying or moving
        Ring(const Ring &) = delete;
        Ring(Ring &&) = delete;
        Ring &operator=(const Ring &) = delete;
        Ring &operator=(Ring &&) = delete;
        
        ~Ring() { io_uring_queue_exit(&ring); }

        // ============ Ring Operations ============ //

        // Print list of io_uring operations supported by the kernel.
        [[maybe_unused]]
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
            io_uring_probe* probe = io_uring_get_probe_ring(&ring);

            fmt::print("You are running kernel version: {}\n"
                       "Supported io_uring operations include: \n", system.release);

            for (auto opCode = 0; opCode < IORING_OP_LAST; opCode++) {
                if (io_uring_opcode_supported(probe, opCode)) {
                    fmt::print("{}\n", uringOps[opCode]);
                }
            }
            free(probe);
        }

        static void PrepQueue(io_uring_sqe* entry, ioData *data, size_t size) {
            if (data->type == ioType::write) {
                prepWriteData(data->fd, data);

                io_uring_prep_writev(entry, data->fd, &data->iov, 1, data->offset);
            } else {
                prepReadData(data->fd, size, data->offset, data);

                io_uring_prep_readv(entry, data->fd, &data->iov, 1, data->offset);
            }

            io_uring_sqe_set_data(entry, data);
        }

        void Submit() {
            int submitted { io_uring_submit(&ring) };

            if (submitted < 0) {
                throw std::runtime_error(fmt::format("Failed to submit to ring. Error: {}\n", strerror(-submitted)));
            }
        }

        std::optional<ioData*> GetIoData() {
            auto foundIOData = std::ranges::find_if(
                    dataPool, [&] (const auto& data) {
                        return data->free;
                    } );
            if (foundIOData != dataPool.end()) {
                foundIOData->get()->free = false;
                return foundIOData->get();
            } else {
                return { };
            }
        }


        static auto GetSQE() { return std::make_unique<io_uring_sqe>(); }

        auto Expose()        { return &ring; }

    private:
        io_uring ring{};
        std::vector<std::unique_ptr<ioData>> dataPool;

        static void prepReadData(int fd, size_t size, size_t offset, ioData* data) {
            data->fd            = fd;
            data->type          = ioType::read;
            data->firstOffset   = offset;
            data->offset        = offset;
            data->iov.iov_base  = data->buffer.data();
            data->iov.iov_len   = size;
            data->firstLength   = size;
        }

        static void prepWriteData(int fd, ioData *data) {
            data->fd            = fd;
            data->type          = ioType::write;
            data->offset        = data->firstOffset;
            data->iov.iov_base  = data->buffer.data();
            data->iov.iov_len   = data->firstLength;
        }

        /**
         * fill ioData pool by n sq entries.
         */
        void fillIoData() {
            dataPool.reserve(ring.sq.ring_entries);
            std::generate_n(std::back_inserter(dataPool), ring.sq.ring_entries,
                            [ ] { auto data { std::make_unique_for_overwrite<ioData>()};
                                  data->buffer.reserve(1024);
                                  return data;});
        }
    };
}