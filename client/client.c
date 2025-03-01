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

static int32_t read_full(int fd, char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = read(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static int32_t write_all(int fd, const char *buf, size_t n)
{
    while (n > 0)
    {
        ssize_t rv = write(fd, buf, n);
        if (rv <= 0)
        {
            return -1;
        }
        assert((size_t)rv <= n);
        n -= (size_t)rv;
        buf += rv;
    }
    return 0;
}

static void msg(const char *msg)
{
    fprintf(stderr, "%s\n", msg);
}

static int32_t query(int fd, const char *text)
{
    uint32_t length = (uint32_t)strlen(text);
    if (length > k_max_msg)
    {
        return -1;
    }

    char write_buffer[4 + k_max_msg];
    memcpy(write_buffer, &length, 4);
    memcpy(&write_buffer[4], text, length);

    int32_t err = write_all(fd, write_buffer, 4 + length);
    if (err)
    {
        return err;
    }

    uint32_t resp_len;
    err = read_full(fd, (char *)&resp_len, 4);
    if (err)
    {
        msg(errno == 0 ? "EOF" : "read() error");
        return err;
    }

    if (resp_len > k_max_msg)
    {
        msg("response too long");
        return -1;
    }

    // Read response message
    char read_buffer[k_max_msg + 1];
    err = read_full(fd, read_buffer, resp_len);
    if (err)
    {
        msg("read() error");
        return err;
    }

    read_buffer[resp_len] = '\0';
    printf("server says: %s\n", read_buffer);
    return 0;
}

static int32_t bulk_query(int fd, const char *queries[], size_t num_queries)
{
    // Calculate total buffer size needed
    size_t total_size = 8; // 4 bytes for marker + 4 bytes for num_queries
    for (size_t i = 0; i < num_queries; i++)
    {
        total_size += 4 + strlen(queries[i]); // 4 bytes length + message
    }

    // Allocate buffer
    char *buffer = malloc(total_size);
    if (!buffer)
    {
        msg("malloc failed");
        return -1;
    }

    // Write marker and number of queries
    uint32_t marker = 0xFFFFFFFF;
    uint32_t queries_count = (uint32_t)num_queries;
    memcpy(buffer, &marker, 4);
    memcpy(buffer + 4, &queries_count, 4);

    // Write each query
    size_t offset = 8;
    for (size_t i = 0; i < num_queries; i++)
    {
        uint32_t len = (uint32_t)strlen(queries[i]);
        memcpy(buffer + offset, &len, 4);
        memcpy(buffer + offset + 4, queries[i], len);
        offset += 4 + len;
    }

    // Send bulk request
    int32_t err = write_all(fd, buffer, total_size);
    free(buffer);
    if (err)
    {
        return err;
    }

    // Read responses
    for (size_t i = 0; i < num_queries; i++)
    {
        uint32_t resp_len;
        err = read_full(fd, (char *)&resp_len, 4);
        if (err)
        {
            msg(errno == 0 ? "EOF" : "read() error");
            return err;
        }

        if (resp_len > k_max_msg)
        {
            msg("response too long");
            return -1;
        }

        char read_buffer[k_max_msg + 1];
        err = read_full(fd, read_buffer, resp_len);
        if (err)
        {
            msg("read() error");
            return err;
        }

        read_buffer[resp_len] = '\0';
        printf("server response %zu: %s\n", i + 1, read_buffer);
    }

    return 0;
}

static int32_t multi_query(int fd, const char *queries[], size_t num_queries)
{
    for (size_t i = 0; i < num_queries; i++)
    {
        printf("Sending query: %s\n", queries[i]);
        int32_t err = query(fd, queries[i]);
        if (err)
            return err;
    }
    return 0;
}

static void die(const char *msg)
{
    int err = errno;
    fprintf(stderr, "[%d] %s\n", err, msg);
    abort();
}

int main()
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0)
    {
        die("socket");
    }

    struct sockaddr_in addr = {};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);
    addr.sin_addr.s_addr = ntohl(INADDR_LOOPBACK);
    int rv = connect(fd, (const struct sockaddr *)&addr, sizeof(addr));
    if (rv)
    {
        die("connect()");
    }

    // Example of a single query
    printf("Sending a single query...\n");
    int32_t err = query(fd, "Hello server");
    if (err)
    {
        goto L_DONE;
    }

    // Example of multiple queries
    printf("\nSending multiple queries...\n");
    const char *multiple_queries[] = {
        "Query 1",
        "Query 2",
        "Query 3"};
    err = multi_query(fd, multiple_queries, 3);

    // Example of bulk queries
    printf("\nSending bulk queries...\n");
    const char *bulk_queries[] = {
        "Query 1",
        "Query 2",
        "Query 3"};
    err = bulk_query(fd, bulk_queries, 3);

L_DONE:
    close(fd);
    return 0;
}