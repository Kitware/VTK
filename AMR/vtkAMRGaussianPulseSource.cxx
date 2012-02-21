/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRGaussianPulseSource.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRGaussianPulseSource.h"
#include "vtkObjectFactory.h"
#include "vtkOverlappingAMR.h"
#include "vtkUniformGrid.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkAMRUtilities.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkCell.h"

#include <cassert>

vtkStandardNewMacro(vtkAMRGaussianPulseSource);

//------------------------------------------------------------------------------
vtkAMRGaussianPulseSource::vtkAMRGaussianPulseSource()
{
  this->SetNumberOfInputPorts(0);
  this->SetNumberOfOutputPorts(1);
  this->RootSpacing[0] =
  this->RootSpacing[1] =
  this->RootSpacing[2] = 0.5;

  this->PulseOrigin[0] =
  this->PulseOrigin[1] =
  this->PulseOrigin[2] = 0.0;

  this->PulseWidth[0]  =
  this->PulseWidth[1]  =
  this->PulseWidth[2]  = 0.5;

  this->NumberOfLevels = 1;
  this->Dimension      = 3;
  this->RefinmentRatio = 2;
  this->PulseAmplitude = 0.0001;
}

//------------------------------------------------------------------------------
vtkAMRGaussianPulseSource::~vtkAMRGaussianPulseSource()
{

}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf( os, indent );
}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::ComputeCellCenter(
    vtkUniformGrid *grid, vtkIdType cellIdx, double centroid[3] )
{
  assert("pre: input grid instance is NULL" && (grid != NULL));
  assert("pre: cell index is out-of-bounds!" &&
         (cellIdx >= 0) && (cellIdx < grid->GetNumberOfCells()));

  vtkCell *myCell = grid->GetCell( cellIdx );
  assert( "ERROR: Cell is NULL" && (myCell != NULL) );

  double pcenter[3];
  double *weights = new double[ myCell->GetNumberOfPoints() ];
  int subId = myCell->GetParametricCenter( pcenter );
  myCell->EvaluateLocation( subId, pcenter, centroid, weights );
  delete [] weights;
}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::GeneratePulseField(vtkUniformGrid* grid)
{
  assert("pre: grid is NULL!" && (grid != NULL));
  assert("pre: grid is empty!" && (grid->GetNumberOfCells() >= 1) );

  vtkDoubleArray *centroidArray = vtkDoubleArray::New();
  centroidArray->SetName("Centroid");
  centroidArray->SetNumberOfComponents( 3 );
  centroidArray->SetNumberOfTuples( grid->GetNumberOfCells() );

  vtkDoubleArray *pulseField = vtkDoubleArray::New();
  pulseField->SetName( "Gaussian-Pulse" );
  pulseField->SetNumberOfComponents( 1 );
  pulseField->SetNumberOfTuples( grid->GetNumberOfCells() );

  double centroid[3];
  vtkIdType cellIdx = 0;
  for(; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
    {
    this->ComputeCellCenter(grid,cellIdx,centroid);
    centroidArray->SetComponent(cellIdx,0,centroid[0]);
    centroidArray->SetComponent(cellIdx,1,centroid[1]);
    centroidArray->SetComponent(cellIdx,2,centroid[2]);

    double pulse = this->ComputePulseAt(centroid);
    pulseField->SetComponent(cellIdx,0,pulse);
    } // END for all cells

  grid->GetCellData()->AddArray( centroidArray );
  centroidArray->Delete();
  grid->GetCellData()->AddArray( pulseField );
  pulseField->Delete();
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRGaussianPulseSource::GetGrid(
    double origin[3], double h[3], int ndim[3] )
{
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin( origin );
  grid->SetSpacing( h );
  grid->SetDimensions( ndim );

  this->GeneratePulseField( grid );
  return( grid );
}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::Generate2DDataSet( vtkOverlappingAMR *amr )
{
  assert( "pre: input amr dataset is NULL" && (amr != NULL) );

  int ndim[3];
  double origin[3];
  double h[3];
  int blockId = 0;
  int level   = 0;

  // Root Block -- Block 0,0
  ndim[0] = 6; ndim[1]   = 6; ndim[2] = 1;
  h[0]      = h[1]  = h[2] = this->RootSpacing[0];
  origin[0] = origin[1] = -2.0; origin[2] = 0.0;
  blockId   = 0;
  level     = 0;
  vtkUniformGrid *grid = this->GetGrid(origin, h, ndim);
  amr->SetDataSet(level,blockId,grid);
  grid->Delete();
  vtkAMRUtilities::GenerateMetaData( amr, NULL );
  amr->GenerateVisibilityArrays();
}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::Generate3DDataSet( vtkOverlappingAMR *amr )
{
  assert("pre: input AMR dataset is NULL" && (amr != NULL) );

  int ndim[3];
  double origin[3];
  double h[3];
  int blockId = 0;
  int level   = 0;

  // Root Block -- Block 0,0
  ndim[0] = 6; ndim[1]   = 6; ndim[2] = 6;
  h[0]      = h[1]  = h[2] = this->RootSpacing[0];
  origin[0] = origin[1] = -2.0; origin[2] = 0.0;
  blockId   = 0;
  level     = 0;
  vtkUniformGrid *grid = this->GetGrid(origin, h, ndim);
  amr->SetDataSet(level,blockId,grid);
  grid->Delete();
  vtkAMRUtilities::GenerateMetaData( amr, NULL );
  amr->GenerateVisibilityArrays();
}

//------------------------------------------------------------------------------
int vtkAMRGaussianPulseSource::RequestData(
    vtkInformation * vtkNotUsed(request),
    vtkInformationVector **vtkNotUsed(inputVector),
    vtkInformationVector *outputVector )
{
  vtkInformation *info = outputVector->GetInformationObject(0);
  vtkOverlappingAMR *output =
      vtkOverlappingAMR::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  assert("pre: output should not be NULL!" && (output != NULL) );

  switch( this->Dimension )
    {
    case 2:
      this->Generate2DDataSet( output );
      break;
    case 3:
      this->Generate3DDataSet( output );
      break;
    default:
      vtkErrorMacro("Dimensions must be either 2 or 3!");
    }
  vtkAMRUtilities::GenerateMetaData( output, NULL );
  return 1;
}

