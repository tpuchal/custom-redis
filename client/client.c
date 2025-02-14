#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <assert.h>

const size_t k_max_msg = 4096;

static int32_t read_full(int fd, char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n) {
    while (n > 0) {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0) {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static void msg(const char *msg) {
    fprintf(stderr, "%s\n", msg);
}


static int32_t query(int fd, const char *text) {
    uint32_t length = (uint32_t)strlen(text);
    if (length > k_max_msg) {
        return -1;
    }

    char write_buffer[4 + k_max_msg];
    memcpy(write_buffer, &length, 4);
    memcpy(&write_buffer[4], text, length);
    int32_t err = write_all(fd, write_buffer, 4 + length);
    if(err) {
        return err;
    }

    char read_buffer[k_max_msg];
    errno = 0;
    err = read_full(fd, read_buffer, 4);
    if (err) {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }


    if (length > k_max_msg) {
        msg("too long");
        return -1;
    }

    err = read_full(fd, &read_buffer[4], length);
    if (err) {
        msg("read() error");
        return err;
    }

    printf("server says: %.*s\n", length, &read_buffer[4]);
    return 0;
}

static void die(const char *msg) {
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) {
        die("socket");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if(rv) {
        die("connect()");
    }

    for(int i = 0 ; i < 100000 ; i++) {
        int32_t err = query(fd, "veryverylonghello");
    if (err) {
        goto L_DONE;
    }
    err = query(fd, "hello2");
    if (err) {
        goto L_DONE;
    }
    }
    
L_DONE:
    close(fd);
    return 0;
}