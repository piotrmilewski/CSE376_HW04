#ifndef PROTOCOL
#define PROTOCOL

#include "baseHeaders.h"

typedef struct job_limits {
  int maxTime;
  int maxMemory;
  int priorityValue;
} job_limits;

typedef struct to_server {
  char commandType;
  int jobid;
  int optional;
  int argc;
  char argv[4096];
  char envp[4096];
  job_limits jobLimits;
} to_server;

typedef struct to_client {
  char replyType;
  int optional;
  char buf[4096];
} to_client;

to_server serverProtocolNewJob(char commandType, int argc, char **argv, char **envp,
                         int maxTime, int maxMemory, int priorityValue);
to_server serverProtocolRequest(char commandType, int jobid, int optional);
to_client createClientProtocol(char replyType, int optional, char *buf);
int parseProtocol(char *input, char **parsed);

#endif
