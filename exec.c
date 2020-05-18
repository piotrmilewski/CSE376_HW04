#include "exec.h"

int run_execvpe(char **cmdArgs, char **clientEnvp, int sd, job_limits jobLimits){

  char buf[4096];
  int optional = 0;
  memset(buf, 0, sizeof(buf)); 
  
  // turn array into name
  char name[1024];
  int jobid = 0;
  strcpy(name, cmdArgs[0]);
  int i = 1;
  while (cmdArgs[i] != NULL){
    strcat(name, " ");
    strcat(name, cmdArgs[i]);
    i++;
  }

  pid_t pid = fork();

  // check if fork failed
  if (pid < 0)
    perror("Fork failure, not executing command\n");

  // child
  else if (pid == 0) {
    if (execvpe(cmdArgs[0], cmdArgs, clientEnvp) < 0)
      exit(EXIT_FAILURE);
  }
  // parent
  else{
    if (checkIfFull() == 0){ // currently have space for another job
      // add the job to the jobs table
      jobid = jobInsert(name, pid, sd, 1, jobLimits);
       // let the client know the job was added
      sprintf(buf, "%s\n", "Job request accepted, now running");    
      optional = 0;
    }
    else{
      // add job to the jobs table to be run later
      jobid = jobInsert(name, pid, sd, 2, jobLimits);

      // stop the job, continue it when there is room
      killJob(sd, SIGTSTP, jobid);
     
      // let the client know the job is in queue to be run
      sprintf(buf, "%s\n", "Job request accepted, job is in queue to be run");
      optional = 1;
    }
  }

  // only parent can continue here
  to_client toClient = createClientProtocol(1, optional, buf);
  int retVal = send(sd, &toClient, sizeof(toClient), 0);
  if (retVal < 0){
    perror("send");
    return -1;
  }
  return 0;
}

int run_builtIn(to_server toServer, int sd){
  char buf[4096];
  to_client toClient;
  int sendRetVal = 0;

  if (toServer.commandType == 2) // list all jobs
    sendRetVal = printAllJobs(sd);
  else if (toServer.commandType == 3) // list one job
    sendRetVal = printOneJob(sd, toServer.jobid);
  else if (toServer.commandType >= 4 && toServer.commandType <= 6){
    int signal;
    if (toServer.commandType == 4) // kill existing job
      signal = SIGKILL;
    else if (toServer.commandType == 5) // suspend existing job
      signal = SIGTSTP;
    else // continue existing job
      signal = SIGCONT;
    memset(buf, 0, sizeof(buf));
    if (killJob(sd, signal, toServer.jobid) == 0)
      strcat(buf, "Successfully sent signal to job\n");
    else
      strcat(buf, "Not able to send signal to job at this time\n");
    toClient = createClientProtocol(2, 0, buf);
    sendRetVal = send(sd, &toClient, sizeof(toClient), 0);
  }
  else if (toServer.commandType == 7){ // change nice priority of job
    sendRetVal = changeNicePriority(sd, toServer.jobid, toServer.optional, 0);
  }
  else if (toServer.commandType == 8) // return status of jobid
    sendRetVal = sendJobExitStatus(sd, toServer.jobid);

  // check if packet was sent
  if (sendRetVal < 0){
    perror("send");
    return -1;
  }
  return 0;
}

int exec(to_server toServer, int sd){

  // check if submitting a new job
  if (toServer.commandType == 1){

    char *cmdArgs[1024]; // can hold up to 1024 commands
    // separate words in input by "\n" into array
    int numArgs = parseProtocol(toServer.argv, cmdArgs);

    char *clientEnvp[1024]; // can gold up to 1024 envps
    int numEnvp = parseProtocol(toServer.envp, clientEnvp);

    // check for blank input
    if (numArgs == 0 || numEnvp == 0)
      return -1;

    return run_execvpe(cmdArgs, clientEnvp, sd, toServer.jobLimits); 
  }
  else if (toServer.commandType > 1 && toServer.commandType <= 9){
    int retVal = run_builtIn(toServer, sd);
    return retVal;
  }
  else
    return -1;

  return 0; // default return
}
