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
#include "vtkStructuredExtent.h"
#include "vtkCell.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkAMRBox.h"

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

#define PRINTEXT( extName, ext ) { \
  std::cout << extName << ": ";    \
  std::cout.flush();               \
  for( int ii=0; ii < 6; ++ii ) {  \
    std::cout << ext[ii] << " ";   \
  }                                \
  std::cout << std::endl;          \
  std::cout.flush();               \
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRGaussianPulseSource::RefinePatch(
    vtkUniformGrid* parent, int patchExtent[6] )
{
  assert("pre: parent grid is NULL!" && (parent!=NULL) );
  assert("dimension must be 2 or 3" && (this->Dimension<=3));

  int ext[6];
  parent->GetExtent(ext);
  assert("pre: patchExtent must be within the parent extent!" &&
         vtkStructuredExtent::Smaller(patchExtent,ext));

//  PRINTEXT("Parent",ext);
//  PRINTEXT("Patch", patchExtent);

  double min[3];
  double max[3];
  double h[3];
  double h0[3];
  int ndim[3];

  // Set some nominal values to ensure proper initialization
  ndim[0] = ndim[1] = ndim[2] = 1;
  min[0]  = min[1]  = min[2]  =
  max[0]  = max[1]  = max[2]  = 0.0;
  h[0]    = h[1]    = h[2]    =
  h0[0]   = h0[1]   = h0[2]   = 0.5;

  // STEP 0: Get min
  int minIJK[3];
  minIJK[0] = patchExtent[0];
  minIJK[1] = patchExtent[2];
  minIJK[2] = patchExtent[4];
  vtkIdType minIdx = vtkStructuredData::ComputePointIdForExtent(ext,minIJK);
//  std::cout << "min: " << minIdx << std::endl;
//  std::cout.flush();
  parent->GetPoint( minIdx, min );

  // STEP 1: Get max
  int maxIJK[3];
  maxIJK[0] = patchExtent[1];
  maxIJK[1] = patchExtent[3];
  maxIJK[2] = patchExtent[5];
  vtkIdType maxIdx = vtkStructuredData::ComputePointIdForExtent(ext,maxIJK);
//  std::cout << "max: " << maxIdx << std::endl;
//  std::cout.flush();
  parent->GetPoint( maxIdx, max );

  int patchdims[3];
  patchdims[0] = patchExtent[1]-patchExtent[0]+1;
  patchdims[1] = patchExtent[3]-patchExtent[2]+1;
  patchdims[2] = patchExtent[5]-patchExtent[4]+1;

  // STEP 2: Compute the spacing of the refined patch and its dimensions
  parent->GetSpacing( h0 );
  for( int i=0; i < this->Dimension; ++i )
    {
    h[i]    = h0[i]/static_cast<double>(this->RefinmentRatio);
    ndim[i] = this->RefinmentRatio*patchdims[i]-(this->RefinmentRatio-1);
    } // END for all dimensions

//  std::cout << "Computed h:"    << h[0] << " " << h[1] << " " << h[2] << "\n";
//  std::cout << "Computed ndim:" << ndim[0] << " " << ndim[1] << " " << ndim[2];
//  std::cout << std::endl;
//  std::cout.flush();

  // STEP 3: Construct uniform grid for requested patch
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin( min );
  grid->SetSpacing( h );
  grid->SetDimensions(ndim);

  // STEP 4: Compute Gaussian-Pulse field on patch
  this->GeneratePulseField(grid);
  return(grid);
}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRGaussianPulseSource::GetGrid(
    double origin[3], double h[3], int ndim[3] )
{
  vtkUniformGrid *grid = vtkUniformGrid::New();
  grid->Initialize();
  grid->SetOrigin(origin);
  grid->SetSpacing(h);
  grid->SetDimensions(ndim);

  this->GeneratePulseField(grid);
  return(grid);
}

