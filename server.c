#include "server.h"

int setupSocket(){
  int socket_fd, retVal;
  socklen_t size;
  struct sockaddr_un sockaddr;

  // create a UNIX domain stream socket
  socket_fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (socket_fd < 0){
    perror("socket");
    return -1;
  }

  // setup the UNIX sockaddr struct
  sockaddr.sun_family = AF_UNIX;
  strcpy(sockaddr.sun_path, SOCKET_PATH);
  size = sizeof(sockaddr);

  // unlink the SOCKET_PATH file to ensure that bind succeeds
  unlink(SOCKET_PATH);
  retVal = bind(socket_fd, (struct sockaddr *)&sockaddr, size);
  if (retVal < 0){
    perror("bind");
    close(socket_fd);
    return -1;
  }
  
  return socket_fd;
}

int main(int argc, char *argv[]){

  int opt = 0;
  int max_jobs = 10;

  //USAGE: server [-j MAXJOBS]
  while ((opt = getopt(argc, argv, "j:")) != -1){
    switch (opt) {
      case 'j':
        if (sscanf(optarg, "%d", &max_jobs) != 1){
          printf("usage: server [-j MAXJOBS]\n\
                  -j MAXJOBS: set the maximum number of jobs to MAXJOBS\n");
        }
        break;
      default:
        printf("usage: server [-j MAXJOBS]\n\
                -j MAXJOBS: set the maximum number of jobs to MAXJOBS\n");
    }
  }

  // set max_jobs
  setMaxJobs(max_jobs);

  int mainReturn = 0;
  int server_socket, retVal, largest_sd;
  int backlog = 256; // limit to how many pending connections
  int client_max = 256;
  int client_socket[client_max]; // max of 256 clients
  fd_set fds; // set of socket descriptors
  struct sockaddr_un sockaddr;
  socklen_t size = sizeof(sockaddr);
  //char buf[1024];

  // set all client sockets to 0 
  for (int i = 0; i < client_max; i++)
    client_socket[i] = 0;

  // create the server socket
  server_socket = setupSocket();
  if (server_socket < 0)
    goto out;

  retVal = listen(server_socket, backlog);
  if (retVal < 0){
    perror("listen");
    goto out;
  }

  int RUNNING = 1;
  while (RUNNING){
   
    // clear the socket set
    FD_ZERO(&fds);

    // add the server socket to the set
    FD_SET(server_socket, &fds);
    largest_sd = server_socket;

    // add child sockets to the set
    for (int i = 0; i < client_max; i++){
      int sd = client_socket[i];

      // if socket descriptor is valid then add it to the set of FDs
      if (sd > 0)
        FD_SET(sd, &fds);

      if (sd > largest_sd)
        largest_sd = sd;
    }

    // wait for activity on one of the sockets
    retVal = select(largest_sd+1, &fds, NULL, NULL, NULL);

    // update job status
    checkBackgroundJobs();

    if ((retVal < 0) && (errno != EINTR))
      goto out;

    // check if there is an incoming connection on the server socket
    if (FD_ISSET(server_socket, &fds)){
      int new_socket = accept(server_socket, (struct sockaddr *)&sockaddr, &size);
      if (new_socket < 0){
        perror("accept");
        goto out;
      }

      // add new socket to the array of sockets
      for (int i = 0; i < client_max; i++){
        // insert if position is empty
        if (client_socket[i] == 0){
          client_socket[i] = new_socket;
          break;
        }
      }
    }
    
    // check if there is some IO operation on one of the client sockets
    for (int i = 0; i < client_max; i++){
      int sd = client_socket[i];

      if (FD_ISSET(sd, &fds)){
        // check to see if the client disconnected
        to_server toServer;
        retVal = recv(sd, &toServer, sizeof(toServer), 0);
        if (retVal == 0){
          printf("Client disconnected\n");
          clearClientJobs(sd);
          close(sd);
          client_socket[i] = 0;
        }
        else{
          // execute based off command type
          exec(toServer, sd);
        }
      }
    }

    // update job status (run again in case signal was sent)
    checkBackgroundJobs();

    printf("Current number of running jobs: %d\n", getRunningJobs());
  }

  goto clean_out;

out:
  mainReturn = 1;

clean_out:
  if (server_socket >= 0)
    close(server_socket);
  for (int i = 0; i < client_max; i++){
    if (client_socket[i] > 0)
      close(client_socket[i]);
  }

  return mainReturn;
}
