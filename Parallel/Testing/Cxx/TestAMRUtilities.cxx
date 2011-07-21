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
#include "vtkXMLHierarchicalBoxDataWriter.h"

//-----------------------------------------------------------------------------
//           H E L P E R   M E T H O D S  &   M A C R O S
//-----------------------------------------------------------------------------

#define CHECK_TEST( P, testName, rval ) {                                 \
  if( !P ) {                                                              \
    std::cerr << "ERROR:" << testName << " FAILED!\n";                    \
    std::cerr << "Location:" << __FILE__ << ":" << __LINE__ << std::endl; \
    std::cerr.flush();                                                    \
    ++rval;                                                               \
  }                                                                       \
}

#define CHECK_CONDITION( P, shortMessage, status ) {                      \
  if( !P ) {                                                              \
    std::cerr << "ERROR:" << shortMessage << std::endl;                   \
    std::cerr << "Location: " << __FILE__ << ":" << __LINE__ << std::endl;\
    status = 0;                                                           \
    return status;                                                        \
  }                                                                       \
}

//-----------------------------------------------------------------------------
// Description:
// Write the AMR data in XML
void WriteAMRData(
    vtkHierarchicalBoxDataSet *myAMRData,
    vtkMultiProcessController *controller )
{
  // Sanity check
  assert( "AMR dataset is NULL" && (myAMRData != NULL) );

  vtkXMLHierarchicalBoxDataWriter *myAMRWriter =
      vtkXMLHierarchicalBoxDataWriter::New();

  std::ostringstream oss;
  oss << "AMR_PROCESS_" << controller->GetLocalProcessId() << "."
      << myAMRWriter->GetDefaultFileExtension();

  myAMRWriter->SetFileName( oss.str().c_str() );
  myAMRWriter->SetInput( myAMRData );
  myAMRWriter->Write();
  myAMRWriter->Delete();
  controller->Barrier();
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Description:
// Gets the grid for the given process
int CheckMetaData( vtkHierarchicalBoxDataSet *myAMRData )
{

  int status = 1;
  vtkAMRBox myBox;

  // STEP 0: Check metadata @(0,0)
  if( myAMRData->GetMetaData( 0, 0, myBox ) == 1 )
    {
       CHECK_CONDITION((myBox.GetBlockId()==0),"BlockId mismatch", status );
       CHECK_CONDITION((myBox.GetLevel()==0),"Level mismatch", status);
       CHECK_CONDITION((myBox.GetProcessId()==0),"Process ID mismatch",status);

       int lo[3]; int hi[3];
       myBox.GetLoCorner( lo );
       myBox.GetHiCorner( hi );
       CHECK_CONDITION((lo[0]==0),"LoCorner mismatch",status);
       CHECK_CONDITION((lo[1]==0),"LoCorner mismatch",status);
      /* CHECK_CONDITION((lo[2]==0),"LoCorner mismatch",status); */
       CHECK_CONDITION((hi[0]==2),"HiCorner mismatch",status);
       CHECK_CONDITION((hi[1]==2),"HiCorner mismatch",status);
      /* CHECK_CONDITION((hi[2]==0),"HiCorner mismatch",status); */

       double spacing[3];
       myBox.GetGridSpacing( spacing );
       CHECK_CONDITION((spacing[0]==1.0),"Check grid spacing",status);
       CHECK_CONDITION((spacing[1]==1.0),"Check grid spacing",status);
      /* CHECK_CONDITION((spacing[2]==1.0),"Check grid spacing",status); */
    }
  else
    {
      std::cerr << "Could not retrieve metadata for item @(0,0)!\n";
      std::cerr.flush( );
      status = 0;
    }

  // STEP 1: Check metadata @(1,0)
  if( myAMRData->GetMetaData(1,0,myBox) == 1 )
    {
      CHECK_CONDITION((myBox.GetBlockId()==0),"BlockId mismatch", status );
      CHECK_CONDITION((myBox.GetLevel()==1),"Level mismatch", status);
      CHECK_CONDITION((myBox.GetProcessId()==1),"Process ID mismatch",status);

      int lo[3]; int hi[3];
      myBox.GetLoCorner( lo );
      myBox.GetHiCorner( hi );
      CHECK_CONDITION((lo[0]==2),"LoCorner mismatch",status);
      CHECK_CONDITION((lo[1]==2),"LoCorner mismatch",status);
     /* CHECK_CONDITION((lo[2]==0),"LoCorner mismatch",status); */
      CHECK_CONDITION((hi[0]==5),"HiCorner mismatch",status);
      CHECK_CONDITION((hi[1]==3),"HiCorner mismatch",status);
     /* CHECK_CONDITION((hi[2]==0),"HiCorner mismatch",status); */

      double spacing[3];
      myBox.GetGridSpacing( spacing );
      CHECK_CONDITION((spacing[0]==0.5),"Check grid spacing",status);
      CHECK_CONDITION((spacing[1]==0.5),"Check grid spacing",status);
      /* CHECK_CONDITION((spacing[2]==0.5),"Check grid spacing",status); */
    }
  else
    {
      std::cerr << "Could not retrieve metadata for item @(0,0)!\n";
      std::cerr.flush( );
      status = 0;
    }

  return( status );
}

//-----------------------------------------------------------------------------
// Description:
// Gets the grid for the given process
int CheckProcessData0( vtkHierarchicalBoxDataSet *myAMRData )
{
  // Sanity Check
  assert( "Input AMR dataset is NULL" && (myAMRData != NULL) );

  int status = 1;

  if( myAMRData->GetDataSet(0,0) == NULL )
    {
      std::cerr << "ERROR: Expected data to be non-NULL, but, data is NULL!\n";
      std::cerr.flush();
      status = 0;
    }
  else if( myAMRData->GetDataSet(1,0) != NULL )
    {
      std::cerr << "ERROR: Expected data to be NULL, but, data is NOT NULL!\n";
      std::cerr.flush( );
      status = 0;
    }
  else
    {
      status = CheckMetaData( myAMRData );
    }

  return status;
}

//-----------------------------------------------------------------------------
// Description:
// Gets the grid for the given process
int CheckProcessData1( vtkHierarchicalBoxDataSet *myAMRData )
{
  // Sanity Check
  assert( "Input AMR dataset is NULL" && (myAMRData != NULL) );

  int status = 1;

  if( myAMRData->GetDataSet(0,0) != NULL )
    {
      std::cerr << "ERROR: Expected data to be NULL, but, data is NOT NULL!\n";
      std::cerr.flush();
      status = 0;
    }
  else if( myAMRData->GetDataSet(1,0) == NULL )
    {
      std::cerr << "ERROR: Expected data to be non-NULL, but, data is NULL!\n";
      std::cerr.flush( );
      status = 0;
    }
  else
    {
      status = CheckMetaData( myAMRData );
    }

  return status;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
// Description:
// Tests the functionality for computing the global bounds.
bool TestGenerateMetaData( vtkMultiProcessController *myController )
{
  assert( "Null Multi-process controller encountered!" &&
          (myController != NULL ) );

  vtkHierarchicalBoxDataSet* myAMRData = vtkHierarchicalBoxDataSet::New();
  GetAMRDataSet( myAMRData, myController );
  vtkAMRUtilities::GenerateMetaData( myAMRData, myController );

  int status    = 1;
  int statusSum = 0;
  for( unsigned int i=0; i < myAMRData->GetNumberOfLevels(); ++i )
    {
      if( myAMRData->GetRefinementRatio( i ) != 2 )
        {
          status =0;
          break;
        }
    }

//  std::cout << "Calling write AMR Data...";
//  std::cout.flush();
  WriteAMRData( myAMRData, myController );
//  std::cout << "[DONE]\n";
//  std::cout.flush();
//  myController->Barrier();

  myAMRData->Delete();
  myController->AllReduce( &status, &statusSum, 1, vtkCommunicator::SUM_OP );
  return( ( (statusSum==2)? true : false ) );
}

//-----------------------------------------------------------------------------
// Description:
// Tests the functionality for computing the global bounds.
bool TestComputeGlobalBounds( vtkMultiProcessController *myController )
{
  assert( "Null Multi-process controller encountered!" &&
          (myController != NULL ) );

  vtkHierarchicalBoxDataSet* myAMRData = vtkHierarchicalBoxDataSet::New();
  GetAMRDataSet( myAMRData, myController );
  double bounds[6];
  vtkAMRUtilities::ComputeGlobalBounds( bounds, myAMRData, myController );
  myAMRData->Delete();

  int status    = 0;
  int statusSum = 0;
  if( (bounds[0]== 0.0) && (bounds[1]==0.0) && /*(bounds[2]==0.0) &&*/
      (bounds[3]== 3.0) && (bounds[4]==3.0) /*&& (bounds[5]==0.0)*/ )
    {
      status = 1;
    }
  else
    {
      std::cerr << "ERROR: The bounds are:";
      for( int i=0; i < 6; std::cerr << bounds[i++] << " " );
      std::cerr << std::endl;
      std::cerr.flush();
    }

  myController->AllReduce( &status, &statusSum, 1, vtkCommunicator::SUM_OP );
  return( ( (statusSum==2)? true : false ) );
}

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

  int status    = 0;
  int statusSum = 0;
  if( (origin[0] == 0.0) && (origin[1] == 0.0) && (origin[2] == 0.0) )
    status = 1;

  myController->AllReduce( &status, &statusSum, 1, vtkCommunicator::SUM_OP  );

  return( ( (statusSum==2)? true : false ) );
}

//-----------------------------------------------------------------------------
// Description:
// Tests the functionality for computing the global data-set origin.
bool TestCollectMetaData( vtkMultiProcessController *myController )
{

  assert( "Null Multi-process controller encountered" &&
          (myController != NULL) );
  myController->Barrier();

  vtkHierarchicalBoxDataSet* myAMRData = vtkHierarchicalBoxDataSet::New();
  GetAMRDataSet( myAMRData, myController );

  vtkAMRUtilities::CollectAMRMetaData( myAMRData, myController );

  int status    = 0;
  int statusSum = 0;
  switch( myController->GetLocalProcessId() )
    {
      case 0:
        status = CheckProcessData0( myAMRData );
        break;
      case 1:
        status = CheckProcessData1( myAMRData );
        break;
      default:
        std::cerr << "ERROR: This test must be run with 2 MPI processes!\n";
        std::cerr.flush( );
        status = 0;
    }
  myAMRData->Delete();

  myController->AllReduce( &status, &statusSum, 1, vtkCommunicator::SUM_OP );
  return ( (statusSum==2)? true : false );
}

//-----------------------------------------------------------------------------
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
  myController->Barrier();

  CHECK_TEST( TestCollectMetaData(myController), "CollectMetaData", rval );
  myController->Barrier();

  CHECK_TEST( TestComputeGlobalBounds(myController),"ComputeBounds",rval );
  myController->Barrier();

  CHECK_TEST( TestGenerateMetaData(myController),"GenerateMetaData",rval );
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
