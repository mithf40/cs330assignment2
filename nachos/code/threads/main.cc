// main.cc 
//	Bootstrap code to initialize the operating system kernel.
//
//	Allows direct calls into internal operating system functions,
//	to simplify debugging and testing.  In practice, the
//	bootstrap code would just initialize data structures,
//	and start a user program to print the login prompt.
//
// 	Most of this file is not needed until later assignments.
//
// Usage: nachos -d <debugflags> -rs <random seed #>
//		-s -x <nachos file> -c <consoleIn> <consoleOut>
//		-f -cp <unix file> <nachos file>
//		-p <nachos file> -r <nachos file> -l -D -t
//              -n <network reliability> -m <machine id>
//              -o <other machine id>
//              -z
//
//    -d causes certain debugging messages to be printed (cf. utility.h)
//    -rs causes Yield to occur at random (but repeatable) spots
//    -z prints the copyright message
//
//  USER_PROGRAM
//    -s causes user programs to be executed in single-step mode
//    -x runs a user program
//    -c tests the console
//
//  FILESYS
//    -f causes the physical disk to be formatted
//    -cp copies a file from UNIX to Nachos
//    -p prints a Nachos file to stdout
//    -r removes a Nachos file from the file system
//    -l lists the contents of the Nachos directory
//    -D prints the contents of the entire file system 
//    -t tests the performance of the Nachos file system
//
//  NETWORK
//    -n sets the network reliability
//    -m sets this machine's host id (needed for the network)
//    -o runs a simple test of the Nachos network software
//
//  NOTE -- flags are ignored until the relevant assignment.
//  Some of the flags are interpreted here; some in system.cc.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#define MAIN
#include "copyright.h"
#undef MAIN

#include "utility.h"
#include "system.h"
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

// External functions used by this file

extern void ThreadTest(void), Copy(char *unixFile, char *nachosFile);
extern void Print(char *file), PerformanceTest(void);
extern void LaunchUserProcess(char *file), ConsoleTest(char *in, char *out);
extern void MailTest(int networkID);
extern void ForkStartFunction(int dummy);
//----------------------------------------------------------------------
// main
// 	Bootstrap the operating system kernel.  
//	
//	Check command line arguments
//	Initialize data structures
//	(optionally) Call test procedure
//
//	"argc" is the number of command line arguments (including the name
//		of the command) -- ex: "nachos -d +" -> argc = 3 
//	"argv" is an array of strings, one for each command line argument
//		ex: "nachos -d +" -> argv = {"nachos", "-d", "+"}
//----------------------------------------------------------------------
 
int
main(int argc, char **argv)
{
    int argCount;			// the number of arguments 
					// for a particular command

    DEBUG('t', "Entering main");
    (void) Initialize(argc, argv);
    
#ifdef THREADS
    ThreadTest();
#endif

    for (argc--, argv++; argc > 0; argc -= argCount, argv += argCount) {
		argCount = 1;
	        if (!strcmp(*argv, "-z"))               // print copyright
	            printf (copyright);
	#ifdef USER_PROGRAM
	    if (!strcmp(*argv, "-x")) {        	// run a user program
	    	ASSERT(argc > 1);
	        LaunchUserProcess(*(argv + 1));
	        argCount = 2;
	    } 
	    else if (!strcmp(*argv, "-c")) {      // test the console
		    if (argc == 1)
		        ConsoleTest(NULL, NULL);
		    else {
				ASSERT(argc > 2);
		        ConsoleTest(*(argv + 1), *(argv + 2));
		        argCount = 3;
	    	}
	    	interrupt->Halt();		// once we start the console, then 
					// Nachos will loop forever waiting 
					// for console input
		}
		else if(!strcmp(*argv, "-F"))
		{
			ASSERT(argc > 1);
			string line;
			ifstream file;
			file.open(*(argv + 1));
			int schedulingAlgorithm;	//which scheduling algorithm to use
			getline(file, line);
			//schedulingAlgorithm = stoi(line);
			stringstream str(line);
			str >> schedulingAlgorithm;
			// cout << schedulingAlgorithm << endl;
			
			while(getline(file,line))
			{
				char *exec, *temp_prior;
				exec = new char[100];
				temp_prior = new char[100];
				int priority_value, i = 0, j = 0, flag = 0,k = 0;
				while(i < line.length())
				{
					if(line[i] != ' ' && !flag)
					{
						exec[j] = line[i];
						j++;
						i++;
					}
					else if(line[i] == ' ' && !flag)
					{
						flag = 1;
						i++;

					}
					if(line[i] != ' ' && flag)
					{
						temp_prior[k] = line[i];
						i++;
						k++;
					}
				}
				if(temp_prior[0] != '\0')
					priority_value = atoi(temp_prior);
				else
					priority_value = 100;
				//cout<<exec<<" "<<priority_value<<endl;


				OpenFile *executable = fileSystem->Open(exec);
				ProcessAddressSpace *space;
				//printf("%d\n",j);

				//check for valid file
				if(executable == NULL){
					printf("Unable to open file %s\n",exec);
					continue;
				}

				space = new ProcessAddressSpace(executable);
				delete executable;		//close file

				space->InitUserModeCPURegisters();

				NachOSThread *new_thread = new NachOSThread(exec);
				priorityValue[new_thread->GetPID()] = priority_value;
				new_thread->space = space;
				new_thread->SaveUserState();
				new_thread->CreateThreadStack(ForkStartFunction, 0);
				new_thread->Schedule();

				printf("Thread created and scheduled: %s\n",exec);
			}
			printf("[pid %d]:Exit called at totalTick = %d. Code: 0\n",currentThread->GetPID(), stats->totalTicks);

			exitThreadArray[currentThread->GetPID()] = true;

			unsigned int i;
			for (i=0;i<thread_index;i++){
				if(!exitThreadArray[i])
					break;
			}

			currentThread->Exit(i==thread_index,0);
		}
	#endif // USER_PROGRAM
	#ifdef FILESYS
		if (!strcmp(*argv, "-cp")) { 		// copy from UNIX to Nachos
		    ASSERT(argc > 2);
		    Copy(*(argv + 1), *(argv + 2));
		    argCount = 3;
		} else if (!strcmp(*argv, "-p")) {	// print a Nachos file
		    ASSERT(argc > 1);
		    Print(*(argv + 1));
		    argCount = 2;
		} else if (!strcmp(*argv, "-r")) {	// remove Nachos file
		    ASSERT(argc > 1);
		    fileSystem->Remove(*(argv + 1));
		    argCount = 2;
		} else if (!strcmp(*argv, "-l")) {	// list Nachos directory
	            fileSystem->List();
		} else if (!strcmp(*argv, "-D")) {	// print entire filesystem
	            fileSystem->Print();
		} else if (!strcmp(*argv, "-t")) {	// performance test
	            PerformanceTest();
		}
	#endif // FILESYS
	#ifdef NETWORK
	        if (!strcmp(*argv, "-o")) {
		    ASSERT(argc > 1);
	            Delay(2); 				// delay for 2 seconds
							// to give the user time to 
							// start up another nachos
	            MailTest(atoi(*(argv + 1)));
	            argCount = 2;
	        }
	#endif // NETWORK
    }

    currentThread->FinishThread();	// NOTE: if the procedure "main" 
				// returns, then the program "nachos"
				// will exit (as any other normal program
				// would).  But there may be other
				// threads on the ready list.  We switch
				// to those threads by saying that the
				// "main" thread is finished, preventing
				// it from returning.
    return(0);			// Not reached...
}
