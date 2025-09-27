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
	  "ip-tool-server <UDP bind port>\n\n");
}

static int convert_port_name(uint16_t *port,
			     const char *port_name) {
  char *end;
  long long int nn;
  uint16_t t;
  long long int tt;

  if (port_name == NULL) return -1;
  if (*port_name == '\0') return -1;
  nn = strtoll(port_name, &end, 0);
  if (*end != '\0') return -1;
  if (nn < ((long long int) 0)) return -1;
  t = (uint16_t) nn;
  tt = (long long int) t;
  if (tt != nn) return -1;
  *port = t;
  return 0;
}

static int open_udp_fd(uint16_t port) {
  int fd;
  struct sockaddr_in serveraddr;
  
  fd = socket(AF_INET, SOCK_DGRAM, 0);
  if (fd < 0) {
    fprintf(stderr,
	    "Cannot create a socket: %s\n",
	    strerror(errno));
    return -1;
  }

  memset(&serveraddr, 0, sizeof(serveraddr));
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
  
  return fd;
}

static int run_tool(int fd) {
  struct sockaddr_in clientaddr;
  socklen_t clientaddrlen;
  ssize_t recv_res;
  uint32_t magic;
  char str[257];
  size_t bytes_to_send;
  ssize_t res_send;
    
  clientaddrlen = (socklen_t) sizeof(clientaddr);
  recv_res = recvfrom(fd, &magic, sizeof(magic), 0,
		      (struct sockaddr *) &clientaddr,
		      &clientaddrlen);
  if (recv_res < ((ssize_t) 0)) {
    fprintf(stderr,
	    "Cannot receive a packet: %s\n",
	    strerror(errno));
    return -1;
  }
  if (((size_t) clientaddrlen) != sizeof(clientaddr)) {
    return 0;
  }
  if (((size_t) recv_res) != sizeof(magic)) {
    return 0;
  }
  
  magic = ntohl(magic);
  if (magic != (MAGIC)) {
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
    return -1;
  }

  bytes_to_send = strlen(str);

  if (bytes_to_send < ((size_t) 1)) {
    return 0;
  }
  
  res_send = sendto(fd, str, bytes_to_send, 0,
		    (struct sockaddr *) &clientaddr,
		    (socklen_t) sizeof(clientaddr));
  if (res_send < ((ssize_t) 0)) {
    fprintf(stderr, "Cannot send: %s\n",
	    strerror(errno));
    return -1;
  }
  return 0;
}

int main(int argc, char **argv) {
  uint16_t port;
  int fd;

  if (argc < 2) {
    usage();
    return 1;
  }

  if (convert_port_name(&port, argv[1]) < 0) {
    fprintf(stderr,
	    "Cannot convert \"%s\" to a valid UDP "
	    "bind port.\n",
	    argv[1]);
    return 1;
  }

  fd = open_udp_fd(port);
  if (fd < 0) return 1;

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