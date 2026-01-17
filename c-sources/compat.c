#define _GNU_SOURCE
#include <unistd.h>
#include <sys/syscall.h>
#include <errno.h>

#ifdef __linux__
#ifndef SYS_copy_file_range
// Fallback implementation using read/write
ssize_t copy_file_range(int fd_in, loff_t *off_in, int fd_out,
                       loff_t *off_out, size_t len, unsigned int flags)
{
    char buf[8192];
    ssize_t bytes_read, bytes_written;
    size_t total = 0;
    
    while (total < len) {
        size_t to_read = len - total;
        if (to_read > sizeof(buf))
            to_read = sizeof(buf);
            
        bytes_read = read(fd_in, buf, to_read);
        if (bytes_read <= 0)
            return total > 0 ? total : bytes_read;
            
        bytes_written = write(fd_out, buf, bytes_read);
        if (bytes_written != bytes_read)
            return -1;
            
        total += bytes_written;
    }
    
    return total;
}
#endif
#endif