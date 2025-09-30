#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAGIC       (((uint32_t) 0xcafebabe))

static void usage() {
  fprintf(stderr,
          "Usage:\n\n"
          "ip-tool-server <TCP bind port>\n\n");
}

static int convert_port_name(uint16_t *port,
                             const char *port_name) {
  char *end;
  long long int nn;
  uint16_t t;
  long long int tt;

  if (port_name == NULL) return -1;
  if (*port_name == '\0') return -1;
  nn = strtoll(port_name, &end, 0); // String to LL
  if (*end != '\0') return -1; 
  if (nn < ((long long int) 0)) return -1;
  t = (uint16_t) nn;
  tt = (long long int) t;
  if (tt != nn) return -1;
  *port = t;
  return 0;
}

static int open_tcp_fd(uint16_t port) {
  int fd;
  struct sockaddr_in serveraddr;

  // Create a Socket
  fd = socket(AF_INET, SOCK_STREAM, 0); //TCP/IPv4 socket
  if (fd < 0) {
    fprintf(stderr,
            "Cannot create a socket: %s\n",
            strerror(errno));
    return -1;
  }

  // Fill the server adress
  memset(&serveraddr, 0, sizeof(serveraddr)); // zero it out
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons(port);
  if (bind(fd,
           (struct sockaddr *) &serveraddr,
           sizeof(serveraddr)) < 0) {
    fprintf(stderr,
            "Cannot bind: %s\n",
            strerror(errno));
    if (close(fd) < 0) {
      fprintf(stderr,
              "Cannot close a socket: %s\n",
              strerror(errno));
    }
    return -1;
  }

  if (listen(fd, 16) < 0) {
    fprintf(stderr,
            "Cannot listen: %s\n",
            strerror(errno));
    if (close(fd) < 0) {
      fprintf(stderr,
              "Cannot close a socket: %s\n",
              strerror(errno));
    }
    return -1;
  }

  return fd;
}

static int run_tool(int fd) {
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen;
  int cfd;
  ssize_t recv_res;
  uint32_t magic;
  char str[257];
  size_t bytes_to_send;
  ssize_t res_send;

  clientaddrlen = (socklen_t) sizeof(clientaddr);
  cfd = accept(fd, (struct sockaddr *) &clientaddr, &clientaddrlen);
  if (cfd < 0) {
    fprintf(stderr,
            "Cannot accept a connection: %s\n",
            strerror(errno));
    return -1;
  }

  if (((size_t) clientaddrlen) != sizeof(clientaddr)) {
    (void) close(cfd);
    return 0;
  }

  recv_res = recv(cfd, &magic, sizeof(magic), MSG_WAITALL);
  if (recv_res < ((ssize_t) 0)) {
    fprintf(stderr,
            "Cannot receive: %s\n",
            strerror(errno));
    (void) close(cfd);
    return -1;
  }
  if (((size_t) recv_res) != sizeof(magic)) {
    (void) close(cfd);
    return 0;
  }

  magic = ntohl(magic);
  if (magic != (MAGIC)) {
    (void) close(cfd);
    return 0;
  }

  memset(str, '\0', sizeof(str));
  if (inet_ntop(AF_INET,
                &clientaddr.sin_addr,
                str,
                sizeof(str) - ((size_t) 1)) == NULL) {
    fprintf(stderr,
            "Cannot convert an IP address to a string: %s\n",
            strerror(errno));
    (void) close(cfd);
    return -1;
  }

  bytes_to_send = strlen(str);
  if (bytes_to_send < ((size_t) 1)) {
    (void) close(cfd);
    return 0;
  }

  res_send = send(cfd, str, bytes_to_send, 0);
  if (res_send < ((ssize_t) 0)) {
    fprintf(stderr, "Cannot send: %s\n",
            strerror(errno));
    (void) close(cfd);
    return -1;
  }

  if (close(cfd) < 0) {
    fprintf(stderr,
            "Cannot close a socket: %s\n",
            strerror(errno));
    return -1;
  }

  return 0;
}

int main(int argc, char **argv) {
  uint16_t port;
  int fd;

  // Argument check if failed print help
  if (argc < 2) {
    usage();
    return 1;
  }

  if (convert_port_name(&port, argv[1]) < 0) {
    fprintf(stderr,
            "Cannot convert \"%s\" to a valid TCP "
            "bind port.\n",
            argv[1]);
    return 1;
  }

  // creates listening socket
  fd = open_tcp_fd(port);
  if (fd < 0) return 1;


  // Server Perpetually 
  for (;;) {
    if (run_tool(fd) < 0) {
      if (close(fd) < 0) {
        fprintf(stderr,
                "Cannot close a socket: %s\n",
                strerror(errno));
      }
      return 1;
    }
  }

  if (close(fd) < 0) {
    fprintf(stderr,
            "Cannot close a socket: %s\n",
            strerror(errno));
    return 1;
  }

  return 0;
}
