#include "client.h"

int main(int argc, char *argv[], char **envp){

  int client_socket, retVal;
  client_socket = -1; // to remove uninitialized error
  int mainReturn = 0;
  to_client toClient;
  to_server toServer;

  // setup the socket
  client_socket = connectToServer();
  if (client_socket < 0)
    goto out; 

  // create data to be sent
  char *clientArg[5];
  clientArg[0] = "sleep";
  clientArg[1] = "22";
  clientArg[2] = NULL;
  toServer = serverProtocolNewJob(1, 2, clientArg, envp, 0, 0, 4);

  // tell server to run "sleep 21"
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);

  // tell server to list all jobs that the client sent out
  toServer = serverProtocolRequest(2, 0, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);

  sleep(200);

  // close the sockets and exit
  goto clean_out;

send:
  perror("send");
  goto out;

recv:
  perror("recv");
  goto out;

out:
  mainReturn = 1;

clean_out:
  if (client_socket >= 0)
    close(client_socket);

  return mainReturn;
}

