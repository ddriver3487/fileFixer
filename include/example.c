
int copy_file(struct io_uring *ring, off_t insize) {
    unsigned long reads, writes;
    struct io_uring_cqe *cqe;
    off_t write_left, offset;
    int ret;

    write_left = insize;
    writes = reads = offset = 0;

    while (insize || write_left) {
        int had_reads, got_comp;

        /* Queue up as many reads as we can */
        had_reads = reads;
        while (insize) {
            off_t this_size = insize;

            if (reads + writes >= QD)
                break;
            if (this_size > BS)
                this_size = BS;
            else if (!this_size)
                break;

            if (queue_read(ring, this_size, offset))
                break;

            insize -= this_size;
            offset += this_size;
            reads++;
        }

        if (had_reads != reads) {
            ret = io_uring_submit(ring);
            if (ret < 0) {
                fprintf(stderr, "io_uring_submit: %s\n", strerror(-ret));
                break;
            }
        }

        /* Queue is full at this point. Let's find at least one completion */
        got_comp = 0;
        while (write_left) {
            struct io_data *data;

            if (!got_comp) {
                ret = io_uring_wait_cqe(ring, &cqe);
                got_comp = 1;
            } else {
                ret = io_uring_peek_cqe(ring, &cqe);
                if (ret == -EAGAIN) {
                    cqe = NULL;
                    ret = 0;
                }
            }
            if (ret < 0) {
                fprintf(stderr, "io_uring_peek_cqe: %s\n",
                        strerror(-ret));
                return 1;
            }
            if (!cqe)
                break;

            data = io_uring_cqe_get_data(cqe);
            if (cqe->res < 0) {
                if (cqe->res == -EAGAIN) {
                    queue_prepped(ring, data);
                    io_uring_cqe_seen(ring, cqe);
                    continue;
                }
                fprintf(stderr, "cqe failed: %s\n",
                        strerror(-cqe->res));
                return 1;
            } else if (cqe->res != data->iov.iov_len) {
                /* short read/write; adjust and requeue */
                data->iov.iov_base += cqe->res;
                data->iov.iov_len -= cqe->res;
                queue_prepped(ring, data);
                io_uring_cqe_seen(ring, cqe);
                continue;
            }

            /*
             * All done. If write, nothing else to do. If read,
             * queue up corresponding write.
             * */

            if (data->read) {
                queue_write(ring, data);
                write_left -= data->first_len;
                reads--;
                writes++;
            } else {
                free(data);
                writes--;
            }
            io_uring_cqe_seen(ring, cqe);
        }
    }

    return 0;
}