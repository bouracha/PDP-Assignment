#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"

#define tag_healthy 1
#define tag_infectionlevel 2
#define tag_popinflux 3

#define tag_squirrelstatus 4
#define tag_babyposition 5

void squirrelcode() {

int parentId;
float position[2];
long seed;
int healthy;

parentId = getCommandData();

//Initialise MPI variables
int rank;
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Request request1;
MPI_Request request2;
MPI_Request request3;
MPI_Request request4;
MPI_Request request5;
MPI_Request request6;

int workerStatus = 1;
while (workerStatus)
{

  /* Receive Squirrel position and random seed */
  MPI_Recv(&position[0], 2, MPI_FLOAT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&seed, 1, MPI_LONG, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&healthy, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  float x = position[0];
  float y = position[1];

  float x_new, y_new;

  int stepnumber = 0;
  int death_clock = 0;

  int infection_level [50] = {0};
  int population_in_flux [50] = {0};

  int terminate = 0;


    while (1)
    {
      squirrelStep(x, y, &x_new, &y_new, &seed);
      x = x_new;
      y = y_new;

      // WE MAKE CELLNUMBER CORRESPOND TO MPI RANK 1-16 INCLUSIVE
      int cellnumber = getCellFromPosition(x, y);
      cellnumber += 2;

      // Tell cell that squirrel has stepped into it and healthy or not
      //MPI_Ssend(&healthy, 1, MPI_INT, cellnumber, tag_healthy, MPI_COMM_WORLD);
      MPI_Issend(&healthy, 1, MPI_INT, cellnumber, tag_healthy, MPI_COMM_WORLD, &request1);

      int flag = 0;
      //int flag2 = 0;
      int loop = 1;
      do
      {
        MPI_Iprobe (cellnumber, tag_infectionlevel, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
        //MPI_Iprobe (cellnumber, tag_popinflux, MPI_COMM_WORLD, &flag1, MPI_STATUS_IGNORE);
//printf("flag1 = %i flag2 = %i\n", flag1, flag2);
        if (flag)
        {
          MPI_Recv(&infection_level[stepnumber], 1, MPI_INT, cellnumber, tag_infectionlevel, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          MPI_Recv(&population_in_flux[stepnumber], 1, MPI_INT, cellnumber, tag_popinflux, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
          loop = 0;
        }

        if (shouldWorkerStop())
        {
          loop = 0;
          terminate = 1;
        }
//printf("Stuck in do-while\n");
      } while (loop);

      // Calculate average infection level and average populationinflux for last 50 steps.
      float avg_inf_level = 0.0;
      float avg_pop = 0.0;
      for (int i = 0; i < 50; i++)
      {
        avg_inf_level +=  infection_level [i];
        avg_pop += population_in_flux [i];
      }

      avg_inf_level /= 50;
      avg_pop /= 50;

      if(willGiveBirth(avg_pop, &seed) && stepnumber == 49)
      {
        int baby_squirrel = 1;
        MPI_Issend(&baby_squirrel, 1, MPI_INT, parentId, tag_squirrelstatus, MPI_COMM_WORLD, &request2);
        float position [2] = {x, y};
        MPI_Issend(&position[0], 2, MPI_FLOAT, parentId, tag_babyposition , MPI_COMM_WORLD, &request3);
      }

      // Healthy = 0 means squirrel has disease
      if (willCatchDisease(avg_inf_level, &seed) && healthy == 1)
      {
        healthy = 0;
        MPI_Issend(&healthy, 1, MPI_INT, parentId, tag_squirrelstatus, MPI_COMM_WORLD, &request4);
      }
      if (healthy == 0)
      {
        death_clock ++;
      }

     //printf("Squirrel %i: stepnumber %i avg_pop: %f avg_inf_level %f \n", rank-17, stepnumber, avg_pop, avg_inf_level);

      if (death_clock >= 50)
      {
        //printf("Squirrel is now dying.\n");
        if (willDie(&seed))
        {
          // Tell master process squirrel has died
          int die_message = - 1;
          MPI_Issend(&die_message, 1, MPI_INT, parentId, tag_squirrelstatus, MPI_COMM_WORLD, &request5);

          break;
        }
      }

      stepnumber++;
      stepnumber = stepnumber%50;

      if (terminate) break;
    }
  //printf("Rank %i is going to sleep\n", rank);
  workerStatus=workerSleep(); // Will sleep until a new task or shutdown
}
}
