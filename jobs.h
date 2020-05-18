#ifndef JOBS
#define JOBS

#include "baseHeaders.h"
#include "protocol.h"

typedef struct job {
  char name[1024]; // regular terminal doesn't accept lines of more than 1024 characters
  pid_t pid;
  int jobid;
  int status; // 0 = suspended, 1 = running, 2 = in queue, 3 = completed, 4 = aborted
  int sd;
  struct timeval tv;
  int exitStatus;
  job_limits jobLimits;
  struct job *nextJob;
} job;

// wait for a certain job to finish
void jobWaitPid(int pid);
// check if any job went over its designated limits
void checkLimits();
// clear the jobs that the client was managing
void clearClientJobs(int sd);
// set max_jobs
void setMaxJobs(int max);
// get number of running jobs
int getRunningJobs();
// check if maximum number of jobs currently running
int checkIfFull();
// inserts a new bg job into the array
int jobInsert(char *name, pid_t pid, int sd, int status, job_limits jobLimits);
// send signal N to job jobNumber
int killJob(int sd, int signal, int jobid);
// change nice priority of existing job
int changeNicePriority(int sd, int jobid, int niceAdd, int dontSend);
// send the exit status of a job to the client
int sendJobExitStatus(int sd, int jobid);
// prints out only one background running job
int printOneJob(int sd, int jobid);
// prints out all the background running jobs
int printAllJobs(int sd);
// goes through all the children and calls waitpid on them, updating them if they finished
void checkBackgroundJobs();
// frees all the jobs at the end of the program (when exit is called)
void freeAllJobs();

#endif
