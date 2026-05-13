#include "httpImp.h"
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 3490
#define MAXDATASIZE 2097152 // 2MB

int main() {
  struct sockaddr_in client, server;
  int lfd, n, confd, num_bytes;
  char r_buff[MAXDATASIZE];
  char *s_buff;

  lfd = socket(AF_INET, SOCK_STREAM, 0);
  server.sin_family = AF_INET;
  server.sin_port = htons(PORT);
  server.sin_addr.s_addr = inet_addr("127.0.0.1");

  printf("waiting for conenction ... \n");

  bind(lfd, (struct sockaddr *)&server, sizeof server);
  listen(lfd, 1);

  n = sizeof client;
  confd = accept(lfd, (struct sockaddr *)&client, &n);

  while (1) {
    num_bytes = recv(confd, r_buff, sizeof r_buff, 0);
    r_buff[num_bytes] = '\0';
    s_buff = procces_request(r_buff);
    send(confd, s_buff, strlen(s_buff), 0);
    break;
  }

  close(confd);
  close(lfd);

  return 0;
}