// Minimal test for existence of MPI_LONG_LONG

#include "mpi.h"
MPI_Datatype vtkMPICommunicatorGetMPIType()
{
  return MPI_LONG_LONG;
}

int main()
{
  vtkMPICommunicatorGetMPIType();
  return 0;
}
