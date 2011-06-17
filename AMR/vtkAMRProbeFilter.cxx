/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRProbeFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include <cassert>
#include <list>
#include <set>
#include <algorithm>

#include "vtkInformation.h"
#include "vtkInformationVector.h"

#include "vtkAMRProbeFilter.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkAMRBox.h"
#include "vtkAMRGridIndexEncoder.h"

vtkStandardNewMacro(vtkAMRProbeFilter);

//------------------------------------------------------------------------------
vtkAMRProbeFilter::vtkAMRProbeFilter()
{
  this->SetNumberOfInputPorts( 2 );
  this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRProbeFilter::~vtkAMRProbeFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::PrintSelf(std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::SetAMRDataSet( vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  this->SetInput(0,amrds);
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::SetProbePoints( vtkPointSet *probes )
{
  assert( "pre: input probe points are NULL" && (probes != NULL) );
  this->SetInput(1,probes);
}

//------------------------------------------------------------------------------
bool vtkAMRProbeFilter::PointInAMRBlock(
    double x, double y, double z,
    int levelIdx, int blockIdx,
    vtkHierarchicalBoxDataSet *amrds )
{
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  assert( "pre: level index is out of bounds!" &&
           ( (levelIdx >= 0) && (levelIdx < amrds->GetNumberOfLevels() ) ) );
  assert( "pre: block index is out of bounds!" &&
    ( (blockIdx >= 0) && (blockIdx < amrds->GetNumberOfDataSets(levelIdx) ) ) );

  vtkAMRBox amrBox;
  amrds->GetMetaData( levelIdx, blockIdx, amrBox );

  if( amrBox.HasPoint( x, y, z ) )
    return true;
  return false;
}

//------------------------------------------------------------------------------
bool vtkAMRProbeFilter::FindPointInLevel(
    double x, double y, double z,
    int levelIdx,vtkHierarchicalBoxDataSet *amrds,
    int &blockId )
{
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  assert( "pre: level index is out of bounds!" &&
   ( (levelIdx >= 0) && (levelIdx < amrds->GetNumberOfLevels() ) ) );

  unsigned int dataIdx = 0;
  for( ; dataIdx < amrds->GetNumberOfDataSets( levelIdx ); ++dataIdx )
    {

      if( this->PointInAMRBlock(x,y,z,levelIdx,dataIdx,amrds) )
        {
          blockId = static_cast< int >( dataIdx );
          return true;
        }

    }

  return false;
}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::ExtractAMRBlocks(
    vtkMultiBlockDataSet *mbds, vtkHierarchicalBoxDataSet *amrds,
    std::set< unsigned int > &blocksToExtract )
{
  assert( "pre: multi-block dataset is NULL" && (mbds != NULL) );
  assert( "pre: AMR dataset is NULL" && (amrds != NULL) );

  mbds->SetNumberOfBlocks( blocksToExtract.size() );

  unsigned int blockIdx = 0;
  std::set< unsigned int >::iterator iter = blocksToExtract.begin();
  for( ; iter != blocksToExtract.end(); ++iter )
    {
      unsigned int gridIdx = *iter;
      int blockId = -1;
      int levelId = -1;
      vtkAMRGridIndexEncoder::Decode( gridIdx, levelId, blockId );
      assert( "level index out-of-bounds" &&
          ( (levelId >= 0) && ( levelId < amrds->GetNumberOfLevels())));
      assert( "block index out-of-bounds" &&
          ( (blockId >= 0) && (blockId < amrds->GetNumberOfDataSets(levelId))));

      vtkUniformGrid *ug = amrds->GetDataSet( levelId, blockId );
      if( ug != NULL )
        {
          vtkUniformGrid *clone = ug->NewInstance();
          clone->ShallowCopy( ug );
          clone->SetCellVisibilityArray( NULL );
          clone->SetPointVisibilityArray( NULL );

          mbds->SetBlock( blockIdx, clone );
          ++blockIdx;
          clone->Delete();
        }

    } // END for all blocks

}

//------------------------------------------------------------------------------
void vtkAMRProbeFilter::ProbeAMR(
    vtkPointSet *probes, vtkHierarchicalBoxDataSet *amrds,
    vtkMultiBlockDataSet *mbds )
{
  // Sanity Check!
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );
  assert( "pre: input probe pointset is NULL" && (probes != NULL) );
  assert( "pre: multiblock output is NULL" && (mbds != NULL) );


  // Contains the packed (levelId,blockId) pair of the blocks
  // to extract in the multiblock dataset.
  std::set< unsigned int > blocksToExtract;

  // STEP 0: Initialize work ID list
  std::list< vtkIdType > workIdList;
  for( vtkIdType idx=0; idx < probes->GetNumberOfPoints(); ++idx )
    workIdList.push_front( idx );

  // STEP 1: loop while the workIdList is not empty, searching for a block
  // that contains the point, starting from highest resolution level to
  // lowest resolution
  while( !workIdList.empty() )
    {
      vtkIdType currentIdx = workIdList.front();
      workIdList.pop_front();

      double pnt[3];
      probes->GetPoint( currentIdx, pnt );

      int blockId               = -1;
      unsigned int currentLevel = amrds->GetNumberOfLevels()-1;
      bool isBlockFound  = false;
      for( ; currentLevel >= 0; --currentLevel )
        {

          if( this->FindPointInLevel(
              pnt[0],pnt[1],pnt[2],
              currentLevel,amrds,blockId ) )
            {
              assert( "invalid block ID" &&
                    ( (blockId >= 0) &&
                      (blockId < amrds->GetNumberOfDataSets(currentLevel) ) ) );

              int level            = static_cast< int >( currentLevel );
              unsigned int grididx =
               vtkAMRGridIndexEncoder::Encode(level,blockId);
              blocksToExtract.insert( grididx );
              isBlockFound = true;
              break;
            }

        } // END for all levels

      if( !isBlockFound )
        {
          //
          // TODO: What should we do with probes that were not found?
          //
        }

    } // END while work ID list is not empty

    // STEP 2: Extract the AMR blocks
    this->ExtractAMRBlocks( mbds, amrds, blocksToExtract );
}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::RequestData(
    vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector )
{

  vtkHierarchicalBoxDataSet *amrds=
   vtkHierarchicalBoxDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input AMR dataset is NULL" && (amrds != NULL) );

  vtkPointSet *probes=
   vtkPointSet::SafeDownCast(
    inputVector[1]->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: input probe pointset is NULL" && (probes != NULL) );

  vtkMultiBlockDataSet *mbds=
   vtkMultiBlockDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: multiblock output is NULL" && (mbds != NULL) );

  this->ProbeAMR( probes, amrds, mbds );

  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::FillInputPortInformation(int port, vtkInformation *info)
{
  int status = 0;
  switch( port )
    {
      case 0:
        info->Set(
         vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHierarchicalBoxDataSet");
        status = 1;
        break;
      case 1:
        info->Set(
         vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
        status = 1;
        break;
      default:
        vtkErrorMacro( "Called FillInputPortInformation with invalid port!" );
        status = 0;
    }
  return( status );
}

//------------------------------------------------------------------------------
int vtkAMRProbeFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}
