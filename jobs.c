#include "jobs.h"

static job *root = NULL;
static job *tail = NULL;
static int currJobNumber = 0;
static int max_jobs = 0;
static int running_jobs = 0;

void setMaxJobs(int max){
  max_jobs = max;
}

int getRunningJobs(){
  return running_jobs;
}

// return 1 if can't run job at the moment
int checkIfFull(){
  if (max_jobs == running_jobs)
    return 1;
  return 0;
}

void jobWaitPid(int pid){
  int status;
  int bgpid = waitpid(pid, &status, 0);
  if (bgpid < 0)
    perror("waitpid");
  if (pid == bgpid){
    if (WIFSIGNALED(status))
      running_jobs--;
  }
}

void checkLimits(){
  job *node = root;
  int fd, peak, retVal;
  char buf[4096], child_status[4096];
  char *vm;

  while (node != NULL){
    if (node->status == 1){
      // CHECK IF WENT OVER MEMORY LIMIT
      // open up file with info on pid
      sprintf(child_status, "/proc/%d/status", node->pid);
      fd = open(child_status, O_RDONLY);
      if (fd < 0){
        perror("open");
        return;
      }

      // get info on pid
      retVal = read(fd, buf, 4095);
      if (retVal < 0){
        perror("read");
        return;
      }
      buf[4095] = '\0';
      close(fd);

      // get peak memory usage
      vm = strstr(buf, "VmPeak:");
      peak = 0;
      if (vm)
        sscanf(vm, "%*s %d", &peak);

      // kill job if went over memory limit
      if (node->jobLimits.maxMemory != 0 && peak > node->jobLimits.maxMemory){ 
        killJob(node->sd, SIGKILL, node->jobid);
        printf("job of id %d has exceeded its memory limit\n", node->jobid);
        jobWaitPid(node->pid);
        node->status = 4;
        node->exitStatus = EXIT_FAILURE;
      }
      else{
        // CHECK IF WENT OVER TIME LIMIT
        // get the current time
        struct timeval currTime;
        gettimeofday(&(currTime), NULL);

        // kill job if went over time limit
        if (node->jobLimits.maxTime != 0 && 
            (currTime.tv_sec - node->tv.tv_sec) > node->jobLimits.maxTime){
          killJob(node->sd, SIGKILL, node->jobid);
          printf("job of id %d has exceeded its time limit\n", node->jobid);
          jobWaitPid(node->pid);
          node->status = 4;
          node->exitStatus = EXIT_FAILURE;
        }
      }
    }
    node = node->nextJob;
  }
}

void clearClientJobs(int sd){
  job *node = root;
  job *prev = NULL;
  job *toFree = NULL;
  while (node != NULL){
    if (node->sd == sd){
      if (node->status == 1){ // kill the running job
        killJob(node->sd, SIGKILL, node->jobid);
        // pickup the zombie before we free the node
        jobWaitPid(node->pid);
        printf("Background job removed due to client closing\n");
      }
    }
    if (node->sd == sd && prev == NULL){ // found root node to remove
      toFree = node;
      node = node->nextJob;
      root = root->nextJob; // set root to nextJob
      free(toFree);
    }
    else if (node->sd == sd && prev != NULL){
      toFree = node;
      node = node->nextJob;
      prev->nextJob = node; // set prev->nextJob to skip a job
      free(node);
    }
    else{
      prev = node;
      node = node->nextJob;
    }
  }
}

// inserts a new bg job into the array
int jobInsert(char *name, pid_t pid, int sd, int status, job_limits jobLimits){

  // create new job
  job *new = (job *)malloc(sizeof(job));
  // add name through array
  strcpy(new->name, name);
  new->pid = pid;
  new->jobid = currJobNumber++;
  new->status = status;
  new->sd = sd;
  new->exitStatus = -1;
  gettimeofday(&(new->tv), NULL); // set creation time
  new->jobLimits = jobLimits;
  new->nextJob = NULL;
  // insert new job
  if (root == NULL){
    root = new;
    tail = new;
  }
  else{
    tail->nextJob = new;
    tail = new;
  }
  if (status == 1){
    running_jobs++;
  }

  changeNicePriority(sd, new->jobid, new->jobLimits.priorityValue, 1);

  return new->jobid;
}

// send signal N to job jobNumber
// return 0 if successfully sent signal, -1 otherwise
int killJob(int sd, int signal, int jobid){
  job *node = root;
  while (node != NULL){
    if (node->sd == sd && node->jobid == jobid){
      kill(node->pid, signal);
      return 0;
    }
    else
      node = node->nextJob;
  }
  return -1;
}

// change nice priority of existing job
int changeNicePriority(int sd, int jobid, int niceAdd, int dontSend){
  int foundJob = 0;
  int niceVal = -1;
  int retVal = 0;
  char buf[4096];
  memset(buf, 0, sizeof(buf));
  job *node = root;
  while (node != NULL){
    if (node->sd == sd && node->jobid == jobid){
      foundJob = 1;
      
      // get previous nice value
      niceVal = getpriority(PRIO_PROCESS, node->pid);
      if (niceVal >= 0){
        // update nice value
        retVal = setpriority(PRIO_PROCESS, node->pid, niceVal+niceAdd);
      
        // get updated nice value
        niceVal = getpriority(PRIO_PROCESS, node->pid);
      
        // check if succeeded in changing nice value
        if (retVal < 0) // failure in setting nice value
          sprintf(buf, "Could not change nice value, current nice value: %d\n", niceVal);
        else
          sprintf(buf, "Success in changing nice value, current nice value: %d\n", niceVal);
      }
      node = NULL;  
    }
    else
      node = node->nextJob;
  }
  // in case jobid provided doesn't exist
  if (foundJob == 0)
    sprintf(buf, "%s\n", "Could not change nice value, job doesn't exist");

  if (dontSend == 0){
    // send message back to the client
    to_client toClient = createClientProtocol(2, 0, buf);
    retVal = send(sd, &toClient, sizeof(toClient), 0);
  }

  return retVal;
}

