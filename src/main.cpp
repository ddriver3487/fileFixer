#include <fmt/core.h>
#include "../include/worker.h"
void printTitle() {
    fmt::print("{:^{}}\n", "   ______ _  __       ______ _                  \n"
                           "   / ____/(_)/ /___   / ____/(_)_  __ ___   _____\n"
                           "  / /_   / // // _ \\ / /_   / /| |/_// _ \\ / ___/\n"
                           " / __/  / // //  __// __/  / /_>  < /  __// /    \n"
                           "/_/    /_//_/ \\___//_/    /_//_/|_| \\___//_/   \n\n\n",
               100);
}

int main() {

    printTitle();
    FileFixer::Ring ring(4096);

//    FileFixer::Worker worker;
//    auto path { std::filesystem::path("~/test") };
//    worker.InputPathHandler(path);
//    worker.ProcessFiles(&ring);
//    worker.PrintContainerCount();




    return 0;
}

/* SPDX-License-Identifier: MIT */
/*
 * gcc -Wall -O2 -D_GNU_SOURCE -o io_uring-cp io_uring-cp.c -luring
// */
//#include <cstdio>
//#include <fcntl.h>
//#include <cstring>
//#include <cstdlib>
//#include <unistd.h>
//#include <cassert>
//#include <cerrno>
//#include <sys/stat.h>
//#include <sys/ioctl.h>
//#include "liburing.h"
//
//#define QD	64
//#define BS	(32*1024)
//
//static int inFD, outFD;
//
//struct io_data {
//    int read;
//    size_t first_offset, offset;
//    size_t first_len;
//    struct iovec iov;
//};
//
//static int setup_context(struct io_uring *ring)
//{
//    int ret;
//
//    ret = io_uring_queue_init(64, ring, 0);
//    if (ret < 0) {
//        fprintf(stderr, "queue_init: %s\n", strerror(-ret));
//        return -1;
//    }
//
//    return 0;
//}
//
//static int get_file_size(int fd, size_t *size)
//{
//    struct stat st{};
//
//    if (fstat(fd, &st) < 0)
//        return -1;
//    if (S_ISREG(st.st_mode)) {
//        *size = st.st_size;
//        return 0;
//    } else if (S_ISBLK(st.st_mode)) {
//        unsigned long long bytes;
//
//        if (ioctl(fd, BLKGETSIZE64, &bytes) != 0)
//            return -1;
//
//        *size = bytes;
//        return 0;
//    }
//
//    return -1;
//}
//
//static void queue_prepped(struct io_uring *ring, struct io_data *data)
//{
//    struct io_uring_sqe *sqe;
//
//    sqe = io_uring_get_sqe(ring);
//    assert(sqe);
//
//    if (data->read)
//        io_uring_prep_readv(sqe, inFD, &data->iov, 1, data->offset);
//    else
//        io_uring_prep_writev(sqe, outFD, &data->iov, 1, data->offset);
//
//    io_uring_sqe_set_data(sqe, data);
//}
//
//static int queue_read(struct io_uring *ring, size_t size, size_t offset)
//{
//    struct io_uring_sqe *sqe;
//    struct io_data *data;
//
//    data = static_cast<io_data *>(malloc(size + sizeof(*data)));
//    if (!data)
//        return 1;
//
//    sqe = io_uring_get_sqe(ring);
//    if (!sqe) {
//        free(data);
//        return 1;
//    }
//
//    data->read = 1;
//    data->offset = data->first_offset = offset;
//
//    data->iov.iov_base = data + 1;
//    data->iov.iov_len = size;
//    data->first_len = size;
//
//    io_uring_prep_readv(sqe, inFD, &data->iov, 1, offset);
//    io_uring_sqe_set_data(sqe, data);
//    return 0;
//}
//
//static void queue_write(struct io_uring *ring, struct io_data *data)
//{
//    data->read = 0;
//    data->offset = data->first_offset;
//
//    data->iov.iov_base = data + 1;
//    data->iov.iov_len = data->first_len;
//
//    queue_prepped(ring, data);
//    io_uring_submit(ring);
//}
//
//static int copy_file(struct io_uring *ring, size_t inSize)
//{
//    unsigned long reads, writes;
//    struct io_uring_cqe *cqe;
//    size_t write_left, offset;
//    int ret;
//
//    write_left = inSize;
//    writes = reads = offset = 0;
//
//    while (inSize || write_left) {
//        unsigned long had_reads;
//        int got_comp;
//
//        /*
//         * Queue up as many reads as we can
//         */
//        had_reads = reads;
//        while (inSize) {
//            size_t this_size = inSize;
//
//            if (reads + writes >= QD)
//                break;
//            if (this_size > BS)
//                this_size = BS;
//            else if (!this_size)
//                break;
//
//            if (queue_read(ring, this_size, offset))
//                break;
//
//            inSize -= this_size;
//            offset += this_size;
//            reads++;
//        }
//
//        if (had_reads != reads) {
//            ret = io_uring_submit(ring);
//            if (ret < 0) {
//                fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
//                break;
//            }
//        }
//
//        /*
//         * Queue is full at this point. Find at least one completion.
//         */
//        got_comp = 0;
//        while (write_left) {
//            struct io_data *data;
//
//            if (!got_comp) {
//                ret = io_uring_wait_cqe(ring, &cqe);
//                got_comp = 1;
//            } else {
//                ret = io_uring_peek_cqe(ring, &cqe);
//                if (ret == -EAGAIN) {
//                    cqe = nullptr;
//                    ret = 0;
//                }
//            }
//            if (ret < 0) {
//                fprintf(stderr, "io_uring_peek_cqe: %s\n",
//                        strerror(-ret));
//                return 1;
//            }
//            if (!cqe)
//                break;
//
//            data = static_cast<io_data *>(io_uring_cqe_get_data(cqe));
//            if (cqe->res < 0) {
//                if (cqe->res == -EAGAIN) {
//                    queue_prepped(ring, data);
//                    io_uring_submit(ring);
//                    io_uring_cqe_seen(ring, cqe);
//                    continue;
//                }
//                fprintf(stderr, "cqe failed: %s\n",
//                        strerror(-cqe->res));
//                return 1;
//            } else if ((size_t)cqe->res != data->iov.iov_len) {
//                /* Short read/write, adjust and requeue */
//                data->iov.iov_base = (void*)((char*)data->iov.iov_base + cqe->res);
//                data->iov.iov_len -= cqe->res;
//                data->offset += cqe->res;
//                queue_prepped(ring, data);
//                io_uring_submit(ring);
//                io_uring_cqe_seen(ring, cqe);
//                continue;
//            }
//
//            /*
//             * All done. if a write operation, nothing else to do. if read,
//             * queue up write.
//             */
//            if (data->read) {
//                queue_write(ring, data);
//                write_left -= data->first_len;
//                reads--;
//                writes++;
//            } else {
//                free(data);
//                writes--;
//            }
//            io_uring_cqe_seen(ring, cqe);
//        }
//    }
//
//    /* wait out pending writes */
//    while (writes) {
//        struct io_data *data;
//
//        ret = io_uring_wait_cqe(ring, &cqe);
//        if (ret) {
//            fprintf(stderr, "wait_cqe=%d\n", ret);
//            return 1;
//        }
//        if (cqe->res < 0) {
//            fprintf(stderr, "write res=%d\n", cqe->res);
//            return 1;
//        }
//        data = static_cast<io_data *>(io_uring_cqe_get_data(cqe));
//        free(data);
//        writes--;
//        io_uring_cqe_seen(ring, cqe);
//    }
//
//    return 0;
//}
//
//int main(int argc, char *argv[])
//{
//    struct io_uring ring{};
//    size_t inSize;
//    int ret;
//
//    if (argc < 3) {
//        printf("%s: infile outfile\n", argv[0]);
//        return 1;
//    }
//
//    inFD = open(argv[1], O_RDONLY);
//    if (inFD < 0) {
//        perror("open infile");
//        return 1;
//    }
//    outFD = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
//    if (outFD < 0) {
//        perror("open outfile");
//        return 1;
//    }
//
//    if (setup_context(&ring))
//        return 1;
//    if (get_file_size(inFD, &inSize))
//        return 1;
//
//    ret = copy_file(&ring, inSize);
//
//    close(inFD);
//    close(outFD);
//    io_uring_queue_exit(&ring);
//    return ret;
//}