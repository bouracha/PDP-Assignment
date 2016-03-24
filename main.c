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


int main(int argc, char * argv[]) 
{
  //Initialise MPI
  MPI_Init(&argc, &argv);

  int size, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &size);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  //Initialise Random Seed, different on each process
  long seed = -1-rank;
  initialiseRNG(&seed);
 

  //printf("Process %i calling random seed = %i \n", rank, seed);

  /*
  * All ranks except 0 block here until recieve from master
  */
  int statusCode = processPoolInit();
  if (statusCode == 1) 
  {
    if (rank == 1)   squirrel_master(&seed);
    if (rank < 18 && rank > 1) {cellcode();}
    if (rank == 18) clockActor();
    if (rank > 18) squirrelcode();
  }
  else if (statusCode == 2) 
  {
    /*
    * This is the master.
    */

    for (int i = 0; i < 18; i++)
    {
      // Starts 16 actors, one for each cell
      startWorkerProcess();
    }

  int masterStatus = 1;
  while (masterStatus)
  {
    masterStatus = masterPoll();
  }

  }

  //printf("Not Stalling\n");
  processPoolFinalise();
  MPI_Finalize();
  return 0;

}










