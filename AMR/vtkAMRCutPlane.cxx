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
#include "vtkPointLocator.h"

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
  this->plane            = NULL;
  this->UseNativeCutter  = 1;
  this->Locator          = vtkPointLocator::New();
}

//------------------------------------------------------------------------------
vtkAMRCutPlane::~vtkAMRCutPlane()
{
  this->blocksToLoad.clear();
  if( this->Locator != NULL )
    this->Locator->Delete();
  this->Locator = NULL;
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
    vtkInformationVector* outputVector )
{
  this->blocksToLoad.clear();

  if( this->plane != NULL )
    this->plane->Delete();

  vtkInformation *input = inputVector[0]->GetInformationObject(0);
  assert( "pre: input information object is NULL" && (input != NULL) );

  if( input->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
      vtkHierarchicalBoxDataSet *metadata =
          vtkHierarchicalBoxDataSet::SafeDownCast(
              input->Get(
                  vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) );

      this->plane = this->GetCutPlane( metadata );
      assert( "Cut plane is NULL" && (this->plane != NULL) );

      this->ComputeAMRBlocksToLoad( this->plane, metadata );
    }

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRCutPlane::RequestUpdateExtent(
    vtkInformation* vtkNotUsed(rqst), vtkInformationVector** inputVector,
    vtkInformationVector* outputVector)
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

  unsigned int blockIdx = 0;
  unsigned int level    = 0;
  for( ; level < inputAMR->GetNumberOfLevels(); ++level )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < inputAMR->GetNumberOfDataSets( level ); ++dataIdx )
        {

          if( this->UseNativeCutter == 1 )
            {
              vtkUniformGrid *grid = inputAMR->GetDataSet( level, dataIdx );
              if( grid != NULL )
                {
                  vtkCutter *myCutter = vtkCutter::New();
                  myCutter->SetInput( grid );
                  myCutter->SetCutFunction( this->plane );
                  myCutter->Update();
                  mbds->SetBlock( blockIdx, myCutter->GetOutput( ) );
                  ++blockIdx;
                  myCutter->Delete();
                }
              else
                {
                  // TODO: implement this
                }
            }
          else
            {

            }

        } // END for all data
    } // END for all levels

  this->Modified();
  return 1;
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

  int maxLevelToLoad =
     (this->LevelOfResolution < m->GetNumberOfLevels() )?
         this->LevelOfResolution : m->GetNumberOfLevels();

  unsigned int level = 0;
  for( ; level <= maxLevelToLoad; ++level )
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
