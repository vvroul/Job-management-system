#define _POSIX_SOURCE
#include "Headers.hpp"
#include "PoolNode.hpp"
#include "Utilities.hpp"

#define MAXBUFF 1024
#define PERMS   0666
#define POOL_SIZE 50
extern int errno;

void PoolHandler(int poolCounter, int jobCounter, char* poolcwd, char* poolcwd1, char* token, char* outDirectory, char* cwd1, PoolNode* poolNode, Info* infoNode, JobNode* jobNode);


void catcher(int signum) 
{
	switch (signum) 
	{
		case SIGUSR1: cout << "Catcher caught SIGUSR1" << endl;
		              break;
		case SIGUSR2: cout << "Catcher caught SIGUSR2" << endl;
		              break;
      	case SIGTERM: cout << "Catcher caught SIGTERM" << endl;
      				  break;
		default:      cout << "Catcher caught unexpected signal " << signum << endl;
	}
}


int main(int argc, char const *argv[])
{
    cout << "Initializing firmware........Firmware functional." << endl;
    cout << "System check............Passed." << endl;
    cout << "Initiating plain language interface...Done." << endl;
    cout << "Loading library session...Done." << endl;
    cout << "Loading \'jms_coord\' executable program....Done." << endl;
    cout << "Ready." << endl << endl << endl;

    sigset_t sigset;
	struct sigaction sact;
	time_t t;
    ifstream fifofile("samplein.txt");
    ofstream fifoOutFile;
    char* outDirectory = new char[300]();
    char* theDir = new char[300]();
    char* dirname = new char[300]();
    char* jmsInput = new char[300]();
    char* jmsOutput = new char[300]();
    char* tempPid = new char[300]();
    char* theSlash = new char[3]();
    char* submitjpid = new char[50]();
    char* submitpid = new char[50]();
    char* tempLogicNumber = new char[100]();
    char cwd[256];
	char cwd1[256];
	char cwdp[256];
	char cwdp1[256];
    strcpy(theSlash, "/");
    char* firstSlash = new char[3]();
    strcpy(firstSlash, "/");
    int maxNumOfJobs;
    int readfd, writefd;
    int poolreadfd, poolwritefd;
    int suspendFlag = 0;

    if (argc > 8)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-l") == 0)			//main directory for output
			{
				strcpy(outDirectory, argv[i+1]);
				//cout << "Output directory's name : " << outDirectory << endl;
				struct stat st = {0};
				if (stat(outDirectory, &st) == -1)
				{
					//Going to create the directory
			    	int res = mkdir(outDirectory, 0700);
			    	if(res == 0)
			    	{
			    		cout << "Directory " << outDirectory << " created successfully! " << res << endl;
			    		strcat(outDirectory, theSlash);
			    		strcat(firstSlash, outDirectory);
			    	}
	    	    	else
	    	    	{
	    	    		cout << "Error while creating the directory " << endl;
	    	    		exit(6);
	    	    	}
	    		}
	    		else
	    		{
	    			cout << "The directory already exists" << endl;
		    		strcat(outDirectory, theSlash);
		    		strcat(firstSlash, outDirectory);
	    		}
				++i;
			}
			else if (strcmp(argv[i], "-n") == 0)	//max number of jobs under pool
			{
				maxNumOfJobs = atoi(argv[i+1]);
				cout << "Maximum number of jobs : " << maxNumOfJobs << endl;
				++i;
			}
			else if (strcmp(argv[i], "-w") == 0)	//named pipe for record writing 
			{
				strcpy(jmsOutput, argv[i+1]);
				cout << "Output named pipe : " << jmsOutput << endl;
				++i;
			}
			else if (strcmp(argv[i], "-r") == 0)	//named pipe for record reading
			{
				strcpy(jmsInput, argv[i+1]);
				cout << "Input named pipe : " << jmsInput << endl;
				++i;
			}
			else
			{
				cout << "You've given wrong input" << endl;
				exit(6);
			}
		}
	}
	else
	{
		cout << "------>  Suggested input: $./jms_coord -l <path> -n <jobs_pool> -w <jms_out> -r <jms_in>" << endl;
		exit(1);
	}

    cout << "*********** Welcome to JMS Coord! ***********" << endl;

    //srand(time(NULL));
	std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
	std::cout.precision(20);

	int fd, rc, fdr, poolfd, poolfdr, coordfd, coordfdr;
	int bytes_in = 0;
	int helloSus = 13;
	int n;
	int coordrc;
	char buff[MAXBUFF];
	char coordbuff[MAXBUFF];
	struct pollfd fdarray[1];
	int jobCounter = 0;
	int poolStatus;
	int poolCounter = 0, poolNumber = 0;
	int* jobsArray = new int[maxNumOfJobs];
	char instr[150];
	char* token = new char[300]();
	char* jobtoken = new char[300]();
	char* argtoken = new char[300]();
	char* restToken;
	pid_t child_pid, pool_pid, waitpoolid;
	struct PoolNode* poolNode;
	struct PoolNode* oldpoolNode;
	struct Info* infoNode = (Info*) malloc(sizeof(struct Info));
	poolNode = (PoolNode *)malloc(POOL_SIZE * sizeof(struct PoolNode));
	//oldpoolNode = (PoolNode *)malloc(poolSize * sizeof(struct PoolNode));
	struct JobNode* jobNode = (JobNode*) malloc(POOL_SIZE * maxNumOfJobs * sizeof(struct JobNode));

	if (getcwd(cwd, sizeof(cwd)) != NULL)
	{
		cout << "Current working dir:" << cwd << endl;
	}
	else
	{
		cout << "getcwd() error" << endl;
	}
	
	strcat(cwd, "/");
	strcpy(cwd1, cwd);
	strcpy(cwdp, cwd);
	strcpy(cwdp1, cwd);
	strcpy(theDir, cwd);
	strcat(cwd, jmsInput);
	strcat(cwd1, jmsOutput);
	strcat(cwdp, "poolFifoIn_");
	strcat(cwdp1, "poolFifoOut_");
	strcat(theDir, outDirectory);

	/* Creation of named pipes */
	readfd = mkfifo(cwd, PERMS);
	if ((readfd < 0) && (errno != EEXIST))
	{
		cerr << "Error while creating the input named pipe fifo file." << endl;
		unlink(cwd);
		exit(10);
	}

	writefd = mkfifo(cwd1, PERMS);
	if ((writefd < 0) && (errno != EEXIST))
	{
		cerr << "Error while creating the output named pipe fifo file." << endl;
		unlink(cwd1);
		exit(10);
	}

	//cout << "Named pipes created successfully!" << endl;
	cout << "Path 1 : " << cwd << endl;
	cout << "Path 2 : " << cwd1 << endl;

	fd = open(cwd, O_RDONLY | O_NONBLOCK);
	if (fd < 0)
	{
		cerr << "Coord can't open input named pipe." << endl;
	}

	cout << "Both named pipes opened successfully!" << endl;


	sigemptyset(&sact.sa_mask);
	sact.sa_flags = 0;
	sact.sa_handler = catcher;
	if (sigaction(SIGUSR1, &sact, NULL) != 0)
		perror("1st sigaction() error");

	if (sigaction(SIGUSR2, &sact, NULL) != 0)
		perror("2nd sigaction() error");


	while (1)
	{
		rc = read(fd, buff, MAXBUFF);

		if (rc > 0)
		{
			//cout << "Read from console : " << buff << endl;
			token = strtok(buff, " ");
			if (strcmp(token, "submit") == 0)
			{
				token = strtok(NULL, " ");
				strcpy(jobtoken, token);
				while (token)
				{
					//cout << "Job : " << token << endl;
					++jobCounter;
					infoNode->jobCounter = jobCounter;

					if ((token = strtok(NULL, " ")) != NULL)
					{
						strcpy(argtoken, token);
						strcat(jobtoken, " ");
						strcat(jobtoken, argtoken);
						token = strtok(NULL, " ");
					}
					else
					{
					}

					//cout << "JOB TOKEN !!!!!!!!!!! : " << jobtoken << endl;

					//****************************  Pool Creation  **************************** 
					if (poolCounter == 0)
					{
						//pool_pid = fork();
						int poolrc;
						char poolcwd[256];
						char poolcwd1[256];
						char poolbuff[MAXBUFF];
						char resultbuff[MAXBUFF];		
						strcpy(poolcwd, cwdp);
						strcpy(poolcwd1, cwdp1);
						sprintf(tempPid, "%d", poolCounter);
						strcat(poolcwd, tempPid);
						strcat(poolcwd1, tempPid);

						PoolHandler(poolCounter, jobCounter, poolcwd, poolcwd1, jobtoken, theDir, cwd1, poolNode, infoNode, jobNode);

						jobNode[jobCounter-1].state = -1;
						--poolNode[poolCounter].activeJobsOfPool;
						++poolCounter;
						infoNode->numberOfPools = poolCounter;
					}
					else if (poolCounter > 0)
					{
						int poolrc;
						char poolcwd[256];
						char poolcwd1[256];
						char poolbuff[MAXBUFF];
						char coordbuffsame[MAXBUFF];		
						strcpy(poolcwd, cwdp);
						strcpy(poolcwd1, cwdp1);
						int coordfdsame, coordfdrsame, coordrcsame;

						int poolCounterCopy = poolCounter;
						for (int i = 0; i < poolCounter; ++i)
						{
							//cout << "We already have pool : " << i << endl; 
							//check current number of jobs assigned to this pool
							sprintf(tempPid, "%d", i);
							strcat(poolcwd, tempPid);
							strcat(poolcwd1, tempPid);

							//cout << "Pid of pool : " << poolNode[i].id << " Current jobs of pool : " << poolNode[i].currentJobs << endl;
							if (poolNode[i].currentJobs < maxNumOfJobs)
							{
								cout << "We still have space in this pool. " << "Logical job id : " << jobCounter << endl; 
								++poolNode[i].currentJobs;
								poolNode[i].activeJobsOfPool = poolNode[i].currentJobs;

								//Open pool pipes for reading and writing
								coordfdsame = open(poolcwd1, O_RDONLY | O_NONBLOCK);
								if (coordfdsame < 0)
								{
									cerr << "Coord can't open input named pipe 2 in same, coordfd." << endl;
								}

								coordfdrsame = open(poolcwd, O_WRONLY | O_NONBLOCK);
								if (coordfdrsame < 0)
								{
									cerr << "Coord can't open output named pipe 1 in same, coordfdr." << endl;
								}

								sprintf(tempLogicNumber, "%d", jobCounter);

								//write to 1
								write(coordfdrsame, jobtoken, MAXBUFF);
								write(coordfdrsame, tempLogicNumber, MAXBUFF);

								//cout << "Parent is sending SIGUSR1 signal - which should be caught" << endl;
								kill(poolNode[i].id, SIGUSR1);

								//read from 2
								while (1)
								{
									coordrcsame = read(coordfdsame, coordbuffsame, MAXBUFF);
									if (coordrcsame > 0)
									{
										//cout << "Read returned from pool in same  : " << coordbuffsame << endl;
										break;
									}
									else
									{
										//cout << "No data returned in same." << endl;
									}
								}

								char* submitjpid = new char[50]();
								char* submitToConsole = new char[150]();
								int fdr;

								sprintf(submitjpid, "%d", jobCounter);
								strcpy(submitToConsole, "JobID : ");
								strcat(submitToConsole, submitjpid);
								strcat(submitToConsole, ", PID : ");
								strcat(submitToConsole, coordbuffsame);
								fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
								if (fdr < 0)
								{
									cerr << "Coord can't open output named pipe." << endl;
								}
								write(fdr, submitToConsole, MAXBUFF);
								close(fdr);

								close(coordfdsame);
								close(coordfdrsame);
							}
							else if (poolNode[i].currentJobs == maxNumOfJobs)
							{
								--poolNode[i].activeJobsOfPool;
								poolNode[i].finished = 1;

								//make a new one
								cout << "Going to make a new pool. " << "Logical job id : " << jobCounter << endl; 
								++poolCounterCopy;
								// if (oldpoolNode =  (PoolNode*) realloc(poolNode, poolSize+1))
								// {
								// 	poolNode = oldpoolNode;
								// }
								// else
							 	// {
							 	//       //handle out-of-memory
							 	//       cerr << "Out of memory exception. Going to exit..." << endl;
							 	//       exit(4);
							 	// }
								// cout << "realloc done" << endl;

								int poolrc;
								char poolcwd[256];
								char poolcwd1[256];
								char poolbuff[MAXBUFF];
								char resultbuff[MAXBUFF];		
								strcpy(poolcwd, cwdp);
								strcpy(poolcwd1, cwdp1);
								sprintf(tempPid, "%d", poolCounter);
								strcat(poolcwd, tempPid);
								strcat(poolcwd1, tempPid);

								PoolHandler(poolCounter, jobCounter, poolcwd, poolcwd1, jobtoken, theDir, cwd1, poolNode, infoNode, jobNode);

								jobNode[jobCounter-2].state = -1;
								jobNode[jobCounter-1].state = -1;
								--poolNode[i].activeJobsOfPool;
								if (poolNode[i+1].currentJobs != 0)
								{
									--poolNode[i+1].activeJobsOfPool;
								}
							}
							else
							{
								--poolNode[poolCounter].activeJobsOfPool;
								poolcwd[strlen(poolcwd)-1] = '\0';
								poolcwd1[strlen(poolcwd1)-1] = '\0';
								continue;
							}
						}

						poolCounter = poolCounterCopy;
						infoNode->numberOfPools = poolCounter;
					}
					else
					{
						exit(1);
					}
				}	
			}
			else if (strcmp(token, "status") == 0)
			{
				char* numberToCompare = new char[300]();
				char* activeTime = new char[300]();
				char* submitToConsoleStat = new char[300]();
				int theStatus;
				int activeFlag = 0;

				//cout << "In status" << endl;
				strcpy(numberToCompare, token);
				numberToCompare = strtok(NULL, " ");
				
				cout << "Job to get the status : " << numberToCompare << endl;
				// cout << "State of job : " << jobNode[atoi(numberToCompare)-1].state << endl;

				int theKIll = kill(atoi(jobNode[atoi(numberToCompare) - 1].theSID), 0);

				if (theKIll == 0)
				{
					//cout << "The process is active/running." << endl;
					if (suspendFlag == 0)
					{
						sprintf(activeTime, "%f", jobNode[atoi(numberToCompare) - 1].theJobTime);
						strcpy(submitToConsoleStat, activeTime); 
						strcat(submitToConsoleStat, ", Status : Active, JobID : ");
						activeFlag = 1;
						//jobNode[atoi(numberToCompare) - 1].state = 1;
					}
					else
					{	
						strcpy(submitToConsoleStat, "Status : Suspended, JobID : ");
						//jobNode[atoi(numberToCompare) - 1].state = 2;
					}
					
					strcat(submitToConsoleStat, numberToCompare);

					fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
					if (fdr < 0)
					{
						cerr << "Coord can't open output named pipe [suspend]." << endl;
					}
					write(fdr, submitToConsoleStat, MAXBUFF);
					close(fdr);
				}
				else if ((theKIll == -1) && (errno == ESRCH))
				{
					//cout << "Process is finished." << endl;
					//jobNode[atoi(numberToCompare) - 1].state = -1;
					// jobNode[atoi(numberToCompare) - 1].state = -1;
					strcpy(submitToConsoleStat, "Status : Finished, JobID : ");
					strcat(submitToConsoleStat, numberToCompare);

					fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
					if (fdr < 0)
					{
						cerr << "Coord can't open output named pipe [suspend]." << endl;
					}
					write(fdr, submitToConsoleStat, MAXBUFF);
					close(fdr);
				}
				else
				{
					cerr << "Error while sending the kill signal in status function." << endl;
					exit(3);
				}
			}
			else if (strcmp(token, "status-all") == 0)
			{
				//cout << "In status-all" << endl;
				char* numberToCompare = new char[300]();
				char* thelogical = new char[50]();
				char* activeTime = new char[300]();
				char* submitToConsoleStatA = new char[300]();
				int theStatus;
				int activeFlag = 0;

				strcpy(numberToCompare, token);
				numberToCompare = strtok(NULL, " ");
				
				cout << "Least time to end all jobs : " << numberToCompare << endl;

				strcpy(submitToConsoleStatA, "JobID ");

				for (int i = 0; i < infoNode->jobCounter; ++i)
				{
					//cout << "State of job : " << jobNode[i].state << endl;
					sprintf(thelogical, "%d", i);
					strcat(submitToConsoleStatA, thelogical);
					strcat(submitToConsoleStatA, " Status: ");

					if ((jobNode[i].state == 1) || (jobNode[i].state == 0))
					{
						if (atoi(numberToCompare) != 0)
						{
							if (jobNode[i].theJobTime < (atoi(numberToCompare)))
							{
								strcat(submitToConsoleStatA, "Active running for : ");
								sprintf(activeTime, "%f", jobNode[i].theJobTime);
								strcat(submitToConsoleStatA, activeTime);
							}
						}
						else
						{
							strcat(submitToConsoleStatA, "Active running for : ");
							sprintf(activeTime, "%f", jobNode[i].theJobTime);
							strcat(submitToConsoleStatA, activeTime);
						}
					}
					else if (jobNode[i].state == 2)
					{
						strcat(submitToConsoleStatA, "Suspended.  ");
					}
					else
					{
						strcat(submitToConsoleStatA, "Finished.  ");
					}
				}

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleStatA, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "show-active") == 0)
			{
				//cout << "In show-active" << endl;
				char* submitToConsoleStatAc = new char[300]();
				char* numberToCompare = new char[300]();
				int theStatus;
				int activeFlag = 0;

				strcpy(submitToConsoleStatAc, "Active jobs : ");

				for (int i = 0; i < infoNode->jobCounter; ++i)
				{
					//cout << "Job's number : " << i << " state : " << jobNode[i].state << endl;
					if ((jobNode[i].state == 1) || (jobNode[i].state == 0))
					{
						activeFlag = 1;
						strcat(submitToConsoleStatAc, " JobID ");
						sprintf(numberToCompare, "%d", i+1);
						strcat(submitToConsoleStatAc, numberToCompare);
					}	
				}

				if (activeFlag == 0)
				{
					strcat(submitToConsoleStatAc, " None.");
				}

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleStatAc, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "show-pools") == 0)
			{
				//cout << "In show-pools" << endl;
				char* numberToCompare = new char[300]();
				char* jobactive = new char[300]();
				char* activeTime = new char[300]();
				char* submitToConsoleSPool = new char[300]();
				int theStatus;
				int activeFlag = 0;

				strcpy(submitToConsoleSPool, "Pool & NumOfJobs ");

				for (int i = 0; i < infoNode->numberOfPools; ++i)
				{
					//cout << "PID of pool " << i << " : " << poolNode[i].id << endl;
					sprintf(numberToCompare, "%d", poolNode[i].id);
					strcat(submitToConsoleSPool, numberToCompare);
					strcat(submitToConsoleSPool, " - ");
					sprintf(jobactive, "%d", poolNode[i].activeJobsOfPool);
					strcat(submitToConsoleSPool, jobactive);
					strcat(submitToConsoleSPool, " , ");
				}

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleSPool, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "show-finished") == 0)
			{
				//cout << "In show-finished" << endl;
				char* numberToCompare = new char[300]();
				char* activeTime = new char[300]();
				char* submitToConsoleFin = new char[300]();
				int theStatus;
				int activeFlag = 0;

				strcpy(submitToConsoleFin, "Finished jobs : ");

				for (int i = 0; i < infoNode->jobCounter; ++i)
				{
					//cout << "Job's number in finished : " << i << " state : " << jobNode[i].state << endl;
					if (jobNode[i].state == -1)
					{
						activeFlag = 1;
						strcat(submitToConsoleFin, " JobID ");
						sprintf(numberToCompare, "%d", i+1);
						strcat(submitToConsoleFin, numberToCompare);
					}
				}

				if (activeFlag == 0)
				{
					strcat(submitToConsoleFin, " None.");
				}

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleFin, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "suspend") == 0)
			{
				char* numberToCompare = new char[300]();
				char* submitToConsoleSus = new char[300]();

				//cout << "In suspend" << endl;
				suspendFlag = 1;
				token = strtok(NULL, " ");
				strcpy(numberToCompare, token);
				cout << "Job to be suspended : " << numberToCompare << endl;
				//cout << "Logical id of job : " << jobNode[atoi(numberToCompare) - 1].logicID << endl;
				//cout << "PID of job : " <<  jobNode[atoi(numberToCompare) - 1].theSID << endl;

				// //send the suspend signal
				kill(atoi(jobNode[atoi(numberToCompare) - 1].theSID), SIGSTOP);

				cout << "Suspend Signal sent." << endl;

				strcpy(submitToConsoleSus, "Sent suspend signal to JobID ");
				strcat(submitToConsoleSus, numberToCompare);

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleSus, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "resume") == 0)
			{
				char* numberToCompare = new char[300]();
				char* submitToConsoleRes = new char[300]();

				suspendFlag = 0;
				//cout << "In resume" << endl;
				token = strtok(NULL, " ");
				strcpy(numberToCompare, token);
				cout << "Job to be resumed : " << numberToCompare << endl;
				//cout << "Logical id of job : " << jobNode[atoi(numberToCompare) - 1].logicID << endl;
				//cout << "PID of job : " <<  jobNode[atoi(numberToCompare) - 1].theSID << endl;

				//send the resume signal
				kill(atoi(jobNode[atoi(numberToCompare) - 1].theSID), SIGCONT);

				cout << "Resume Signal sent." << endl;

				strcpy(submitToConsoleRes, "Sent resume signal to JobID ");
				strcat(submitToConsoleRes, numberToCompare);

				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleRes, MAXBUFF);
				close(fdr);
			}
			else if (strcmp(token, "shutdown") == 0)
			{
				//cout << "In shutdown" << endl;
				char poolcwd[256];
				char poolcwd1[256];
				char submitToConsoleShut[300];		
				strcpy(poolcwd, cwdp);
				strcpy(poolcwd1, cwdp1);
				char* tempSPid = new char[300]();
				char* allJobs = new char[300]();
				char* allAcJobs = new char[300]();
				int currentlyRunningJobs;
				//cout << "Parent is going to kill all pools : " << endl;

				for (int i = 0; i < infoNode->numberOfPools; ++i)
				{
					sprintf(tempSPid, "%d", i);
					strcat(poolcwd, tempSPid);
					strcat(poolcwd1, tempSPid);

					if (poolNode[i].currentJobs > maxNumOfJobs)
					{
						//cout << "Going to kiil already killed pool...Continue" << endl;
						poolcwd[strlen(poolcwd)-1] = '\0';
						poolcwd1[strlen(poolcwd1)-1] = '\0';
						currentlyRunningJobs += poolNode[i].activeJobsOfPool;
						if (poolNode[i].activeJobsOfPool != 0)
						{
							for (int j = 0; j < poolNode[i].activeJobsOfPool; ++j)
							{
								kill(atoi(jobNode[j].theSID), SIGTERM);
							}
						}
						continue;
					}
					else
					{
						kill(poolNode[i].id, SIGTERM);

						if (poolNode[i].activeJobsOfPool != 0)
						{
							for (int k = 0; k < poolNode[i].activeJobsOfPool; ++k)
							{
								kill(atoi(jobNode[k].theSID), SIGTERM);
							}
						}

						if (unlink(poolcwd) < 0) 
						{
							perror("PoolHandler can't unlink input pool named pipe.");
						}

						if (unlink(poolcwd1) < 0) 
						{
							perror("PoolHandler can't unlink output pool named pipe.");
						}

						poolcwd[strlen(poolcwd)-1] = '\0';
						poolcwd1[strlen(poolcwd1)-1] = '\0';
						currentlyRunningJobs += poolNode[i].activeJobsOfPool;
						cout << "Killed pool : " << i << endl;
					}
				}

				sprintf(allJobs, "%d", infoNode->jobCounter);
				sprintf(allAcJobs, "%d", currentlyRunningJobs);	

				strcpy(submitToConsoleShut, "Served ");
				strcat(submitToConsoleShut, allJobs);
				strcat(submitToConsoleShut, " jobs, ");
				strcat(submitToConsoleShut, allAcJobs);
				strcat(submitToConsoleShut, " were still in porgress.");
				fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
				if (fdr < 0)
				{
					cerr << "Coord can't open output named pipe [suspend]." << endl;
				}
				write(fdr, submitToConsoleShut, MAXBUFF);
				close(fdr);
				close(fd);

			   	if (unlink(cwd) < 0) 
			   	{
			   		cerr << "Coord can't unlink input named pipe." << endl;
			   	}

			   	if (unlink(cwd1) < 0) 
			   	{
			   		cerr << "Coord can't unlink output named pipe." << endl;
			   	}

			   	cout << "Named pipes removed." << endl;

			    return 0;
			}
			else
			{
			}

			bytes_in = 1;
		}
		else
		{
		}

		if ((rc == 0) && (bytes_in == 1))
		{
			cout << "No more data. Going to break" << endl;
			break;
		}

		if ((rc == -1) && (errno == EINTR))
		{
			cout << "Read interrupted by signal" << endl;
		}		

		if ((rc == -1) && (errno == EAGAIN))
		{
			//cout << "Pipe opened for writing" << endl;
		}
        
	}
}





