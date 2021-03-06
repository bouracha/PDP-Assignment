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


#define SERIAL_THRESHOLD 10
#define DATA_LENGTH 100


int main(int argc, char * argv[])
{
  //MPI Variables
  MPI_Init(&argc, &argv);
  int rank;
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);


  //Initialise Random Seed, different on each process
  long seed = -1-rank;
  initialiseRNG(&seed);


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

    // Start a masteractor
    startWorkerProcess();


    int masterStatus = 1;
    while (masterStatus) //exits when shut down pool is called
    {
      masterStatus = masterPoll();
    }

  }

  processPoolFinalise();
  MPI_Finalize();
  return 0;

}









