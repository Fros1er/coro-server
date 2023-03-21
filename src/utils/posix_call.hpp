#pragma once

#include <cstring>
#include <exception>
#include <cerrno>

#define POSIX_CALL_EXCEPTION(call) \
    if ((call)) throw std::runtime_error(strerror(errno))

#define POSIX_CALL_RETRY(res, call) \
    while((res = (call)) == -1 && errno == EINTR) {} 0
