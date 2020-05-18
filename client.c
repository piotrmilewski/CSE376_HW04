#include "client.h"

int connectToServer(){
  int socket_fd, retVal;
  socklen_t size;
  struct sockaddr_un server_sockaddr;

  // create a UNIX domain stream socket
  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0){
    perror("socket");
    return -1;
  }

  // setup the UNIX sockaddr struct for server
  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path, SOCKET_PATH);
  size = sizeof(server_sockaddr);

  // connect to the server socket
  retVal = connect(socket_fd, (struct sockaddr *)&server_sockaddr, size);
  if (retVal < 0){
    perror("connect");
    return -1;
  }

  return socket_fd;
}

