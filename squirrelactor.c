#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"


void squirrelcode() {
  //int workerStatus = 1;
  int parentId;
  float position[2];
  long seed;
  int healthy;

  parentId = getCommandData();

  //Initialise MPI variables
  int rank;     
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  /* Receive Squirrel position and random seed */
  MPI_Recv(&position[0], 2, MPI_FLOAT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&seed, 1, MPI_LONG, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  MPI_Recv(&healthy, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

  float x = position[0];
  float y = position[1];

  float x_new, y_new;

  int stepnumber = 0;
  int death_clock = 0;

  float infection_level [50] = {0};
  float population_in_flux [50] = {0};

  int workerStatus = 1;
  while (workerStatus)
  {
    squirrelStep(x, y, &x_new, &y_new, &seed);
    x = x_new;
    y = y_new;

    // WE MAKE CELLNUMBER CORRESPOND TO MPI RANK 1-16 INCLUSIVE
    int cellnumber = getCellFromPosition(x, y);
    cellnumber += 1;

    // Tell cell that squirrel has stepped into it and healthy or not
    MPI_Ssend(&healthy, 1, MPI_INT, cellnumber, 0, MPI_COMM_WORLD);


    MPI_Recv(&infection_level[stepnumber], 1, MPI_FLOAT, cellnumber, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("Received 1!\n");
    MPI_Recv(&population_in_flux[stepnumber], 1, MPI_FLOAT, cellnumber, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    //printf("Received 2!\n");
    //printf("Infection level: %f population in flux %f \n", infection_level[stepnumber], population_in_flux[stepnumber]);
    
    // Calculate average infection level and average populationinflux for last 50 steps.
    float avg_inf_level = 0;
    float avg_pop = 0;
    for (int i = 0; i < 50; i++)
    {
      avg_inf_level +=  infection_level [i];
      avg_pop += population_in_flux [i];
    }
    avg_inf_level /= 50;
    avg_pop /= 50;

    if(willGiveBirth(avg_pop, &seed) && stepnumber == 49)
    {
      printf("Will give birth \n");
      int baby_squirrel = 1;
      MPI_Ssend(&baby_squirrel, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD); 
      float position [2] = {x, y};
      MPI_Ssend(&position[0], 2, MPI_FLOAT, parentId, 0 , MPI_COMM_WORLD);
      printf("Squirrel gave BIRTH at position  %lf, %lf after %i steps\n", x, y, stepnumber);
    }

    // Healthy = 0 means squirrel has disease
    if (willCatchDisease(avg_inf_level, &seed)) healthy = 0;
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
        printf("Squirrel died at position  %lf, %lf after %i infected steps\n", x, y, death_clock%50);
        // Tell master process squirrel has died 
        int die_message = - 1;
        MPI_Ssend(&die_message, 1, MPI_INT, parentId, 0, MPI_COMM_WORLD); 
        
        workerStatus=workerSleep();
        //break;
      }
    }

    stepnumber++;
    stepnumber = stepnumber%50;
  }

    //workerStatus=workerSleep(); // Will sleep until a new task or shutdown
}
