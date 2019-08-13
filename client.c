#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define FIB_DEV "/dev/fibonacci"

int main()
{
    int fd, fd_out;
    long long sz;

    clockid_t cid = 1;
    struct timespec start, end;

    char buf[1];
    char write_buf[] = "testing writing";
    int offset = 92;  // TODO: test something bigger than the limit
    int i = 0;

    fd = open(FIB_DEV, O_RDWR);
    fd_out = open("client_output.txt", O_RDWR | O_CREAT | O_TRUNC, 644);

    if (fd < 0) {
        perror("Failed to open character device");
        exit(1);
    }

    for (i = 0; i <= offset; i++) {
        sz = write(fd, write_buf, strlen(write_buf));
        printf("Writing to " FIB_DEV ", returned the sequence %lld\n", sz);
    }

    char buffer[300];
    char kbuffer[300];

    for (i = 0; i <= offset; i++) {
        lseek(fd, i, SEEK_SET);

        clock_gettime(cid, &start);
        sz = read(fd, kbuffer, 1);
        clock_gettime(cid, &end);
        long int sec, nanosec;
        sec = end.tv_sec - start.tv_sec;
        nanosec = end.tv_nsec - start.tv_nsec;
        snprintf(buffer, 300, "%d %ld %ld\n", i, sec * 1000000000 + nanosec,
                 atol(kbuffer));
        write(fd_out, buffer, strlen(buffer));
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }

    for (i = offset; i >= 0; i--) {
        lseek(fd, i, SEEK_SET);
        sz = read(fd, buf, 1);
        printf("Reading from " FIB_DEV
               " at offset %d, returned the sequence "
               "%lld.\n",
               i, sz);
    }

    close(fd);
    close(fd_out);
    return 0;
}