//------------------------------------------------------------------------------
void vtkAMRGaussianPulseSource::Generate2DDataSet( vtkOverlappingAMR *amr )
{
  assert( "pre: input amr dataset is NULL" && (amr!=NULL) );

  int ndim[3];
  double origin[3];
  double h[3];
  int blockId = 0;
  int level   = 0;

  // Define the patches to be refined apriori
  int patches[2][6] = {
      {0,2,0,3,0,0},
      {3,5,2,5,0,0}
  };

  // Root Block -- Block 0,0
  ndim[0] = 6; ndim[1]   = 6; ndim[2] = 1;
  h[0]      = h[1]  = h[2] = this->RootSpacing[0];
  origin[0] = origin[1] = -2.0; origin[2] = 0.0;
  blockId   = 0;
  level     = 0;

  std::vector<int> blocksPerLevel(2);
  blocksPerLevel[0]=1;
  blocksPerLevel[1]=2;

  vtkUniformGrid *grid = this->GetGrid(origin, h, ndim);
  vtkAMRBox box(grid->GetOrigin(), grid->GetDimensions(), grid->GetSpacing(),origin,amr->GetGridDescription());

  amr->Initialize(2,&blocksPerLevel[0]);
  amr->SetOrigin(grid->GetOrigin());
  amr->SetGridDescription(grid->GetGridDescription());
  amr->SetSpacing(level,grid->GetSpacing());
  amr->SetAMRBox(level,blockId,box);
  amr->SetDataSet(level,blockId,grid);

  vtkUniformGrid *refinedPatch = NULL;
  for( int patchIdx=0; patchIdx < 2; ++patchIdx )
    {
    refinedPatch = RefinePatch( grid, patches[patchIdx] );
    assert("pre: refined grid is NULL" && (refinedPatch != NULL) );
    box = vtkAMRBox(refinedPatch->GetOrigin(), refinedPatch->GetDimensions(), refinedPatch->GetSpacing(),origin,amr->GetGridDescription());
    amr->SetSpacing(level+1,refinedPatch->GetSpacing());
    amr->SetAMRBox(level+1,patchIdx,box);
    amr->SetDataSet(level+1,patchIdx,refinedPatch);
    refinedPatch->Delete();
    refinedPatch = NULL;
    }

  grid->Delete();
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

  // Define the patches to be refined apriori
  int patches[2][6] = {
      {0,2,0,3,0,5},
      {3,5,2,5,0,5}
  };

  // Root Block -- Block 0,0
  ndim[0] = 6; ndim[1]   = 6; ndim[2] = 6;
  h[0]      = h[1]  = h[2] = this->RootSpacing[0];
  origin[0] = origin[1] = -2.0; origin[2] = 0.0;
  blockId   = 0;
  level     = 0;

  std::vector<int> blocksPerLevel(2);
  blocksPerLevel[0]=1;
  blocksPerLevel[1]=2;

  vtkUniformGrid *grid = this->GetGrid(origin, h, ndim);
  vtkAMRBox box (grid->GetOrigin(), grid->GetDimensions(), grid->GetSpacing(),origin,amr->GetGridDescription());

  amr->Initialize(2, &blocksPerLevel[0]);
  amr->SetOrigin(grid->GetOrigin());
  amr->SetGridDescription(grid->GetGridDescription());
  amr->SetSpacing(level,grid->GetSpacing());
  amr->SetAMRBox(level,blockId,box);
  amr->SetDataSet(level,blockId,grid);

  vtkUniformGrid *refinedPatch = NULL;
  for( int patchIdx=0; patchIdx < 2; ++patchIdx )
    {
    refinedPatch = RefinePatch( grid, patches[patchIdx] );
    assert("pre: refined grid is NULL" && (refinedPatch != NULL) );

    box = vtkAMRBox(refinedPatch->GetOrigin(), refinedPatch->GetDimensions(), refinedPatch->GetSpacing(),amr->GetOrigin(),amr->GetGridDescription());;
    amr->SetSpacing(level+1,refinedPatch->GetSpacing());
    amr->SetAMRBox(level+1,patchIdx,box);
    amr->SetDataSet(level+1,patchIdx,refinedPatch);
    refinedPatch->Delete();
    refinedPatch = NULL;
    }

  grid->Delete();
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

  vtkAMRUtilities::BlankCells(output);
  return 1;
}
