/* server side code */
#include <ctype.h>
#include <errno.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

/* study and change as needed */
int better_write(int fd, const void *buf, size_t size) {
  size_t bytes_to_write, bytes_already_written, bytes_written_this_time;
  ssize_t res_write;

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

static void usage() {
  fprintf(stderr, "Usage:\n\n"
                  "No input parameters, port is set to: 9999\n\n");
}

void process_message(char *message, const size_t size) {
  size_t string_len = size;
  for (size_t i = 0; i < size; i++) {
    if (message[i] == '\0') {
      string_len = i;
      break;
    }
  }
  for (int i = 0; i < string_len; i++) {
    message[i] = (char)toupper((unsigned char)message[i]);
  }
}

static int open_tcp_fd(uint16_t port) {
  struct sockaddr_in serveraddr;
  int fd;
  int backlog_size = 5;
  /* create socket file descriptor */
  fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    fprintf(stderr, "Could not create socket: %s\n", strerror(errno));
    return -1;
  }
  /* fill address */
  memset(&serveraddr, 0, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);
  /* bind port to socket address */
  if (bind(fd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) < 0) {
    fprintf(stderr, "Cannot bind: %s\n", strerror(errno));
    if (close(fd) < 0) {
      fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    }
    return -1;
  }

  /* open queue for listening */
  if (listen(fd, backlog_size) < 0) {
    fprintf(stderr, "Cannot listen on port %d\n%s\n", port, strerror(errno));
    return -1;
  }

  char *listening_message = "listening on port '9999'\n.";
  better_write(STDOUT_FILENO, listening_message, strlen(listening_message));

  return fd;
}

int run_server_tool(const int aux_fd) {
  /* bytes to send should equal to bytes received */
  struct sockaddr_in client_addr;
  socklen_t clientaddrlen;
  ssize_t received_result;
  ssize_t result_of_send;
  size_t bytes_to_send;
  int client_file_desc;
  char message[8192];
  clientaddrlen = (socklen_t)sizeof(client_addr);
  /* accept client file descriptor */
  client_file_desc =
      accept(aux_fd, (struct sockaddr *)&client_addr, &clientaddrlen);

  if (client_file_desc < 0) {
    fprintf(stderr, "Could not accept connection %s\n", strerror(errno));
    if (close(client_file_desc) < 0) {
      fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    }
    return 0;
  }

  /* received message from client file descriptor */
  received_result = recv(client_file_desc, message, sizeof(message), 0);

  /* check if packet was received */
  if (received_result < ((ssize_t)0)) {
    fprintf(stderr, "Cannot receive a packet: %s\n", strerror(errno));
    if (close(client_file_desc) < 0) {
      fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    }
    return -1;
  }
  /* client closed connection */
  else if (received_result == ((size_t)0)) {
    if (close(client_file_desc) < 0) {
      fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    }
    return 0;
  }

  /* received message and format */
  /* use received_result to ensure correct size */
  process_message(message, (size_t)received_result);

  bytes_to_send = received_result;

  /* write to client */
  result_of_send = better_write(client_file_desc, message, bytes_to_send);

  if (result_of_send < (ssize_t)0) {
    fprintf(stderr, "Could not send message: %s\n", strerror(errno));
    if (close(client_file_desc) < 0) {
      fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    }
    return -1;
  }
  if (close(client_file_desc) < 0) {
    fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
  }

  return 0;
}

int main(int argc, char **argv) {
  uint16_t port = 9999;
  int fd;
  /* takes no arguments */
  if (argc > 1) {
    usage();
    return 1;
  }

  /* create auxilary file descriptor for socket */
  fd = open_tcp_fd(port);

  /* main loop */
  for (;;) {
    if (run_server_tool(fd) < 0) {
      fprintf(stderr, "Failed to run server tool.\n");
      if (close(fd) < 0) {
        fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
      }
      return 1;
    }
  }

  /* final close */
  if (close(fd) < 0) {
    fprintf(stderr, "Cannot close a socket: %s\n", strerror(errno));
    return 1;
  }
  return 0;
}
