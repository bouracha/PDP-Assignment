#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"
#include "squirrelactor.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

void cellcode() {

  int parentId; 

  parentId = getCommandData();

  float infection_level [2] = {0};
  float population_in_flux [3] = {0};
  int month = 0;
  int infection_month_number = 0;
  int population_month_number = 0;

  float num_infected_squirrels = 0.0;
  float population = 0.0;

  int terminate = 0;
  int healthy;

  //Initialise MPI variables
  int rank;     
  MPI_Status status;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Request request;

  int workerStatus = 1;
  while (workerStatus)
  {

    //Block here until receive message from a squirrel
    MPI_Recv(&healthy, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);

    int month_changed = 0;
    MPI_Irecv(&month_changed, 1, MPI_INT, 17, 1, MPI_COMM_WORLD, &request);
    if(month_changed == 1)
    {
      month++;
      printf("Infection level in cell %i for month %i: %f \n", rank, month, num_infected_squirrels);
      printf("Population influx in cell %i for month %i: %f \n", rank, month, population);
      infection_month_number = month%2;
      population_month_number = month%3;
      infection_level [(infection_month_number)] = 0;
      population_in_flux [(population_month_number)] = 0;
    }

    if (healthy == 0)
    {
      infection_level [(infection_month_number)] += 1;
    }
    population_in_flux [(population_month_number)] += 1;

    num_infected_squirrels = 0.0;
    for (int i = 0; i < 2; i++)
    {
      num_infected_squirrels +=  infection_level [i];
    }

    population = 0.0;
    for (int i = 0; i < 3; i++)
    {
      population +=  population_in_flux [i];
    }
    population -= 1; //Squirrel doesn't copulate with itself

//printf("popinflux %f in cell: %i \n",population, rank);
//printf("infection_level %f, population_in_flux %f\n", infection_level [(infection_month_number)], population_in_flux [0], status.MPI_SOURCE);

    // Send information back to the squirrel.
    MPI_Ssend(&num_infected_squirrels, 1, MPI_FLOAT, status.MPI_SOURCE, 0, MPI_COMM_WORLD); 
    MPI_Ssend(&population, 1, MPI_FLOAT, status.MPI_SOURCE, 0, MPI_COMM_WORLD);



    //MPI_Irecv(&terminate, 1, MPI_INT, parentId, 1, MPI_COMM_WORLD, &request);
    //printf("Received terminate  %i from rank %i \n", terminate, parentId);
    if (terminate) workerStatus=workerSleep();
  }

  MPI_Wait(&request, MPI_STATUS_IGNORE);

      //shouldWorkerStop();
  //workerStatus=workerSleep(); // Will sleep until a new task or shutdown
}

