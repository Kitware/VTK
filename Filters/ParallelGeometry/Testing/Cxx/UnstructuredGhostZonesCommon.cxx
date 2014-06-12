// MPI include
#include <mpi.h>

// VTK includes
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkDoubleArray.h"
#include "vtkExtentRCBPartitioner.h"
#include "vtkIdList.h"
#include "vtkIdTypeArray.h"
#include "vtkMPIController.h"
#include "vtkPointData.h"
#include "vtkStructuredData.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridWriter.h"

#include "UnstructuredGhostZonesCommon.h"

double global::Origin[3]  = {0.0,0.0,0.0};
double global::Spacing[3] = {0.5,0.5,0.5};
int global::Dims[3]       = {50,50,50};

int global::Rank   = -1;
int global::NRanks = 0;

vtkUnstructuredGrid* global::Grid;