void PoolHandler(int poolCounter, int jobCounter, char* poolcwd, char* poolcwd1, char* token, char* outDirectory, char* cwd1, PoolNode* poolNode, Info* infoNode, JobNode* jobNode)
{
	pid_t pool_pid;	
	sigset_t sigset;
	struct sigaction sact;
	time_t t;
	int poolreadfd, poolwritefd;
	int fd, rc, fdr, poolfd, poolfdr, coordfd, coordfdr;
	int coordrc;
	char coordbuff[MAXBUFF];
	int poolrc, poolrc_new;
	char poolbuff[MAXBUFF];
	char maCounter[100];
	char* tempLogicNumber = new char[100]();
	char* jpidc = new char[50]();
	char samplepoolbuff[MAXBUFF];
	int theloop = 0;

	if ((pool_pid = fork()) == 0) 
	{
		int dump = 0;

		/* Creation of pool named pipes */
		poolreadfd = mkfifo(poolcwd, PERMS);
		if ((poolreadfd < 0) && (errno != EEXIST))
		{
			cerr << "Error while creating the input named pipe (pool) fifo file." << endl;
			unlink(poolcwd);
			exit(10);
		}

		poolwritefd = mkfifo(poolcwd1, PERMS);
		if ((poolwritefd < 0) && (errno != EEXIST))
		{
			cerr << "Error while creating the output named pipe (pool) fifo file." << endl;
			unlink(poolcwd1);
			exit(10);
		}

		poolfd = open(poolcwd, O_RDONLY | O_NONBLOCK);
		if (poolfd < 0)
		{
			cerr << "Cant open input pool named pipe 1." << endl;
		}

		//cout << "Named pipes in pool created successfully!" << endl;
		//cout << "PCwd : " << poolcwd << endl;
		//cout << "PCwd1 : " << poolcwd1 << endl;

		//Wait for signal (shutdown or number of assigned jobs equals max)
		sigfillset(&sigset);
		sigdelset(&sigset, SIGUSR1);
		sigdelset(&sigset, SIGTERM);

		kill(getppid(), SIGUSR2);

		while (1)
		{
			time(&t);
			//printf("Child is waiting for parent to send SIGUSR1 at %s", ctime(&t));
			sigsuspend(&sigset);
			time(&t);
			//printf("Sigsuspend is over at %s", ctime(&t));
			pid_t job_pid, waitjob_pid, pidforDir;
			int jstatus = 0;
			int theCounter;

			poolrc = read(poolfd, poolbuff, MAXBUFF);
			if (poolrc > 0)
			{
				//cout << "Read : " << poolbuff << endl;
				strcpy(samplepoolbuff, poolbuff);
			}
			else
			{
				cout << "No data returned." << endl;
			}

			poolrc_new = read(poolfd, maCounter, MAXBUFF);
			if (poolrc_new > 0)
			{
				//cout << "Read : " << maCounter << endl;
			}
			else
			{
				cout << "No data returned 2." << endl;
			}

			if (theloop == 1)
			{
				theloop = 5;
				jobNode[atoi(maCounter) - 1].state = -1;
				//--poolNode[poolCounter].activeJobsOfPool;
			}

			clock_t tic = clock();

			//Make a child to handle this job
			if ((job_pid = fork()) == 0) 
			{
				poolfdr = open(poolcwd1, O_WRONLY | O_NONBLOCK);
				if (poolfdr < 0)
				{
					cerr << "Job can't open output named pipe 2, poolfdr." << endl;
				}

				char* account_name = new char[15]();
				char* myDate = new char[20]();
				char* myTime = new char[20]();
				char* myYear = new char[10]();
				char* myMonth = new char[4]();
				char* myDay = new char[4]();
				char* myHour = new char[4]();
				char* myMin = new char[4]();
				char* mySec = new char[4]();
				char* myUnder = new char[2]();
				char* dirname = new char[300]();
				char* myjobPid = new char[50]();
				char jobOutFile[100];
				char jobErrFile[100];
				char outResFile[300];
				char outErrFile[300];

				strcpy(account_name, "sdi1300025");
				strcpy(jobOutFile, "stdout_");
				strcpy(jobErrFile, "stderr_");
				strcpy(dirname, account_name);
				strcpy(myUnder, "_");
				strcat(dirname, myUnder);
				pidforDir = getpid();

				sprintf(jpidc, "%d", getpid());

				//write to 2
				write(poolfdr, jpidc, MAXBUFF);
				close(poolfdr);

				sleep(2);

				strcat(jobOutFile, maCounter);
				strcat(jobErrFile, maCounter);
				strcat(dirname, maCounter);
				strcat(dirname, myUnder);
				sprintf(myjobPid, "%d", pidforDir);
				//cout << "pid of job :  " << myjobPid << endl;
				strcat(dirname, myjobPid);
				strcat(dirname, myUnder);

				time_t theTime = time(NULL);
				struct tm tm = *localtime(&theTime);

				sprintf(myYear, "%d", tm.tm_year + 1900);
				sprintf(myMonth, "%d", tm.tm_mon + 1);
				sprintf(myDay, "%d", tm.tm_mday);
				sprintf(myHour, "%d", tm.tm_hour);
				sprintf(myMin, "%d", tm.tm_min);
				sprintf(mySec, "%d", tm.tm_sec);
				
				if (strlen(myYear) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, myYear);
					strcpy(myDate, theZero);
				}
				else
				{
					strcpy(myDate, myYear);
				}

				if (strlen(myMonth) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, myMonth);
					strcat(myDate, theZero);
				}
				else
				{
					strcat(myDate, myMonth);
				}

				if (strlen(myDay) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, myDay);
					strcat(myDate, theZero);
				}
				else
				{
					strcat(myDate, myDay);
				}

				//************************************************************************

				if (strlen(myHour) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, myHour);
					strcpy(myTime, theZero);
				}
				else
				{
					strcpy(myTime, myHour);
				}

				if (strlen(myMin) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, myMin);
					strcat(myTime, theZero);
				}
				else
				{
					strcat(myTime, myMin);
				}

				if (strlen(mySec) == 1)
				{
					char* theZero = new char[2]();
					strcpy(theZero, "0");
					strcat(theZero, mySec);
					strcat(myTime, theZero);
				}
				else
				{
					strcat(myTime, mySec);
				}

				strcat(dirname, myDate);
				strcat(dirname, myUnder);
				strcat(dirname, myTime);

				strcat(outDirectory, dirname);

				//create dir
				struct stat st = {0};
				if (stat(outDirectory, &st) == -1)
				{
					//Going to create the directory
			    	int res = mkdir(outDirectory, 0700);
			    	if(res == 0)
			    	{
			    		cout << "Directory " << outDirectory << " created successfully! " << res << endl;
			    	}
	    	    	else
	    	    	{
	    	    		cout << "Error while creating the job's " << maCounter <<  " directory." << endl;
	    	    		exit(6);
	    	    	}
	    		}
	    		else
	    		{
	    			cout << "The job's directory already exists" << endl;
	    		}

				//create files with results
				strcat(outDirectory, "/");
	    		strcpy(outResFile, outDirectory);
	    		strcpy(outErrFile, outDirectory);

	    		strcat(outResFile, jobOutFile);
	    		strcat(outErrFile, jobErrFile);

				samplepoolbuff[strlen(samplepoolbuff) - 1] = '\0';
				char *args[64];
				char **next = args;
				char *temp = strtok(samplepoolbuff, " ");
				while (temp != NULL)
				{
				    *next++ = temp;
				    temp = strtok(NULL, " \n");
				}
				*next = NULL;


				int fdjout = open(outResFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
				int fdjerr = open(outErrFile, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);

			    dup2(fdjout, 1);   //stdout go to out file
			    dup2(fdjerr, 2);   //stderr to another file

			    close(fdjout);
			    close(fdjerr);


			    if (execvp(args[0], args) == -1)
			    {
			    	cerr << "execvp failure @@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@ : " << endl;
			    }

				exit(0);	
			}
			else
			{
			}

			while ((waitjob_pid = wait(&jstatus)) > 0);
			theloop = 1;
			clock_t toc = clock();
			double jobTime =  (double)(toc - tic) / CLOCKS_PER_SEC;
			jobNode[atoi(maCounter)-1].theJobTime = jobTime;
		}

		close(poolfd);
		exit(1);
	}
	else
	{
		sigset_t mysigset;
		struct sigaction mysact;
		time_t t;
		sigemptyset(&mysact.sa_mask);
		mysact.sa_flags = 0;
		mysact.sa_handler = catcher;

		if (sigaction(SIGUSR2, &mysact, NULL) != 0)
			perror("2nd sigaction() error");

		sigfillset(&mysigset);
		sigdelset(&mysigset, SIGUSR2);
		sigsuspend(&mysigset);

		//Open pool pipes for reading and writing
		coordfd = open(poolcwd1, O_RDONLY | O_NONBLOCK);
		if (coordfd < 0)
		{
			cerr << "Coord can't open input named pipe 2, coordfd." << endl;
		}

		coordfdr = open(poolcwd, O_WRONLY | O_NONBLOCK);
		if (coordfdr < 0)
		{
			cerr << "Coord can't open output named pipe 1, coordfdr." << endl;
		}

		sprintf(tempLogicNumber, "%d", jobCounter);

		//write to 1
		write(coordfdr, token, MAXBUFF);
		write(coordfdr, tempLogicNumber, MAXBUFF);

		//Send a signal to start suspended child 
		//cout << "Parent is sending SIGUSR1 signal - which should be caught" << endl;
		kill(pool_pid, SIGUSR1);

		while (1)
		{
			coordrc = read(coordfd, coordbuff, MAXBUFF);
			if (coordrc > 0)
			{
				//cout << "Read returned from pool  : " << coordbuff << endl;
				jobNode[jobCounter-1].state = 1;
				break;
			}
			else
			{
				//cout << "No data returned." << endl;
			}
		}

		//close pools
		close(coordfd);
		close(coordfdr);

		poolNode[poolCounter].id = pool_pid;
		poolNode[poolCounter].currentJobs = 1;
		poolNode[poolCounter].activeJobsOfPool = poolNode[poolCounter].currentJobs;
		poolNode[poolCounter].finished = 0;

		jobNode[jobCounter-1].logicID = jobCounter;
		jobNode[jobCounter-1].theSID = coordbuff;

		char* submitjpid = new char[50]();
		char* submitToConsole = new char[150]();
		int fdr;

		sprintf(submitjpid, "%d", jobCounter);
		strcpy(submitToConsole, "JobID : ");
		strcat(submitToConsole, submitjpid);
		strcat(submitToConsole, ", PID : ");
		strcat(submitToConsole, coordbuff);

		fdr = open(cwd1, O_WRONLY | O_NONBLOCK);
		if (fdr < 0)
		{
			cerr << "Coord can't open output named pipe." << endl;
		}
		write(fdr, submitToConsole, MAXBUFF);
		close(fdr);
	}
}