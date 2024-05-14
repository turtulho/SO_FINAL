#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/wait.h>

// Named pipes definitions
#define USER_PIPE "/tmp/user_pipe"
#define BACK_PIPE "/tmp/back_pipe"

// Function to write messages to a named pipe
void write_to_pipe(const char *pipe_name, const char *message) {
    int pipe_fd = open(pipe_name, O_WRONLY);  // Open the pipe for writing
    if (pipe_fd == -1) {
        perror("Error opening pipe");
        exit(EXIT_FAILURE);
    }

    size_t message_length = strlen(message);
    ssize_t bytes_written = write(pipe_fd, message, message_length);

    if (bytes_written == -1) {
        perror("Error writing to pipe");
    } else {
        printf("Message written to %s: %s\n", pipe_name, message);
    }

    close(pipe_fd);  // Close the pipe
}

int main() {
    // Ensure the pipes exist (create them if they don't)
    if (mkfifo(USER_PIPE, 0666) == -1 && errno != EEXIST) {
        perror("Error creating USER_PIPE");
        exit(EXIT_FAILURE);
    }

    if (mkfifo(BACK_PIPE, 0666) == -1 && errno != EEXIST) {
        perror("Error creating BACK_PIPE");
        exit(EXIT_FAILURE);
    }
            
        // Send messages to USER_PIPE
   
        write_to_pipe(USER_PIPE, "12#100#\n");
        sleep(1);  // Delay for demonstration purposes
       

    return 0;  // Success
}
