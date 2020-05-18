#ifndef EXEC
#define EXEC

#include "baseHeaders.h"
#include "protocol.h"
#include "jobs.h"

int run_execvpe(char **cmdArgs, char **clientEnvp, int sd, job_limits jobLimits);
// void return # if builtin is run such as exit
int run_builtIn(to_server toServer, int sd);
int exec(to_server toServer, int sd);

#endif
