#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"


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


    while (1)
    {
      squirrelStep(x, y, &x_new, &y_new, &seed);
      x = x_new;
      y = y_new;

      // WE MAKE CELLNUMBER CORRESPOND TO MPI RANK 1-16 INCLUSIVE
      int cellnumber = getCellFromPosition(x, y);
      cellnumber += 1;

      // Tell cell that squirrel has stepped into it and healthy or not
      MPI_Issend(&healthy, 1, MPI_INT, cellnumber, 0, MPI_COMM_WORLD, &request1);

      // Check
      int terminate = 0;
      MPI_Recv(&terminate, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      if (terminate == -3) break;
      MPI_Recv(&infection_level[stepnumber], 1, MPI_INT, cellnumber, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Recv(&population_in_flux[stepnumber], 1, MPI_INT, cellnumber, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    
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
        MPI_Ssend(&baby_squirrel, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD); 
        float position [2] = {x, y};
        MPI_Ssend(&position[0], 2, MPI_FLOAT, parentId, 0 , MPI_COMM_WORLD);
      }

      // Healthy = 0 means squirrel has disease
      if (willCatchDisease(avg_inf_level, &seed)) 
      {
        healthy = 0;
        MPI_Ssend(&healthy, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD); 
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
          MPI_Ssend(&die_message, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD); 
        
          break;
        }
      }

      stepnumber++;
      stepnumber = stepnumber%50;
    }
  printf("Worker is going to sleep\n");
  workerStatus=workerSleep(); // Will sleep until a new task or shutdown
  printf("Worker has woken up again\n");
} 
}