// send the exit status of a job to the client
int sendJobExitStatus(int sd, int jobid){
  int retVal;
  int foundJob = 0;
  char buf[4096];
  memset(buf, 0, sizeof(buf));
  job *node = root;
  while (node != NULL){
    if (node->sd == sd && node->jobid == jobid){
      foundJob = 1;

      // check if job is finished running
      if (node->exitStatus == -1)
        sprintf(buf, "%s\n", "Cannot retrieve exit status, job has not yet terminated");
      else
        sprintf(buf, "Exit Status of job #%d: %d\n", jobid, node->exitStatus);

      node = NULL;
    }
    else
      node = node->nextJob;
  }
  // in case jobid provided doesn't exist
  if (foundJob == 0)
    sprintf(buf, "%s\n", "Cannot retrieve exit status, job doesn't exist");

  // send message back to the client
  to_client toClient = createClientProtocol(2, 0, buf);
  retVal = send(sd, &toClient, sizeof(toClient), 0);

  return retVal;
}

// print out only one background running job
int printOneJob(int sd, int jobid){
  char buf[4096];
  char temp[2056];
  memset(buf, 0 , sizeof(buf));
  strcat(buf, "job number, name, pid, status, priority value\n");
  job *node = root;
  while (node != NULL){
    if (node->sd == sd && node->jobid == jobid){
      memset(temp, 0, sizeof(temp));
      sprintf(temp, "%d, %s, %d, %s, %d\n", node->jobid, node->name, node->pid, 
              node->status == 0 ? "suspended" : node->status == 1 ? "running" : 
              node->status == 2 ? "in queue" : node->status == 3 ? "completed" : "aborted",
              getpriority(PRIO_PROCESS, node->pid));
      strcat(buf, temp);
    }
    node = node->nextJob;
  }
  to_client toClient = createClientProtocol(2, 0, buf);
  int retVal = send(sd, &toClient, sizeof(toClient), 0);
  return retVal;
}

// prints out all the background running jobs
int printAllJobs(int sd){
  char buf[4096];
  char temp[2056];
  memset(buf, 0 , sizeof(buf));
  strcat(buf, "job number, name, pid, status, priority value\n");
  job *node = root;
  while (node != NULL){
    if (node->sd == sd){
      memset(temp, 0, sizeof(temp));
      sprintf(temp, "%d, %s, %d, %s, %d\n", node->jobid, node->name, node->pid, 
              node->status == 0 ? "suspended" : node->status == 1 ? "running" : 
              node->status == 2 ? "in queue" : node->status == 3 ? "completed" : "aborted",
              getpriority(PRIO_PROCESS, node->pid));
      strcat(buf, temp);
    }
    node = node->nextJob;
  }
  to_client toClient = createClientProtocol(2, 0, buf);
  int retVal = send(sd, &toClient, sizeof(toClient), 0);
  return retVal;
}

// goes through all the children and calls waitpid on them, updating them if they finished
void checkBackgroundJobs(){
  job *node = root;
  int status;
  int bgpid;
  // check if any job violated any limits
  checkLimits();
  // check if any jobs finished
  while (node != NULL){
    if (node->exitStatus == -1 && node->status != 2){
      bgpid = waitpid(node->pid, &status, WNOHANG | WUNTRACED | WCONTINUED);
      if (bgpid < 0)
        perror("waitpid");
      if (node->pid == bgpid){
        if (WIFEXITED(status)){
          node->exitStatus = WEXITSTATUS(status);
          node->status = 3;
          running_jobs--;
        }
        else if (WIFSIGNALED(status)){
          printf("Background program aborted\n");
          node->exitStatus = WTERMSIG(status);
          node->status = 4;
          running_jobs--;
        }
        else if (WIFSTOPPED(status)){
          printf("Background Program suspended\n");
          node->status = 0;
          running_jobs--;
        }
        else if (WIFCONTINUED(status)){
          printf("Background Program continued\n");
          node->status = 1;
          running_jobs++;
        }
      }
    }
    node = node->nextJob;
  }
  // startup any jobs if there are any
  node = root;
  while (node != NULL && running_jobs < max_jobs){
    if (node->status == 2){
      // startup the job
      printf("starting up job in queue\n");
      killJob(node->sd, SIGCONT, node->jobid);
      // perform a waitpid for the job being restarted
      bgpid = waitpid(node->pid, &status, WCONTINUED);
      if (bgpid < 0)
        perror("waitpid");
      if (node->pid == bgpid){
        if (WIFCONTINUED(status)){
          printf("Background Program started up\n");
          node->status = 1;
          running_jobs++;
        }
      }
    }
    node = node->nextJob;
  }
}


// frees all the jobs at the end of the program (when exit is called)
void freeAllJobs(){
  job *node;
  while (root != NULL){
    node = root; // store node to free
    root = root->nextJob; // move to the next node
    free(node); // free the stored node
  }
}

