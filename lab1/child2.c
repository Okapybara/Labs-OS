#include <unistd.h>
#include <ctype.h>

int main(){
    char buffer[1024];
    ssize_t bytes_read;

    while ((bytes_read = read(0, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (isspace((unsigned char)buffer[i]))
                buffer[i] = '_';
        }

        if (write(1, buffer, bytes_read) < 0){
            write(2, "write error\n", 12);
            return 1;
        }
    }
    
    if (bytes_read < 0) {
        write(2, "read error\n", 11);
        return 1;
    }
    
    return 0;
}
