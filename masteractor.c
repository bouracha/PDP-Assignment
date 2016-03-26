#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"
#include "squirrelactor.h"
#include "cellactor.h"
#include "clockactor.h"

#define tag_squirrelstatus 4
#define tag_babyposition 5

void InitialiseSquirrel(float, float, long*, int);

void squirrel_master(long * seed)
{
  //MPI vairables
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Status status;

  int num_healthy=30;
  int num_infected=4;
  int max_pop=45;
  /*
  printf ("\nParallel Squirrel Simulator\n");
  printf ("##################################################\n");
  printf ("Please enter initial number of HEALTHY Squirrels: ");
  scanf ("%d", &num_healthy);
  printf ("Please enter initial number of INFECTED Squirrels: ");
  scanf ("%i",&num_infected);
  printf ("Please enter a MAX population: ");
  scanf ("%i",&max_pop);
  printf ("##################################################\n");*/

    // Start 17 actors, one for each of the 16 cells
    // and one clock actor
    for (int i = 0; i < 17; i++)
    {
      startWorkerProcess();
    }

    int healthysquirrels = 0;
    for (int i = 0; i < num_healthy; i++)
    {
      int healthy = 1;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, seed);
      InitialiseSquirrel(x_new, y_new, seed, healthy);
      healthysquirrels++;
    }

    int diseasedsquirrels = 0;
    for (int i = 0; i < num_infected; i++)
    {
      int healthy = 0;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, seed);
      InitialiseSquirrel(x_new, y_new, seed, healthy);
      diseasedsquirrels++;
    }

  int flag;
  int loop;
  int terminate = 0;
  int squirrel_status;

  int Num_squirrels = healthysquirrels+diseasedsquirrels;

  int workerStatus = 1;
  while (workerStatus)
  {
      while (1)
      {
        // squirrel_status resets to anything but -1, 0 or 1
        // as these values have meaning in the code.
        squirrel_status = 10;
        flag = 0;
        loop = 1;
        // Masteractor waits in this while loop until it receives a message that a squirrel is changing
        // its status or until shouldWorkerStop returns that shutdownPool() has been issued on another actor
        do
        {
          MPI_Iprobe (MPI_ANY_SOURCE, tag_squirrelstatus, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
          if (flag)
          {
            // Receive a message when a squirrel is born, dies or gets infected
            MPI_Recv(&squirrel_status, 1, MPI_INT, MPI_ANY_SOURCE, tag_squirrelstatus, MPI_COMM_WORLD, &status);
            loop = 0;
          }

          if (shouldWorkerStop())
          {
            loop = 0;
            terminate = 1;
          }
        } while (loop);

        // If a squirrel_status died
        if (squirrel_status == -1)
        {
          diseasedsquirrels--;
          //printf("Squirrel has DIED on process %d leaving %d squirrels\n", status.MPI_SOURCE, (healthysquirrels+diseasedsquirrels));
        }
        // If new squirrel_status was born start new worker process
        if (squirrel_status == 1)
        {
          float position [2];
          MPI_Recv(&position[0], 2, MPI_FLOAT, status.MPI_SOURCE, tag_babyposition, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          float x = position[0];
          float y = position[1];
          int healthy = 1;
          InitialiseSquirrel(x, y, seed, healthy);
          healthysquirrels++;
          //printf("\nSquirrel been BORN from process %d now there's %d squirrels\n", status.MPI_SOURCE, (healthysquirrels+diseasedsquirrels));
        }
        // If a squirrel_status was catch disease
        if (squirrel_status == 0)
        {
          healthysquirrels--;
          diseasedsquirrels++;
        }



        Num_squirrels = healthysquirrels + diseasedsquirrels;
        if (Num_squirrels <= 0 || Num_squirrels >= max_pop)
        {
          if(Num_squirrels == 0) printf("\n\nALL SQUIRRELS HAVE DIED FROM INFECTION!\n\n");
          if(Num_squirrels >= max_pop) printf("\n\nTOO MANY SQUIRRELS FOR THE ENVIRONMENT.\n\n");
          if(terminate == 1) printf("\n\nSIMULATION TIME COMPLETE\n\n");

          shutdownPool();
          terminate = 1;
        }

        if (terminate) break;
      }

    workerStatus=workerSleep();
  }
}



// This initialises a squirrel_status at every call
void InitialiseSquirrel(float  x, float  y, long * seed, int healthy)
{
  // Three non-blocking sends
  MPI_Request request1;
  MPI_Request request2;
  MPI_Request request3;

  int workerPid = startWorkerProcess();

  //printf("Worker with workerid %i has made a squirrel_status at position: %f, %f\n", workerPid, x, y);

  // Master sends information on new squirrel to squirrel worker code
  float position [2] = {x, y};

  MPI_Issend(&position[0], 2, MPI_FLOAT, workerPid, 0 , MPI_COMM_WORLD, &request1);
  MPI_Issend(seed, 1, MPI_LONG, workerPid, 0 , MPI_COMM_WORLD, &request2);
  MPI_Issend(&healthy, 1, MPI_INT, workerPid, 0 , MPI_COMM_WORLD, &request3);
}
