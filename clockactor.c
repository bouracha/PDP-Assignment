#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"

#define tag_healthy 1

void clockActor()
{
  //Initialise MPI variables
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Request request1;
  MPI_Request request2;
  MPI_Status status;
  int months = 24;
  int count = 1;
  int terminate = 0;
  int month_changed = 2;

  int tag_message = 1;

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
      MPI_Issend(&month_changed, 1, MPI_INT, i, tag_healthy, MPI_COMM_WORLD, &request1);
    }


    count++;

    if (shouldWorkerStop())
    {
      break;
    }
  }


  shutdownPool();
  workerStatus=workerSleep();
  workerStatus = 0;
}
