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
  clientArg[0] = "echo";
  clientArg[1] = "hello";
  clientArg[2] = NULL;
  toServer = serverProtocolNewJob(1, 2, clientArg, envp, 0, 0, 4);

  // tell server to run "sleep 20"
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  usleep(200); // wait for unix to finish job

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);

  // tell server to run "sleep 20"
  clientArg[0] = "sleep";
  clientArg[1] = "20";
  toServer = serverProtocolNewJob(1, 2, clientArg, envp, 0, 0, 2);
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

  // tell server to list one job that the client sent out
  toServer = serverProtocolRequest(3, 0, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);

  // tell server to increase the priority of the second job
  toServer = serverProtocolRequest(7, 1, 1);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);

  // return exit status of the first job
  toServer = serverProtocolRequest(8, 0, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  // read the data sent from the server
  retVal = recv(client_socket, &toClient, sizeof(toClient), 0);
  if (retVal < 0)
    goto recv;
  printf("%s", toClient.buf);
  
  // stop the second job
  toServer = serverProtocolRequest(5, 1, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  usleep(200); // wait for unix to handle signal

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

  // continue the second job
  toServer = serverProtocolRequest(6, 1, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  usleep(200); // wait for unix to handle signal

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

  // kill the second job
  toServer = serverProtocolRequest(4, 1, 0);
  retVal = send(client_socket, &toServer, sizeof(toServer), 0);
  if (retVal < 0)
    goto send;

  usleep(200); // wait for unix to handle signal

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

