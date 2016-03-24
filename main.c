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

#define SERIAL_THRESHOLD 10
#define DATA_LENGTH 100

static int InitialiseCell();
static void InitialiseSquirrel(float, float, long*, int);

int main(int argc, char * argv[]) 
{
  //Initialise MPI
  MPI_Init(&argc, &argv);

  int size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Request request1;

  int flag;
  int max_pop = 40;

  //Initialise Random Seed, different on each process
  long seed = -1-rank;
  initialiseRNG(&seed);
  int terminate = 0;

  //printf("Process %i calling random seed = %i \n", rank, seed);

  /*
  * All ranks except 0 block here until recieve from master
  */
  int statusCode = processPoolInit();
  if (statusCode == 1) 
  {
    if (rank < 17 && rank > 0) {cellcode();}
    if (rank == 17) clockActor();
    if (rank > 17) squirrelcode();
  }
  else if (statusCode == 2) 
  {
    /*
    * This is the master.
    */

    for (int i = 0; i < 17; i++)
    {
      // Starts 16 actors, one for each cell
      startWorkerProcess();
    }

    int healthysquirrels = 0;
    for (int i = 0; i < 10; i++)
    {
      int healthy = 1;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, &seed);
      InitialiseSquirrel(x_new, y_new, &seed, healthy);
      healthysquirrels++;
    }

    int diseasedsquirrels = 0;
    for (int i = 0; i < 20; i++)
    {
      int healthy = 0;
      float x, y = 0;
      float x_new, y_new;
      squirrelStep(x, y, &x_new, &y_new, &seed);
      InitialiseSquirrel(x_new, y_new, &seed, healthy);
      diseasedsquirrels++;
    }

    MPI_Status status;
    int squirrel_health;
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
        InitialiseSquirrel(x, y, &seed, healthy);
        healthysquirrels++;
        printf("\nSquirrel been BORN on process %d now there's %d squirrels\n", status.MPI_SOURCE, (healthysquirrels+diseasedsquirrels));
      }
      if (squirrel_health == 0)
      {
        healthysquirrels--;
        diseasedsquirrels++;
      }
      
      //MPI_Iprobe(0, 0, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
      //if (flag) MPI_Recv(&terminate, 1, MPI_INT, 17, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

      int Num_squirrels = 0;
      Num_squirrels = healthysquirrels + diseasedsquirrels;
      if (Num_squirrels <= 0 || Num_squirrels >= max_pop || terminate == -3) 
      {
        if(Num_squirrels == 0) printf("\n\nAll Squirrels have died, exiting program.\n\n");
        if(Num_squirrels >= max_pop) printf("\n\nToo many squirrels for environment.\n\n");
        int poisonedpill = -3;
        //MPI_Bcast(&poisonedpill, 1, MPI_INT, 0, MPI_COMM_WORLD);
        for (int i = 1; i < (18+Num_squirrels); i++)
        {
          MPI_Issend(&poisonedpill, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &request1);
        }
        terminate = -3;
      }

      if (terminate == -3) break;
    }

  }

  //printf("Not Stalling\n");
  processPoolFinalise();
  MPI_Finalize();
  return 0;

}







// This initialises a squirrel_health at every call
void InitialiseSquirrel(float  x, float  y, long * seed, int healthy)
{
  int workerPid = startWorkerProcess();

  //printf("Worker with workerid %i has made a squirrel_health at position: %f, %f\n", workerPid, x, y);

  /* Master sends information on new squirrel_health to squirrel_health worker code */
  float position [2] = {x, y};
  MPI_Ssend(&position[0], 2, MPI_FLOAT, workerPid, 0 , MPI_COMM_WORLD);
  MPI_Ssend(seed, 1, MPI_LONG, workerPid, 0 , MPI_COMM_WORLD);
  MPI_Ssend(&healthy, 1, MPI_INT, workerPid, 0 , MPI_COMM_WORLD);
}



