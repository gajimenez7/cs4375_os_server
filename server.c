/* server side code */
#include <stdint.h>
#include <stdio.h>
#include <unistd.h>

static void usage() {
  fprintf(stderr, "Usage:\n\n"
                  "server-tool <UUP bind port>\n\n");
}

/* study and change as needed */
int better_write(int fd, const void *buf, size_t size) {
  size_t bytes_to_write, bytes_already_written, bytes_written_this_time;
  ssize_t res_write;

  printf("fd parameter: %d\n", fd);
  printf("buf parameter: %s\n", (char *)buf);
  printf("size parameter: %d\n", (int)size);
  bytes_to_write = size;
  bytes_already_written = (size_t)0;
  while (bytes_to_write > ((size_t)0)) {

    res_write =
        write(fd, &((const char *)buf)[bytes_already_written], bytes_to_write);
    if (res_write < ((ssize_t)0)) {
      return -1;
    }
    bytes_written_this_time = (size_t)res_write;
    bytes_to_write -= bytes_written_this_time;
    bytes_already_written += bytes_written_this_time;
  }
  return 0;
}

int convert_port_name(uint16_t *port, const char *prog_name) {
  if (better_write(STDOUT_FILENO, prog_name, sizeof(prog_name)) < 0) {
    printf("prog_name: %s\n", (char *)prog_name);
    printf("prog_name size: %d\n", (int)sizeof(prog_name));
    fprintf(stderr, "Error converting port name.\n");
    return 1;
  }
  return 0;
}

int run_server_tool(const int aux_fd) { return 0; }

int main(int argc, char **argv) {
  uint16_t port;
  int fd_param;
  if (argc < 2) {
    usage();
    return 1;
  }

  if (convert_port_name(&port, argv[1]) < 0) {
    fprintf(stderr, "Cannot convert '%s' to a valid UDP  bind port.\n",
            argv[2]);
    return 1;
  }

  if (run_server_tool(fd_param) < 0) {
    fprintf(stderr, "Failed to run server tool.\n");
    return 1;
  }
  return 0;
}
