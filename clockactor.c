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
  int months = 24;
  int count = 1;
  int terminate = 0;
  int month_changed = 2;

  int flag = 0;

  int parentId; 
  parentId = getCommandData();

  int workerStatus = 1;	
  while(count < months)
  {
    //function that delays before moving on. we have set it to 2 seconds
    //this allows the processors to move the squirrels for 2 seconds before resseting
    sleep(3);

    printf("\nSending monthly notice all cells \t Month %i\n\n", count);
    //MPI_Bcast(&month_changed, 1, MPI_INT, 0, MPI_COMM_WORLD);
    for (int i = 1; i < 17; i++)
    {
      MPI_Issend(&month_changed, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request1);
    }

    MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
    if (flag) MPI_Recv(&terminate, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

    count++;
      
    if (terminate == -3)
    {
      break;
    }
  }
  
  //terminate = -3;
  //MPI_Ssend(&terminate, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  printf("Clock sleeping\n");
  workerStatus=workerSleep();
}
