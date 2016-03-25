#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"
#include "squirrelactor.h"
#include "cellactor.h"
#include "clockactor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void InitialiseSquirrel(float, float, long*, int);

void squirrel_master(long * seed)
{
  int size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

      int flag;
      int max_pop = 20;
 int terminate = 0;
  MPI_Request request1;

      MPI_Status status;
      int squirrel_health;

    int healthysquirrels = 0;
    for (int i = 0; i < 5; i++)
    {
      int healthy = 1;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, seed);
      InitialiseSquirrel(x_new, y_new, seed, healthy);
      healthysquirrels++;
    }

    int diseasedsquirrels = 0;
    for (int i = 0; i < 0; i++)
    {
      int healthy = 0;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, seed);
      InitialiseSquirrel(x_new, y_new, seed, healthy);
      diseasedsquirrels++;
    }

int workerStatus = 1;
while (workerStatus)
{
    while (1) 
    {
      // Receive a message when a squirrel is born or dies
      MPI_Recv(&squirrel_health, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
      // If a squirrel_health died
      if (squirrel_health == -1) 
      {
        diseasedsquirrels--;
        printf("Squirrel has DIED on process %d leaving %d squirrels\n", status.MPI_SOURCE, (healthysquirrels+diseasedsquirrels));
      }
      // If new squirrel_health was born start new worker process
      if (squirrel_health == 1)
      {
        float position [2];
        MPI_Recv(&position[0], 2, MPI_FLOAT, status.MPI_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        float x = position[0];
        float y = position[1];
        int healthy = 1;
        InitialiseSquirrel(x, y, seed, healthy);
        healthysquirrels++;
        printf("\nSquirrel been BORN from process %d now there's %d squirrels\n", status.MPI_SOURCE, (healthysquirrels+diseasedsquirrels));
      }
      if (squirrel_health == 0)
      {
        healthysquirrels--;
        diseasedsquirrels++;
      }
      
      //MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
      //if (flag) MPI_Recv(&terminate, 1, MPI_INT, 17, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      int shouldTerminate = 0;
      shouldTerminate = shouldWorkerStop();
printf("shouldTerminate  = %i", shouldTerminate );
      int Num_squirrels = 0;
      Num_squirrels = healthysquirrels + diseasedsquirrels;
      if (Num_squirrels <= 0 || Num_squirrels >= max_pop || shouldTerminate == 1) 
      {
        if(Num_squirrels == 0) printf("\n\nAll Squirrels have died, exiting program.\n\n");
        if(Num_squirrels >= max_pop) printf("\n\nToo many squirrels for environment.\n\n");
        if(shouldTerminate == 1) printf("\n\nSimulation time complete.\n\n");
        int poisonedpill = -3;
        //MPI_Bcast(&poisonedpill, 1, MPI_INT, 0, MPI_COMM_WORLD);
        for (int i = 2; i < (19+Num_squirrels); i++)
        {
          if (i != 18)MPI_Ssend(&poisonedpill, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
          printf("rank: %i has been terminated\n", i);
        }

        shouldTerminate = 1;
      }

      if (shouldTerminate) break;
    }

  printf("rank: %i has been terminated\n", rank);
  workerStatus=workerSleep();
}
}



// This initialises a squirrel_health at every call
void InitialiseSquirrel(float  x, float  y, long * seed, int healthy)
{

  int workerPid = startWorkerProcess();

  printf("Worker with workerid %i has made a squirrel_health at position: %f, %f\n", workerPid, x, y);

  /* Master sends information on new squirrel_health to squirrel_health worker code */
  float position [2] = {x, y};

  MPI_Ssend(&position[0], 2, MPI_FLOAT, workerPid, 0 , MPI_COMM_WORLD);
  MPI_Ssend(seed, 1, MPI_LONG, workerPid, 0 , MPI_COMM_WORLD);
  MPI_Ssend(&healthy, 1, MPI_INT, workerPid, 0 , MPI_COMM_WORLD);
}
