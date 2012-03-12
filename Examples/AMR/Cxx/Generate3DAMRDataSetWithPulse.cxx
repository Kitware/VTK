/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Generate3DAMRDataSetWithPulse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Generate3DAMRDataSetWithPulse.cxx -- Generated sample 3D AMR dataset
//
// .SECTION Description
//  This utility code generates a simple 3D AMR dataset with a gaussian
//  pulse at the center. The resulting AMR dataset is written using the
//  vtkXMLHierarchicalBoxDataSetWriter.

#include <iostream>
#include <cmath>
#include <sstream>
#include <cassert>

#include "vtkUniformGrid.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"
#include "vtkCell.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkAMRBox.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkXMLHierarchicalBoxDataWriter.h"
#include "vtkAMRUtilities.h"

struct PulseAttributes {
  double origin[3]; // xyz for the center of the pulse
  double width[3];  // the width of the pulse
  double amplitude; // the amplitude of the pulse
} Pulse;

//
// Function prototype declarations
//

// Description:
// Sets the pulse attributes
void SetPulse();

// Description:
// Constructs a uniform grid instance given the prescribed
// origin, grid spacing and dimensions.
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim );

// Description:
// Computes the gaussian pulse at the cell center of the cell
// corresponding to the given cellIdx w.r.t the given grid.
double ComputePulseAt( vtkUniformGrid *grid, const int cellIdx );

// Description:
// Computes the cell center for the cell corresponding to cellIdx w.r.t.
// the given grid. The cell center is stored in the supplied buffer c.
void ComputeCellCenter( vtkUniformGrid *grid, const int cellIdx, double c[3] );

// Description:
// Constructs the vtkHierarchicalBoxDataSet.
vtkHierarchicalBoxDataSet* GetAMRDataSet();

// Description:
// Writes the amr data set using the prescribed prefix.
void WriteAMRData( vtkHierarchicalBoxDataSet *amrData, std::string prefix );

//
// Program main
//
int main( int argc, char **argv )
{
  // Fix compiler warning for unused variables
  static_cast<void>(argc);
  static_cast<void>(argv);

  // STEP 0: Initialize gaussian pulse parameters
  SetPulse();

  // STEP 1: Get the AMR dataset
  vtkHierarchicalBoxDataSet *amrDataSet = GetAMRDataSet();
  assert( "pre: NULL AMR dataset" && ( amrDataSet != NULL ) );

  WriteAMRData( amrDataSet, "Gaussian3D" );
  amrDataSet->Delete();
  return 0;
}

//=============================================================================
//                    Function Prototype Implementation
//=============================================================================

void SetPulse()
{
  Pulse.origin[0] = Pulse.origin[1] = Pulse.origin[2] = -1.0;
  Pulse.width[0]  = Pulse.width[1]  = Pulse.width[2]  = 6.0;
  Pulse.amplitude = 0.0001;
}

//------------------------------------------------------------------------------
void WriteAMRData( vtkHierarchicalBoxDataSet *amrData, std::string prefix )
{
  assert( "pre: AMR Data is NULL!" && (amrData != NULL) );

  vtkXMLHierarchicalBoxDataWriter *myAMRWriter=
    vtkXMLHierarchicalBoxDataWriter::New();

  std::ostringstream oss;
  oss << prefix << ".vthb";

  myAMRWriter->SetFileName( oss.str().c_str() );
  myAMRWriter->SetInputData( amrData );
  myAMRWriter->Write();
  myAMRWriter->Delete();
}

//------------------------------------------------------------------------------
vtkHierarchicalBoxDataSet* GetAMRDataSet()
{
  vtkHierarchicalBoxDataSet *data = vtkHierarchicalBoxDataSet::New();
  data->Initialize();

  double origin[3];
  double h[3];
  int    ndim[3];
  int    blockId = -1;
  int    level   = -1;

  // Root Block -- Block 0
  ndim[0]   = 6; ndim[1]   = ndim[2]   = 5;
  h[0]      = h[1]      = h[2]      = 1.0;
  origin[0] = origin[1] = origin[2] = -2.0;
  blockId   = 0;
  level     = 0;
  vtkUniformGrid *root = GetGrid(origin, h, ndim);
  data->SetDataSet( level, blockId,root);
  root->Delete();

  // Block 1
  ndim[0]   = 3; ndim[1]   = ndim[2]   = 5;
  h[0]      = h[1]      = h[2]      = 0.5;
  origin[0] = origin[1] = origin[2] = -2.0;
  blockId   = 0;
  level     = 1;
  vtkUniformGrid *grid1 = GetGrid(origin, h, ndim);
  data->SetDataSet( level, blockId,grid1);
  grid1->Delete();

  // Block 2
  ndim[0]   = 3; ndim[1]   = ndim[2]   = 5;
  h[0]      = h[1]      = h[2]      = 0.5;
  origin[0] = 0.0; origin[1] = origin[2] = -1.0;
  blockId   = 1;
  level     = 1;
  vtkUniformGrid *grid2 = GetGrid(origin, h, ndim);
  data->SetDataSet( level, blockId,grid2);
  grid2->Delete();

  // Block 3
  ndim[0]   = 3; ndim[1]   = ndim[2]   = 7;
  h[0]      = h[1]      = h[2]      = 0.5;
  origin[0] = 2.0; origin[1] = origin[2] = -1.0;
  blockId   = 2;
  level     = 1;
  vtkUniformGrid *grid3 = GetGrid(origin, h, ndim);
  data->SetDataSet( level, blockId,grid3);
  grid3->Delete();

  vtkAMRUtilities::GenerateMetaData( data, NULL );
  data->GenerateVisibilityArrays();
  return( data );
}

//------------------------------------------------------------------------------
void ComputeCellCenter( vtkUniformGrid *grid, const int cellIdx, double c[3] )
{
  assert( "pre: grid != NULL" && (grid != NULL) );
  assert( "pre: Null cell center buffer" && (c != NULL)  );
  assert( "pre: cellIdx in bounds" &&
          (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells() ) );

  vtkCell *myCell = grid->GetCell( cellIdx );
  assert( "post: cell is NULL" && (myCell != NULL) );

  double pCenter[3];
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  int subId       = myCell->GetParametricCenter( pCenter );
  myCell->EvaluateLocation( subId,pCenter,c,weights );
  delete [] weights;
}

//------------------------------------------------------------------------------
double ComputePulseAt( vtkUniformGrid *grid, const int cellIdx )
{
  // Sanity check
  assert( "pre: grid != NULL" && (grid != NULL) );
  assert( "pre: cellIdx in bounds" &&
         ( (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()) ) );

  double xyzCenter[3];
  ComputeCellCenter( grid, cellIdx, xyzCenter );

  double r = 0.0;
  for( int i=0; i < 2; ++i )
    {
      double dx = xyzCenter[i]-Pulse.origin[i];
      r += (dx*dx) / (Pulse.width[i]*Pulse.width[i]);
    }
  double f = Pulse.amplitude*std::exp( -r );
  std::cout << "G(" << xyzCenter[0] << ",";
  std::cout << xyzCenter[1] << ",";
  std::cout << xyzCenter[2] << ") = ";
  std::cout << f  << "\t";
  std::cout << "r=" << r << std::endl;
  std::cout.flush();
  return( f );
}

//------------------------------------------------------------------------------
vtkUniformGrid* GetGrid( double* origin,double* h,int* ndim )
{
  vtkUniformGrid *grd = vtkUniformGrid::New();
  grd->Initialize();
  grd->SetOrigin( origin );
  grd->SetSpacing( h );
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
