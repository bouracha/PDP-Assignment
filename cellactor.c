#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include "mpi.h"
#include "pool.h"
#include "ran2.h"
#include "squirrel-functions.h"
#include "squirrelactor.h"

#define tag_healthy 1
#define tag_infectionlevel 2
#define tag_popinflux 3

void cellcode() {

  int parentId;
  parentId = getCommandData();

  float infection_level [2] = {0};
  float population_in_flux [3] = {0};
  int month = 0;
  int infection_month_number = 0;
  int population_month_number = 0;

  int num_infected_squirrels = 0;
  int population = 0;

  int terminate = 0;
  int message = 10;

  //Initialise MPI variables
  int rank;
  MPI_Status status;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Request request1;
  MPI_Request request2;
  MPI_Request request3;


  int workerStatus = 1;
  while (workerStatus)
  {

    int flag = 0;
    int loop = 1;
    do
    {
      MPI_Iprobe (MPI_ANY_SOURCE, tag_healthy, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
      if (flag)
      {
        MPI_Recv(&message, 1, MPI_INT, MPI_ANY_SOURCE, tag_healthy, MPI_COMM_WORLD, &status);
        loop = 0;
      }

      if (shouldWorkerStop())
      {
        loop = 0;
        terminate = 1;
        //message = 2;
      }

    } while (loop);


    if (message == 0 || message == 1)
    {
      if (message == 0)
      {
        infection_level [(infection_month_number)] += 1;
      }
      population_in_flux [(population_month_number)] += 1;

      num_infected_squirrels = 0;
      for (int i = 0; i < 2; i++)
      {
        num_infected_squirrels +=  infection_level [i];
      }

      population = 0;
      for (int i = 0; i < 3; i++)
      {
        population +=  population_in_flux [i];
      }
      population -= 1; //Squirrel doesn't copulate with itself

      //printf("popinflux %f in cell: %i \n",population, rank);
      //printf("infection_level %f, population_in_flux %f\n", infection_level [(infection_month_number)], population_in_flux [0], status.MPI_SOURCE);

      // Send information back to the squirrel.
      MPI_Issend(&num_infected_squirrels, 1, MPI_INT, status.MPI_SOURCE, tag_infectionlevel, MPI_COMM_WORLD, &request2);
      MPI_Issend(&population, 1, MPI_INT, status.MPI_SOURCE, tag_popinflux, MPI_COMM_WORLD, &request3);
    }

    //MPI_Irecv(&month_changed, 1, MPI_INT, 17, 1, MPI_COMM_WORLD, &request);
    if(message == 2)
    {
      month++;
      infection_month_number = month%2;
      population_month_number = month%3;

      infection_level [(infection_month_number)] = 0;
      population_in_flux [(population_month_number)] = 0;

      num_infected_squirrels = 0;
      for (int i = 0; i < 2; i++)
      {
        num_infected_squirrels +=  infection_level [i];
      }

      population = 0;
      for (int i = 0; i < 3; i++)
      {
        population +=  population_in_flux [i];
      }

      printf("Cell %i: Popinflux: %i Infection Num: %i \n", rank, population, num_infected_squirrels);
    }

    if (terminate) break;
  }


  //printf("Cell rank %i is going to sleep\n", rank);
  workerStatus=workerSleep(); // Will sleep until a new task or shutdown
}
