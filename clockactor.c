#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"

void clockActor()
{
  MPI_Request request1;
  MPI_Request request2;
  MPI_Status status;
  int months = 24;
  int count = 1;
  int terminate = 0;
  int month_changed = 2;
int shouldbreak = 0;

  int flag = 0;

  int parentId; 
  parentId = getCommandData();

  int workerStatus = 1;	
  while(count <= months)
  {
    //function that delays before moving on. we have set it to 2 seconds
    //this allows the processors to move the squirrels for 2 seconds before resseting
    sleep(1);

    printf("\nSending monthly notice all cells \t Month %i\n\n", count);

    //MPI_Bcast(&month_changed, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 2; i < 18; i++)
    {
      //printf("i %i\n", i);
      MPI_Issend(&month_changed, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request1);
    }

    shouldbreak = shouldWorkerStop();

    count++;
      
    if (shouldbreak)
    {
      printf("clock terminating\n");
      break;
    }
  }
  
  terminate = -3;
  //MPI_Ssend(&terminate, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
  shutdownPool();

  printf("Shutting down pool\n");
  shutdownPool();
  workerStatus=workerSleep();
  workerStatus = 0;
}

