#ifndef POOLNODE_H
#define POOLNODE_H


struct PoolNode
{
	int currentJobs;
	int activeJobsOfPool;
	pid_t id;
	int finished;
	char* cmd;
	char* argvh[2];	
};

struct Info
{
	int numberOfPools;
	int jobCounter;
};


struct JobNode
{
	int logicID;
	pid_t theJID;
	char* theSID;
	int state;
	double theJobTime;
};

#endif //!POOLNODE_H