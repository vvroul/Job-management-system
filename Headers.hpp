#ifndef HEADERS_H
#define HEADERS_H

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <math.h>
#include <cmath>

using namespace std;

extern "C"
{
    #include <semaphore.h>
    #include <dirent.h>
	#include <sys/stat.h>
	#include <poll.h>
	#include <signal.h>
    #include <stdio.h>
    #include <errno.h>
    #include <fcntl.h>
    #include <ctype.h>
    #include <time.h>
    #include <stdlib.h>
    #include <string.h>
    #include <unistd.h>
    #include <sys/wait.h>
    #include <sys/types.h>
    #include <sys/ipc.h>
    #include <sys/shm.h>
    #include <sys/sem.h>
}


#endif //!HEADERS_H
