//
// Created by xavier on 2/1/23.
//

#pragma once

#include <cstdint>
#include <liburing.h>
#include <sys/utsname.h>
#include <vector>
#include <fmt/core.h>

namespace FileFixer {
    class Ring {
    public:

        // ============ Ring Construction and Destruction ============ //

        /**
         * Initialize a io_uring instance.
         * @param entries sets the SQ and CQ queue depth.
         * @param flags Multiple flags can be set with | (pipe) separator.
         */
        constexpr explicit Ring(unsigned entries , unsigned flags = 0) {
            io_uring_queue_init(entries, &ring, flags);
        }

        //Disable all copying or moving
        Ring(const Ring &) = delete;
        Ring(Ring &&) = delete;
        Ring &operator=(const Ring &) = delete;
        Ring &operator=(Ring &&) = delete;
        
        ~Ring() { io_uring_queue_exit(&ring); }

        // ============ Ring Operations ============ //

        // Print list of io_uring operations supported by users system kernel version.
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



    private:
        io_uring ring{};
    };
}