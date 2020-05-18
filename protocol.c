#include "protocol.h"

to_server serverProtocolNewJob(char commandType, int argc, char **argv, char **envp, 
                               int maxTime, int maxMemory, int priorityValue){
  to_server toServer;
  toServer.commandType = commandType;
  toServer.argc = argc;
  toServer.optional = 0;
  toServer.jobid = 0;
  
  // store argv and envp as strings separated by \0's
  toServer.argv[0] = '\0';
  toServer.envp[0] = '\0';
  if (argc != 0){
    for (int i = 0; i < argc; i++){
      strcat(toServer.argv, argv[i]);
      strcat(toServer.argv, "\n");
    }
  }
  if (envp != NULL){
    while (*envp != NULL){
      strcat(toServer.envp, *envp);
      strcat(toServer.envp, "\n");
      envp++;
    }
  }
  
  // setup job_limits struct
  toServer.jobLimits.maxTime = maxTime;
  toServer.jobLimits.maxMemory = maxMemory;
  toServer.jobLimits.priorityValue = priorityValue;
  return toServer;
}

to_server serverProtocolRequest(char commandType, int jobid, int optional){
  to_server toServer;
  toServer.commandType = commandType;
  toServer.jobid = jobid;
  toServer.optional = optional;

  // fill the rest as blanks
  toServer.argc = 0;
  toServer.argv[0] = '\0';
  toServer.envp[0] = '\0';
  toServer.jobLimits.maxTime = 0;
  toServer.jobLimits.maxMemory = 0;
  toServer.jobLimits.priorityValue = 0;
  return toServer;
}


to_client createClientProtocol(char replyType, int optional, char *buf){
  to_client toClient;
  toClient.replyType = replyType;
  toClient.optional = optional;
  memset(toClient.buf, 0, sizeof(toClient.buf));

  // set buf for a message to the client
  if (buf != NULL)
    strcpy(toClient.buf, buf);
  return toClient;
}

int parseProtocol(char *input, char **parsed){

  int index = 0;
  char *found;

  // get the first word
  found = strtok(input, "\n");

  // get the rest of the words
  while (NULL != found){
    parsed[index] = found;
    index++;
    found = strtok(NULL, "\n");
  }

  // store an empty string at the end
  parsed[index] = NULL;

  return index; // return num of cmdArgs
}
