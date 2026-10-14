#include <unistd.h>
#include <sys/wait.h>

int main() {
    int pipe1[2];
    int pipe2[2];
    int pipe_mid[2];

    if (pipe(pipe1) < 0 || pipe(pipe2) < 0 || pipe(pipe_mid) < 0) {
        write(2, "pipe error\n", 11);
        return 1;
    }

    pid_t child1_pid = fork();
    if (child1_pid < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }

    if (child1_pid == 0){
        if (dup2(pipe1[0], 0) < 0 || dup2(pipe_mid[1], 1) < 0) {
            write(2, "dup2 error\n", 11);
            _exit(1);
        }

        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        close(pipe_mid[0]); close(pipe_mid[1]);

        execl("./child1", "child1", NULL);
        write(2, "execl error\n", 12);
        _exit(1);
    }


    pid_t child2_pid = fork();
    if (child2_pid < 0) {
        write(2, "fork error\n", 11);
        return 1;
    }

    if (child2_pid == 0){
        if (dup2(pipe_mid[0], 0) < 0 || dup2(pipe2[1], 1) < 0) {
            write(2, "dup2 error\n", 11);
            _exit(1);
        }

        close(pipe1[0]); close(pipe1[1]);
        close(pipe2[0]); close(pipe2[1]);
        close(pipe_mid[0]); close(pipe_mid[1]);

        execl("./child2", "child2", NULL);
        write(2, "execl error\n", 12);
        _exit(1);
    }

    close(pipe1[0]);
    close(pipe2[1]);
    close(pipe_mid[0]);
    close(pipe_mid[1]);

    
    char inbuf[1024];
    char outbuf[1024];
    ssize_t bytes_read;
    ssize_t bytes_write;

    while((bytes_read = read(0, inbuf, sizeof(inbuf))) > 0) {
        bytes_write = write(pipe1[1], inbuf, bytes_read);
        if (bytes_write < 0) {
            write(2, "write error\n", 12);
            return 1;
        }

        ssize_t out_read = 0;
        while(out_read < bytes_read) {
            ssize_t n = read(pipe2[0], outbuf + out_read, bytes_read - out_read);
            if (n < 0) {
                write(2, "read error\n", 11);
                return 1;
            }

            if (n == 0) break;
            out_read += n;
        }

        if (write(1, outbuf, out_read) < 0) {
            write(2, "write error\n", 12);
            return 1;
        }

        if (out_read == 0 || outbuf[out_read - 1] != '\n')
            write(1, "\n", 1);
    }

    if (bytes_read < 0) {
        write(2, "read error\n", 11);
        return 1;
    }

    close(pipe1[1]);
    close(pipe2[0]);

    int status;
    waitpid(child1_pid, &status, 0);
    waitpid(child2_pid, &status, 0);

    return 0;
}
