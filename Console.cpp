#include "Headers.hpp"

#define MAXBUFF 1024
#define PERMS   0666

int main(int argc, char const *argv[])
{
    cout << "Initializing firmware........Firmware functional." << endl;
    cout << "System check............Passed." << endl;
    cout << "Initiating plain language interface...Done." << endl;
    cout << "Loading library session...Done." << endl;
    cout << "Loading \'jms_console\' executable program....Done." << endl;
    cout << "Ready." << endl << endl << endl;

    ifstream operationsFile;
    ifstream maout;
    ofstream mafifo;
    char* dirname = new char[300]();
    char* utilexec = new char[300]();
    char* jmsIn = new char[300]();
    char* jmsOut = new char[300]();
    char* firstSlash = new char[3]();
    char* theSlash = new char[3]();
    char cwd[256];
    char cwd1[256];
    char buff[MAXBUFF];
    int inputFileUsed = 0;
    int readfd, writefd;
    int fd, fdr, rc;
    int ofd;
    int wnum;
    int bytes_in;
    strcpy(theSlash, "/");
    strcpy(firstSlash, "/");

    if (argc > 4)
	{
		for (int i = 1; i < argc; i++)
		{
			if (strcmp(argv[i], "-w") == 0)			//named pipe for writing
			{
				strcpy(jmsIn, argv[i+1]);
				cout << "Named pipe in : " << jmsIn << endl;
				++i;
			}
			else if (strcmp(argv[i], "-r") == 0)	//named pipe for record reading
			{
				strcpy(jmsOut, argv[i+1]);
				cout << "Named pipe out : " << jmsOut << endl;
				++i;
			}
			else if (strcmp(argv[i], "-o") == 0)
			{
				strcpy(dirname, argv[i+1]);
				operationsFile.open(argv[i+1]);   	//input file is next on argv
				if (operationsFile == NULL)
				{
					cout << "You've given a wrong input file." << endl;
					exit(1);
				}
				else
				{
					cout << "File : " << argv[i+1] << " opened successfully!" << endl;
    				inputFileUsed = 1;
				}
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
		cout << "------>  Suggested input: $./jms_console –w <jms_in> -r <jms_out> -o <operations_file>" << endl;
		exit(1);
	}

    cout << "*********** Welcome to JMS Console! ***********" << endl;

    //srand(time(NULL));
	std::cout.setf(std::ios_base::fixed, std::ios_base::floatfield);
	std::cout.precision(2);

	//Getting current directory
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
	strcat(cwd, jmsIn);
	strcat(cwd1, jmsOut);

	fd = open(cwd, O_WRONLY | O_NONBLOCK);
	if (fd < 0)
	{
		cerr << "Console can't open named pipe." << endl;
		exit(11);
	}

	fdr = open(cwd1, O_RDONLY | O_NONBLOCK);
	if (fdr < 0)
	{
		cerr << "Coord can't open input named pipe." << endl;
	}

	cout << "Path 1 : " << cwd << endl;
	cout << "Path 2 : " << cwd1 << endl;
	cout << "Named pipes (Console) opened successfully!" << endl;

    while (operationsFile.getline(utilexec, MAXBUFF))
    {
    	//cout << "Util to be executed : " << utilexec << endl;
    	write(fd, utilexec, MAXBUFF);
		//cout << "Wrote successfully to fifo file" << endl;
		sleep(2);
		while (1)
		{
			rc = read(fdr, buff, MAXBUFF);
			if (rc > 0)
			{
				cout << buff << endl;
				break;
			}
		}
    }


    while (cin.getline(utilexec, MAXBUFF))
    {
    	//cout << "Util to be executed : " << utilexec << endl;
    	write(fd, utilexec, MAXBUFF);
		//cout << "Wrote successfully to fifo file" << endl;
		sleep(2);
		while (1)
		{
			rc = read(fdr, buff, MAXBUFF);
			if (rc > 0)
			{
				cout << buff << endl;
				break;
			}
		}
    }


    close(fd);
    close(fdr);

    cout << "Named pipes closed successfully!" << endl;

    return 0;   	
}
