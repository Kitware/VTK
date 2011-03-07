/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAMRUtilities.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <iostream>
#include <sstream>
#include <cassert>
#include <mpi.h>

#include "vtkAMRUtilities.h"
#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkUniformGrid.h"
#include "vtkImageToStructuredGrid.h"
#include "vtkStructuredGridWriter.h"

//-----------------------------------------------------------------------------
//           H E L P E R   M E T H O D S  &   M A C R O S
//-----------------------------------------------------------------------------
#define CHECK_TEST( P, testName, rval ) {                                 \
  if( !P ) {                                                              \
    std::cerr << "ERROR:" << testName << " FAILED!\n";                    \
    std::cerr << "Location:" << __FILE__ << ":" << __LINE__ << std::endl; \
    ++rval;                                                               \
  }                                                                       \
}

// Description:
// Gets the grid for the given process
void WriteUniformGrid( vtkUniformGrid *myGrid, std::string prefix )
{
  assert( "Input Grid is not NULL" && (myGrid != NULL) );

  vtkImageToStructuredGrid* myImage2StructuredGridFilter =
      vtkImageToStructuredGrid::New();
  assert( "Cannot create Image2StructuredGridFilter" &&
          (myImage2StructuredGridFilter != NULL) );

  myImage2StructuredGridFilter->SetInput( myGrid );
  myImage2StructuredGridFilter->Update();
  vtkStructuredGrid* myStructuredGrid =
      myImage2StructuredGridFilter->GetOutput();
  assert( "Structured Grid output is NULL!" &&
          (myStructuredGrid != NULL) );

  vtkStructuredGridWriter *myWriter = vtkStructuredGridWriter::New();
  myWriter->SetFileName( prefix.c_str() );
  myWriter->SetInput( myStructuredGrid );
  myWriter->Update();
  myWriter->Delete();
  myImage2StructuredGridFilter->Delete();
}

// Description:
// Gets the grid for the given process
void GetGrid( vtkUniformGrid *myGrid, int &level, int &index,
              vtkMultiProcessController *myController )
{
  assert( "Input Grid is not NULL" && (myGrid != NULL) );
  assert( "Null Multi-process controller encountered" &&
          (myController != NULL) );

  switch( myController->GetLocalProcessId() )
    {
      case 0:
        {
          level=index=0;
          double myOrigin[3] = {0.0,0.0,0.0};
          int ndim[3] = {4,4,1};
          double spacing[3] = {1,1,1};
          myGrid->Initialize();
          myGrid->SetOrigin( myOrigin );
          myGrid->SetSpacing( spacing );
          myGrid->SetDimensions( ndim );
        }
        break;
      case 1:
        {
          level=1;index=0;
          double myOrigin[3] = {1.0,1.0,0.0};
          int ndim[3] = {5,3,1};
          double spacing[3] = {0.5,0.5,0.5};
          myGrid->Initialize();
          myGrid->SetOrigin( myOrigin );
          myGrid->SetSpacing( spacing );
          myGrid->SetDimensions( ndim );
        }
        break;
      default:
        std::cerr << "Undefined process!\n";
        std::cerr.flush();
        myGrid->Delete();
        myGrid=NULL;
    }

}

// Description:
// Get the AMR data-structure for the given process
void GetAMRDataSet(
    vtkHierarchicalBoxDataSet *amrData,
    vtkMultiProcessController *myController )
{
  assert( "Input AMR Data is NULL!" && (amrData != NULL) );
  assert( "Null Multi-process controller encountered" &&
           (myController != NULL) );

  int level = -1;
  int index = -1;
  vtkUniformGrid* myGrid = vtkUniformGrid::New();
  GetGrid( myGrid, level, index, myController );
  assert( "Invalid level" && (level >= 0) );
  assert( "Invalid index" && (index >= 0) );
  std::ostringstream oss;
  oss.clear();
  oss.str("");
  oss << "Process_" << myController->GetLocalProcessId() << "_GRID_";
  oss << "L" << level << "_" << index << ".vtk";
  WriteUniformGrid( myGrid, oss.str( ) );

  amrData->SetDataSet( level, index, myGrid );
  myGrid->Delete();
}

//-----------------------------------------------------------------------------
//                   T E S T   M E T H O D S
//-----------------------------------------------------------------------------

// Description:
// Tests the functionality for computing the global data-set origin.
bool TestComputeDataSetOrigin( vtkMultiProcessController *myController )
{
  assert( "Null Multi-process controller encountered" &&
          (myController != NULL) );
  myController->Barrier();

  vtkHierarchicalBoxDataSet* myAMRData = vtkHierarchicalBoxDataSet::New();
  GetAMRDataSet( myAMRData, myController );
  double origin[3];
  vtkAMRUtilities::ComputeDataSetOrigin( origin, myAMRData, myController );
  myAMRData->Delete();

  myController->Barrier();
  std::cout << myController->GetLocalProcessId() << ": ";
  std::cout << origin[0] << ", " << origin[1] << ", " << origin[2] << std::endl;
  std::cout.flush();
  myController->Barrier();

  if( (origin[0] == 0.0) && (origin[1] == 0.0) && (origin[2] == 0.0) )
    return true;
  else
    return false;
}

// Description:
// Main Test driver
int TestAMRUtilities(int,char*[])
{
  vtkMultiProcessController *myController =
      vtkMultiProcessController::GetGlobalController();
  assert( "Null Multi-process controller encountered" &&
            (myController != NULL) );

  // Synchronize Processes
  myController->Barrier();

  int rval=0;
  CHECK_TEST( TestComputeDataSetOrigin(myController),"ComputeOrigin", rval );

  // Synchronize Processes
  myController->Barrier();
  return( rval );
}

//-----------------------------------------------------------------------------
//                   P R O G R A M    M A I N
//-----------------------------------------------------------------------------

int main( int argc, char **argv )
{
  MPI_Init(&argc,&argv);

  vtkMPIController* contr = vtkMPIController::New();
  contr->Initialize( &argc, &argv, 1);

  vtkMultiProcessController::SetGlobalController(contr);

  int rc = TestAMRUtilities( argc, argv );
  contr->Barrier();

  contr->Finalize();
  contr->Delete();
  return rc;
}
