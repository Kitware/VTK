// This example just send a message from one process to another to
// test if every thing compiles OK.

#include "vtkObject.h"
#include "mpi.h"


void main( int argc, char *argv[] )
{
  char a;
  int numprocs, myid;
  int id0 = 0;
  int id1 = 1;


  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myid);

  cerr << "process: " << myid << " of " << numprocs << endl;

  if (myid == id1) 
    {
    MPI_Status status;
    int a, b;
    
    a = 11;
    
    cerr << "Process " << myid << " Sending int " << a << " to " << id0 << endl;

    MPI_Send(&a, 1, MPI_INT, id0, 99, MPI_COMM_WORLD );

    cerr << "Process " << myid << " waiting for message from process " << id0 << endl;
    MPI_Recv(&b, 1, MPI_INT, id0, 99, MPI_COMM_WORLD, &status);

    cerr << "Process " << myid << " Received int " << b << " from process " << id0 << endl;    
    }


  // set up the renderer in process 0
  if (myid == id0) 
    {
    MPI_Status status;
    int a, b;
    
    a = 23;
    
    cerr << "Process " << myid << " waiting for message from process " << id1 << endl;
    MPI_Recv(&b, 1, MPI_INT, id1, 99, MPI_COMM_WORLD, &status);

    cerr << "Process " << myid << " Received int " << b << " from process " << id1 << endl;    

    cerr << "Process " << myid << " Sending int " << a << " to " << id1 << endl;

    MPI_Send(&a, 1, MPI_INT, id1, 99, MPI_COMM_WORLD );

    }

  cerr << myid << " waiting at barrier\n";
  MPI_Barrier (MPI_COMM_WORLD);
  cerr << myid << " past barrier\n";
  MPI_Finalize();
  return(0);
}


