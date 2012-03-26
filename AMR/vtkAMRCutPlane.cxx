/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRCutPlane.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "vtkAMRCutPlane.h"
#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkCompositeDataPipeline.h"
#include "vtkMultiProcessController.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkIndent.h"
#include "vtkPlane.h"
#include "vtkAMRBox.h"
#include "vtkAMRUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkCutter.h"
#include "vtkLocator.h"
#include "vtkPointLocator.h"
#include "vtkPolyData.h"
#include "vtkContourValues.h"
#include "vtkIdList.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkPoints.h"

#include <cassert>
#include <algorithm>

vtkStandardNewMacro(vtkAMRCutPlane);

//------------------------------------------------------------------------------
vtkAMRCutPlane::vtkAMRCutPlane()
{
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
  this->LevelOfResolution = 0;
  this->initialRequest    = true;
  for( int i=0; i < 3; ++i )
    {
    this->Center[i] = 0.0;
    this->Normal[i] = 0.0;
    }
  this->Controller       = vtkMultiProcessController::GetGlobalController();
  this->Plane            = NULL;
  this->UseNativeCutter  = 1;
  this->contourValues    = vtkContourValues::New();
}

//------------------------------------------------------------------------------
vtkAMRCutPlane::~vtkAMRCutPlane()
{

  this->blocksToLoad.clear();

  if( this->contourValues != NULL )
    this->contourValues->Delete();
  this->contourValues = NULL;

  if( this->Plane != NULL )
    this->Plane->Delete();
  this->Plane = NULL;
}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::PrintSelf( std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(
      vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),
      "vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  assert( "pre: information object is NULL!" && (info != NULL) );
  info->Set(
      vtkDataObject::DATA_TYPE_NAME(), "vtkMultiBlockDataSet" );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestInformation(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{
  this->blocksToLoad.clear();

  if( this->Plane != NULL )
    this->Plane->Delete();

  vtkInformation *input = inputVector[0]->GetInformationObject(0);
  assert( "pre: input information object is NULL" && (input != NULL) );

  if( input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
      vtkHierarchicalBoxDataSet *metadata =
          vtkHierarchicalBoxDataSet::SafeDownCast(
              input->Get(
                  vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );

      this->Plane = this->GetCutPlane( metadata );
      assert( "Cut plane is NULL" && (this->Plane != NULL) );

      this->ComputeAMRBlocksToLoad( this->Plane, metadata );
    }

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestUpdateExtent(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* vtkNotUsed(outputVector) )
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  assert( "pre: inInfo is NULL" && (inInfo != NULL) );

  inInfo->Set(
      vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(),
      &this->blocksToLoad[0], this->blocksToLoad.size() );
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestData(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{
  // STEP 0: Get input object
  vtkInformation *input = inputVector[0]->GetInformationObject( 0 );
  assert( "pre: input information object is NULL" && (input != NULL)  );
  vtkHierarchicalBoxDataSet *inputAMR=
      vtkHierarchicalBoxDataSet::SafeDownCast(
          input->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input AMR dataset is NULL!" && (inputAMR != NULL) );

  // STEP 1: Get output object
  vtkInformation *output = outputVector->GetInformationObject( 0 );
  assert( "pre: output information is NULL" && (output != NULL) );
  vtkMultiBlockDataSet *mbds=
      vtkMultiBlockDataSet::SafeDownCast(
          output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output multi-block dataset is NULL" && (mbds != NULL) );

  if( this->IsAMRData2D( inputAMR ) )
    {
      // TODO: implemment this
    }
  unsigned int blockIdx = 0;
  unsigned int level    = 0;
  for( ; level < inputAMR->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < inputAMR->GetNumberOfDataSets( level ); ++dataIdx )
        {

          vtkUniformGrid *grid = inputAMR->GetDataSet( level, dataIdx );
          if( this->UseNativeCutter == 1 )
            {

              if( grid != NULL )
                {
                  vtkCutter *myCutter = vtkCutter::New();
                  myCutter->SetInputData( grid );
                  myCutter->SetCutFunction( this->Plane );
                  myCutter->Update();
                  mbds->SetBlock( blockIdx, myCutter->GetOutput( ) );
                  ++blockIdx;
                  myCutter->Delete();
                }
              else
                {
                  // TODO: handle the case where the dataset is distributed
                }
            }
          else
            {
              this->CutAMRBlock( grid, mbds );
            }

        } // END for all data
    } // END for all levels

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
//void vtkAMRCutPlane::CutAMRBlock(
//    vtkUniformGrid *grid, vtkMultiBlockDataSet *output )
//{
//  if( grid == NULL )
//    return NULL;

//  vtkPointLocator *myLocator = vtkPointLocator::New();
//  vtkPolyData *slice = vtkPolyData::New();
//
//  vtkDoubleArray *cutScalars = vtkDoubleArray::New();
//  cutScalars->SetName( "Fx");
//  cutScalars->SetNumberOfTuples( grid->GetNumberOfPoints() );
//  cutScalars->SetNumberOfComponents( 1 );
//  vtkIdType ptIdx = 0;
//  for( ; ptIdx < grid->GetNumberOfPoints(); ++ptIdx )
//    {
//     double val = this->Plane->EvaluateFunction( grid->GetPoint(ptIdx) );
//     cutScalars->SetComponent( ptIdx, 0, val );
//    } // END for all cells
//  grid->GetPointData()->AddArray( cutScalars );
//  unsigned int blockIdx = output->GetNumberOfBlocks( );
//  output->SetBlock( blockIdx, grid );
//}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::CutAMRBlock(
    vtkUniformGrid *grid, vtkMultiBlockDataSet *output )
{

  // Locator, used for detecting duplicate points
  vtkPointLocator *locator = vtkPointLocator::New();


  vtkPolyData *mesh   = vtkPolyData::New();
  vtkPoints *meshPts  = vtkPoints::New();
  vtkCellArray *cells = vtkCellArray::New();


  vtkIdType cellIdx = 0;
  for( ; cellIdx < grid->GetNumberOfCells(); ++cellIdx )
    {

      if( grid->IsCellVisible( cellIdx ) &&
          this->PlaneIntersectsCell( grid->GetCell(cellIdx) ) )
        {
          this->ExtractCellFromGrid(
              grid,grid->GetCell(cellIdx),locator,meshPts,cells );
        } // END if

    } // END for all cells

  locator->Delete();
  unsigned int blockIdx = output->GetNumberOfBlocks();
  output->SetBlock( blockIdx, mesh );
}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::ExtractCellFromGrid(
            vtkUniformGrid *grid,
            vtkCell* cell, vtkLocator *loc,
            vtkPoints *pts, vtkCellArray *cells)
{
  assert( "pre: grid is NULL"  && (grid != NULL)  );
  assert( "pre: cell is NULL"  && (cell != NULL)  );
  assert( "pre: loc is NULL"   && (loc != NULL)   );
  assert( "pre: pts is NULL"   && (pts != NULL)   );
  assert( "pre: cells is NULL" && (cells != NULL) );

  vtkIdType nodeIdx = 0;
  for( ; nodeIdx < cell->GetNumberOfPoints(); ++nodeIdx )
    {

    } // END for all nodes

}

//------------------------------------------------------------------------------
vtkPlane* vtkAMRCutPlane::GetCutPlane( vtkHierarchicalBoxDataSet *metadata )
{
  assert( "pre: metadata is NULL" && (metadata != NULL) );

  vtkPlane *pl = vtkPlane::New();

  // Get global bounds
  double minBounds[3];
  double maxBounds[3];
  vtkAMRBox root;
  metadata->GetRootAMRBox( root );
  root.GetMinBounds( minBounds );
  root.GetMaxBounds( maxBounds );

  this->InitializeCenter( minBounds, maxBounds );

  pl->SetNormal( this->Normal );
  pl->SetOrigin( this->Center );
  return( pl );
}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::ComputeAMRBlocksToLoad(
      vtkPlane* p, vtkHierarchicalBoxDataSet *m)
{
  assert( "pre: Plane object is NULL" && (p != NULL) );
  assert( "pre: metadata is NULL" && (m != NULL) );

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = p->GetNormal()[0];
  plane[1] = p->GetNormal()[1];
  plane[2] = p->GetNormal()[2];
  plane[3] = p->GetNormal()[0]*p->GetOrigin()[0] +
             p->GetNormal()[1]*p->GetOrigin()[1] +
             p->GetNormal()[2]*p->GetOrigin()[2];

  double bounds[6];

  int NumLevels = m->GetNumberOfLevels();
  int maxLevelToLoad =
     (this->LevelOfResolution < NumLevels )?
         this->LevelOfResolution : NumLevels;

  unsigned int level = 0;
  for( ; level <= static_cast<unsigned int>(maxLevelToLoad); ++level )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < m->GetNumberOfDataSets( level ); ++dataIdx )
        {
          vtkAMRBox box;
          m->GetMetaData( level, dataIdx, box  );
          bounds[0] = box.GetMinX();
          bounds[1] = box.GetMaxX();
          bounds[2] = box.GetMinY();
          bounds[3] = box.GetMaxY();
          bounds[4] = box.GetMinZ();
          bounds[5] = box.GetMaxZ();

          if( this->PlaneIntersectsAMRBox( plane, bounds ) )
            {
              unsigned int amrGridIdx =
                  m->GetCompositeIndex(level,dataIdx);
              this->blocksToLoad.push_back( amrGridIdx );
            }
        } // END for all data
    } // END for all levels

    std::sort( this->blocksToLoad.begin(), this->blocksToLoad.end() );
}

//------------------------------------------------------------------------------
void vtkAMRCutPlane::InitializeCenter( double min[3], double max[3] )
{
  if( !this->initialRequest )
    return;

  this->Center[0] = 0.5*( max[0]-min[0] );
  this->Center[1] = 0.5*( max[1]-min[1] );
  this->Center[2] = 0.5*( max[2]-min[2] );
  this->initialRequest = false;
}

//------------------------------------------------------------------------------
bool vtkAMRCutPlane::PlaneIntersectsCell( vtkCell *cell )
{
  assert( "pre: cell is NULL!" && (cell != NULL) );
  return( this->PlaneIntersectsAMRBox( cell->GetBounds() ) );
}
//------------------------------------------------------------------------------
bool vtkAMRCutPlane::PlaneIntersectsAMRBox( double bounds[6] )
{
  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = this->Plane->GetNormal()[0];
  plane[1] = this->Plane->GetNormal()[1];
  plane[2] = this->Plane->GetNormal()[2];
  plane[3] = this->Plane->GetNormal()[0]*this->Plane->GetOrigin()[0] +
             this->Plane->GetNormal()[1]*this->Plane->GetOrigin()[1] +
             this->Plane->GetNormal()[2]*this->Plane->GetOrigin()[2];

 return( this->PlaneIntersectsAMRBox( plane,bounds) );
}

//------------------------------------------------------------------------------
bool vtkAMRCutPlane::PlaneIntersectsAMRBox( double plane[4], double bounds[6] )
{
  bool lowPnt  = false;
  bool highPnt = false;

  for( int i=0; i < 8; ++i )
    {
      // Get box coordinates
      double x = ( i&1 ? bounds[1] : bounds[0] );
      double y = ( i&2 ? bounds[3] : bounds[2] );
      double z = ( i&3 ? bounds[5] : bounds[4] );

      // Plug-in coordinates to the plane equation
      double v = plane[3] - plane[0]*x - plane[1]*y - plane[2]*z;

      if( v == 0.0 ) // Point is on a plane
        return true;

      if( v < 0.0 )
        lowPnt = true;
      else
        highPnt = true;

      if( lowPnt && highPnt )
        return true;
    }

    return false;
}

//------------------------------------------------------------------------------
bool vtkAMRCutPlane::IsAMRData2D( vtkHierarchicalBoxDataSet *input )
{
  assert( "pre: Input AMR dataset is NULL" && (input != NULL)  );

  vtkAMRBox box;
  input->GetMetaData( 0, 0, box );

  if( box.GetDimensionality() == 2 )
   return true;

 return false;
}
