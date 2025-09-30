#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

// Helper function to get the address from the sockaddr struct
void *get_in_addr(struct sockaddr *sa) {
  if (sa->sa_family == AF_INET) {
    return &(((struct sockaddr_in *)sa)->sin_addr);
  }
  return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char **argcv) {

  struct addrinfo hints, *servinfo;
  int status;

  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_flags = AI_PASSIVE;

  if ((status = getaddrinfo(NULL, "8080", &hints, &servinfo)) != 0) {
    fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
    exit(1);
  }
  int sockfd;
  struct addrinfo *p;

  // Loop through all the results and bind to the first we can
  for (p = servinfo; p != NULL; p = p->ai_next) {
    if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
      perror("server: socket");
      continue; // If socket creation fails, try the next address
    }

    if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
      close(sockfd); // Clean up the socket we just made
      perror("server: bind");
      continue; // If binding fails, try the next address
    }

    break;
  }

  if (p == NULL) {
    // This means we looped through the whole list and couldnt bind
    fprintf(stderr, "server: failed to bind\n");
    exit(1);
  }

  freeaddrinfo(servinfo);
  if (listen(sockfd, 10) == -1) { // its 10 because fuck you
    perror("listen");
    exit(1);
  }

  printf("server: waiting for connections...\n");

  int new_fd;
  struct sockaddr_storage their_addr; // Connector's address information
  socklen_t sin_size;
  char s[INET6_ADDRSTRLEN];

  while (1) { // Main accept() loop
    sin_size = sizeof their_addr;
    new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
    if (new_fd == -1) {
      perror("accept");
      continue; // Go back to the top of the loop and wait again
    }

    char buf[8192];
    int numbytes;

    // Read data from the client
    if ((numbytes = read(new_fd, buf, sizeof(buf) - 1)) == -1) {
      perror("read");
      close(new_fd); // Close the connection on error
      continue;      // Go wait for the next connection
    }

    buf[numbytes] = '\0';

    printf("server: received '%s'\n", buf);
    close(new_fd);
  }
}
