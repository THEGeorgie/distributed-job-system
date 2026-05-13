#include "httpImp.h"

#define PORT 3490           // the port client will be connecting to
#define MAXDATASIZE 2097152 // 2MB

#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

int main(int argc, char *argv[]) {
  if (argc != 4) {
    perror("usage: client hostname, directory of program,  directory "
           "of data(must be a csv file)\n");
    exit(1);
  }
  struct sockaddr_in server;
  int lfd;
  char r_buff[MAXDATASIZE] = "";
  char *s_buff;

  lfd = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = inet_addr(argv[1]);

  if (connect(lfd, (struct sockaddr *)&server, sizeof server) < 0) {
    perror("Unable to establish a connection with the server");
    return 2;
  }

  while (1) {
    s_buff = create_request("POST", "/process", argv[3], argv[2]);
    if (s_buff == NULL) {
      perror("Failed creating a request.");
      break;
    }
    send(lfd, s_buff, strlen(s_buff), 0);
    if (strcmp(s_buff, "end") == 0)
      break;

    recv(lfd, r_buff, sizeof r_buff, 0);
    printf("[server] %s", r_buff);
    break;

    printf("\n");
  }

  close(lfd);

  return 0;
}