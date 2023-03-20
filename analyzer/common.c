#include <sys.socket.h>
#include <pb_encode.h>
#include <pb_decode.h>

#include "include/common.h"

static bool writeCallback(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    int fd = (intptr_t)stream->state;
    return send(fd, buf, count, 0) == count;
}

static bool readCallback(pb_istream_t *stream, uint8_t *buf, size_t count) {
    int fd = (intptr_t)stream->state;
    int result;

    if (count == 0)
        return true;

    result = recv(fd, buf, count, MSG_WAITALL);

    if (result == 0) {
        stream->bytes_left = 0;
    }

    return result == count;
}

pb_ostream_t pbOstreamFromSocket(int fd) {
    pb_ostream_t ostream = {&writeCallback, (void*)(intptr_t)fd, SIZE_MAX, 0};
    return ostream;
}

pb_istream_t pbIstreamFromSocket(int fd) {
    pb_istream_t istream = {&readCallback, (void*)(intptr_t)fd, SIZE_MAX, 0};
    return istream;
}
