/*=========================================================================

  Program:   Visualization Toolkit
  Module:    AMRDataTransferPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Description
//  A simple utility to demonstrate & test the parallel AMR functionality
//  and inter-block data transfer.
//
// .SECTION Note
// This utility code is currently deprecated.

#include <cmath>
#include <sstream>
#include <cassert>
#include <mpi.h>

#include "vtkAMRBaseReader.h"
#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAMRBox.h"
#include "vtkAMRConnectivityFilter.h"
//#include "vtkAMRDataTransferFilter.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkMultiProcessController.h"
#include "vtkMPIController.h"
#include "vtkAMRUtilities.h"
#include "vtkAMRDualMeshExtractor.h"

// Global Variables
vtkMultiProcessController *Controller;

// Function Prototypes
void WriteAMRData( vtkHierarchicalBoxDataSet *amrData, std::string prefix );
vtkHierarchicalBoxDataSet* GetSerialAMRDataSet();
vtkHierarchicalBoxDataSet* GetParallelAMRDataSet();
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim );
void ComputeGaussianPulseError( vtkHierarchicalBoxDataSet *amrData );
double ComputePulseAt( vtkUniformGrid *grid, const int cellIdx );

//
// Main
//
int main( int argc, char **argv )
{
//  Controller = vtkMPIController::New();
//  Controller->Initialize( &argc, &argv );
//  assert("pre: Controller != NULL" && (Controller != NULL ) );
//
//  // STEP 0: Read In AMR data
//  std::cout << "Constructing Sample AMR data!" << std::endl;
//  std::cout.flush( );
//
//  vtkHierarchicalBoxDataSet *amrData = NULL;
//
//  if( Controller->GetNumberOfProcesses( ) == 3 )
//    amrData = GetParallelAMRDataSet( );
//  else if( Controller->GetNumberOfProcesses() == 1 )
//    amrData = GetSerialAMRDataSet( );
//  else
//    {
//      std::cerr << "Can only run with 1 or 3 MPI processes!\n";
//      std::cerr.flush();
//      return( -1 );
//    }
//
//  assert("pre: AMRData != NULL" && (amrData != NULL) );
//  assert("pre: numLevels == 2" && (amrData->GetNumberOfLevels()==2) );
//
//  std::cout << "Done reading!" << std::endl;
//  std::cout.flush( );
//  Controller->Barrier( );
//
//  // STEP 1: Compute inter-block and inter-process connectivity
//  std::cout << "Computing inter-block & inter-process connectivity!\n";
//  std::cout.flush( );
//
//  vtkAMRConnectivityFilter* connectivityFilter=vtkAMRConnectivityFilter::New( );
//  connectivityFilter->SetController( Controller );
//  connectivityFilter->SetAMRDataSet( amrData );
//  connectivityFilter->ComputeConnectivity();
//
//  std::cout << "Done computing connectivity!\n";
//  std::cout.flush( );
//  Controller->Barrier();
//
//  connectivityFilter->Print( std::cout );
//  Controller->Barrier();
//
//  // STEP 2: Data transfer
//  std::cout << "Transfering solution\n";
//  std::cout.flush();
//
//  vtkAMRDataTransferFilter* transferFilter = vtkAMRDataTransferFilter::New();
//
//  transferFilter->SetController( Controller );
//  transferFilter->SetAMRDataSet( amrData );
//  transferFilter->SetNumberOfGhostLayers( 1 );
//  transferFilter->SetRemoteConnectivity(
//   connectivityFilter->GetRemoteConnectivity() );
//  transferFilter->SetLocalConnectivity(
//   connectivityFilter->GetLocalConnectivity() );
//  transferFilter->Transfer();
//
//  vtkHierarchicalBoxDataSet *newData = transferFilter->GetExtrudedData();
//  assert( "extruded data is NULL!" && (newData != NULL) );
//  ComputeGaussianPulseError( newData );
//  WriteAMRData( newData, "NEWDATA" );
//
//  std::cout << "[DONE]\n";
//  std::cout.flush();
//  Controller->Barrier();
//
//  vtkAMRDualMeshExtractor *dme = vtkAMRDualMeshExtractor::New();
//  dme->SetInput( newData );
//  dme->Update();
//
//  std::cout << "Writting dual...";
//  std::cout.flush();
//  dme->WriteMultiBlockData( dme->GetOutput(), "FINALDUAL" );
//  std::cout << "[DONE]\n";
//  std::cout.flush();
//
//  // STEP 3: CleanUp
//  dme->Delete();
//  amrData->Delete();
//  connectivityFilter->Delete();
//  transferFilter->Delete();
//
//  Controller->Finalize();
//  Controller->Delete();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

//------------------------------------------------------------------------------
void WriteAMRData( vtkHierarchicalBoxDataSet *amrData, std::string prefix )
{
  assert( "pre: AMR Data is NULL!" && (amrData != NULL) );

  vtkXMLHierarchicalBoxDataWriter *myAMRWriter=
    vtkXMLHierarchicalBoxDataWriter::New();

  std::ostringstream oss;
  oss << prefix << "." << myAMRWriter->GetDefaultFileExtension();

  myAMRWriter->SetFileName( oss.str().c_str() );
  myAMRWriter->SetInput( amrData );
  myAMRWriter->Write();
  myAMRWriter->Delete();
}

//------------------------------------------------------------------------------
double ComputePulseAt( vtkUniformGrid *grid, const int cellIdx )
{
  // Sanity check
  assert( "pre: grid != NULL" && (grid != NULL) );
  assert( "pre: cellIdx in bounds" &&
         ( (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()) ) );

  // Gaussian Pulse parameters
  double x0 = 6.0;
  double y0 = 6.0;
  double lx = 12;
  double ly = 12;
  double a  = 0.1;

  vtkCell* myCell = grid->GetCell( cellIdx);
  assert( "post: myCell != NULL" && (myCell != NULL) );

  vtkPoints *cellPoints = myCell->GetPoints();

  double xyzCenter[3];
  xyzCenter[0] = 0.0;
  xyzCenter[1] = 0.0;
  xyzCenter[2] = 0.0;

  for( int cp=0; cp < cellPoints->GetNumberOfPoints(); ++cp )
    {
       double pnt[3];
       cellPoints->GetPoint( cp, pnt );
       xyzCenter[0] += pnt[0];
       xyzCenter[1] += pnt[1];
       xyzCenter[2] += pnt[2];
    } // END for all cell points

  double x = xyzCenter[0] / (cellPoints->GetNumberOfPoints());
  double y = xyzCenter[1] / (cellPoints->GetNumberOfPoints());
  double z = xyzCenter[2] / (cellPoints->GetNumberOfPoints());
  double r = ( ( (x-x0)*(x-x0) ) / ( lx*lx ) ) +
             ( ( (y-y0)*(y-y0) ) / ( ly*ly ) );
  double f = a*std::exp( -r );

  return( f );
}

//------------------------------------------------------------------------------
void ComputeGaussianPulseError( vtkHierarchicalBoxDataSet *data )
{
  // Sanity check
  assert( "pre: data is NULL!" && (data != NULL) );

  for( unsigned int level=0; level < data->GetNumberOfLevels(); ++level )
    {
      for( unsigned int idx=0; idx < data->GetNumberOfDataSets(level); ++idx)
        {

          vtkUniformGrid *ug = data->GetDataSet(level,idx);
          if( ug != NULL )
            {
              vtkDoubleArray *err = vtkDoubleArray::New();
              err->SetName( "err" );
              err->SetNumberOfComponents( 1 );
              err->SetNumberOfValues( ug->GetNumberOfCells() );

              vtkCellData *CD = ug->GetCellData();
              assert( "pre: Grid must have cell data!" &&
                      (CD->GetNumberOfArrays() > 0) );

              vtkDataArray *pulse = CD->GetArray( "GaussianPulse" );
              assert( "pre: Pulse data not found on grid!" && (pulse!=NULL) );

              for( int cell=0; cell < ug->GetNumberOfCells(); ++cell )
                {
                  double expected = ComputePulseAt( ug, cell );
                  double actual   = pulse->GetComponent( cell, 0 );
                  double abserr   = std::abs( (actual-expected) );
                  err->SetComponent( cell, 0, abserr );
                } // END for all grid cells

              CD->AddArray( err );
              err->Delete();
            }

        } // END for all data
    } // END for all levels
}

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetSerialAMRDataSet( )
{

  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  data->Initialize();

  double origin[3];
  double h[3];
  int    ndim[3];
  int    dim      =  2;
  int    blockId  = -1;
  int    level    = -1;
  int    rank     = 0;

  // BLOCK 0
  ndim[0]               = 25;
  ndim[1]               = 25;
  ndim[2]               = 1;
  origin[0]             = 0.0;
  origin[1]             = 0.0;
  origin[2]             = 0.0;
  blockId               = 0;
  level                 = 0;
  h[0] = h[1] = h[2]    = 0.5;
  vtkUniformGrid *grid0 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid0);
  grid0->Delete();

  // BLOCK 1
  ndim[0]            = 11;
  ndim[1]            = 7;
  ndim[2]            = 1;
  origin[0]          = 1.5;
  origin[1]          = 1.5;
  origin[2]          = 0.0;
  blockId            = 0;
  level              = 1;
  h[0] = h[1] = h[2] = 0.25;
  vtkUniformGrid *grid1 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid1);
  grid1->Delete();

  // BLOCK 2
  ndim[0]               = 11;
  ndim[1]               = 7;
  ndim[2]               = 1;
  origin[0]             = 1.0;
  origin[1]             = 3.0;
  origin[2]             = 0.0;
  blockId               = 1;
  level                 = 1;
  h[0] = h[1] = h[2]    = 0.25;
  vtkUniformGrid *grid2 = GetGrid( origin,h,ndim );
  data->SetDataSet( level,blockId,grid2);
  grid2->Delete();

  vtkAMRUtilities::GenerateMetaData( data, NULL );
  data->GenerateVisibilityArrays();
  return( data );
}

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetParallelAMRDataSet( )
{
  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New( );
  data->Initialize();

  // TODO: imlpement this

  return( data );

}

//----------------------------------------------------------------------------
vtkUniformGrid* GetGrid( double *origin, double *spacing, int *ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( spacing );
  grd->SetDimensions( ndim );

  vtkDoubleArray* xyz = vtkDoubleArray::New( );
  xyz->SetName( "GaussianPulse" );
  xyz->SetNumberOfComponents( 1 );
  xyz->SetNumberOfTuples( grd->GetNumberOfCells() );
  for( int cellIdx=0; cellIdx < grd->GetNumberOfCells(); ++cellIdx )
    {
      xyz->SetTuple1(cellIdx, ComputePulseAt(grd,cellIdx) );
    } // END for all cells

  grd->GetCellData()->AddArray(xyz);
  xyz->Delete();
  return grd;
}
