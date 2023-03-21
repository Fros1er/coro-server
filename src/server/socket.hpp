#pragma once

class IOContext;

class Socket {
    int fd_;
    IOContext &ctx_;

public:
    explicit Socket(int fd, IOContext &ctx);

    [[nodiscard]]
    int fd() const {
        return fd_;
    }

    IOContext &ctx() {
        return ctx_;
    }

//    void async_write();

//    ssize_t async_read_some(void *buf, size_t buf_len);
};
