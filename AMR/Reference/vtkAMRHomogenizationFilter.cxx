/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRHomogenizationFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "vtkAMRHomogenizationFilter.h"
#include "vtkObjectFactory.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkAMRUtilities.h"
#include "vtkUniformGrid.h"
#include "vtkExtractVOI.h"
#include "vtkUniformGrid.h"
#include "vtkMultiBlockDataSet.h"

#include <cassert>
#include <set>
#include <vector>

//
// Standard New
//
vtkStandardNewMacro( vtkAMRHomogenizationFilter );

vtkAMRHomogenizationFilter::vtkAMRHomogenizationFilter()
{
  this->SetNumberOfInputPorts( 1 );
  this->SetNumberOfOutputPorts( 1 );
}

//------------------------------------------------------------------------------
vtkAMRHomogenizationFilter::~vtkAMRHomogenizationFilter()
{
  // TODO Auto-generated destructor stub
}

//------------------------------------------------------------------------------
void vtkAMRHomogenizationFilter::PrintSelf(
    std::ostream &oss, vtkIndent indent )
{
  this->Superclass::PrintSelf( oss, indent );
}

//------------------------------------------------------------------------------
int vtkAMRHomogenizationFilter::FillInputPortInformation(
    int vtkNotUsed(port),vtkInformation *info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkAMRHomogenizationFilter::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(
   vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
void vtkAMRHomogenizationFilter::GetPatchExtent(
    vtkUniformGrid *ug, int celldims[3], int cellijk[3], int pExtent[6],
    std::set<vtkIdType> &cellHistory )
{
  assert( "pre: input grid is NULL" && (ug != NULL) );

  pExtent[0] = cellijk[0]; // imin
  pExtent[1] = cellijk[0]; // imax
  pExtent[2] = cellijk[1]; // jmin
  pExtent[3] = cellijk[1]; // jmax
  pExtent[4] = cellijk[2]; // kmin
  pExtent[5] = cellijk[2]; // kmax

  int i,j,k;
  // Sweep along i till we hit an inter-level interface
  j=cellijk[1]; k=cellijk[2];
  for( i=cellijk[0]+1; i < celldims[0]; ++i )
    {
      int ijk[3];
      ijk[0]=i; ijk[1]=j; ijk[2]=k;
      vtkIdType cellIdx = vtkStructuredData::ComputePointId(celldims,ijk);
      if( ug->IsCellVisible( cellIdx ) )
        {
          cellHistory.insert( cellIdx );
          pExtent[1]++;
        }
      else
        break;
    }

  // Sweep along j till we hit an inter-level interface
  for( j=cellijk[1]+1; j < celldims[1]; ++j )
    {

      bool goToNextLevel = true;
      std::vector< vtkIdType > cellids;
      for( i=cellijk[0]; i <= pExtent[1]; ++i )
        {
          int ijk[3];
          ijk[0]=i; ijk[1]=j; ijk[2]=k;
          vtkIdType cellIdx=vtkStructuredData::ComputePointId(celldims,ijk);
          if( ! ug->IsCellVisible(cellIdx) )
            {
              goToNextLevel = false;
              break;
            }
          else
            {
              cellids.push_back( cellIdx );
            }

        } // END for the i extent of the current patch

      if( goToNextLevel )
        {
          pExtent[3]++;
          for( unsigned int myIdx=0; myIdx < cellids.size(); ++myIdx )
            cellHistory.insert( cellids[ myIdx ] );
        }
      else
        break;

    } // END for all j

  std::cout << "For cell ijk: ";
  for( int ii=0; ii < 3; ++ii )
   std::cout << cellijk[ ii ] << " ";
  std::cout << std::endl;
  std::cout.flush();

  std::cout << "extents: ";
  for( int ii=0; ii < 3; ++ii )
    {
      std::cout << "[" << pExtent[ii*2] << "-";
      std::cout << pExtent[ii*2+1] << "] ";
    }
  std::cout << std::endl;
  std::cout.flush();

// TODO: support 3-D
// Sweep along k till we hit an inter-level interface
//  for( int k=cellijk[2]; k < celldims[2]; ++k )
//    {
//
//    }

}

//------------------------------------------------------------------------------
vtkUniformGrid* vtkAMRHomogenizationFilter::ExtractPatch(
    vtkUniformGrid *ug, int extent[6] )
{
  assert( "pre: input grid is NULL" && (ug != NULL) );

  int dims[3];
  dims[0] = dims[1] = dims[2] = 1;
  for( int i=0; i < ug->GetDataDimension(); ++i )
    dims[i] = extent[i*2+1]-extent[i*2]+2;

  int min[3];
  min[0] = extent[0];
  min[1] = extent[2];
  min[2] = extent[4];
  vtkIdType pntIdx = vtkStructuredData::ComputePointId(ug->GetDimensions(),min);

  std::cout << "Grid dimensions: ";
  for( int i=0; i < 3; ++i )
    std::cout << dims[i] << " ";
  std::cout << std::endl;
  std::cout.flush();

  vtkUniformGrid *myGrid = vtkUniformGrid::New();
  myGrid->SetOrigin( ug->GetPoint( pntIdx ) );
  myGrid->SetSpacing( ug->GetSpacing() );
  myGrid->SetDimensions( dims );


  return( myGrid );
}

//------------------------------------------------------------------------------
void vtkAMRHomogenizationFilter::ExtractNonOverlappingPatches(
    vtkUniformGrid *ug, const unsigned int level,
    vtkMultiBlockDataSet *outAMR )
{
  assert( "pre: input grid is NULL" && (ug != NULL) );
  assert( "pre: output AMR dataset is NULL" && (outAMR != NULL) );

  vtkExtractVOI *gridExtractionFilter  = vtkExtractVOI::New();
  gridExtractionFilter->SetInput( ug );
  gridExtractionFilter->SetSampleRate(1,1,1);

  std::set< vtkIdType > cellHistory;
  int patchExtent[6];
  int celldims[3];
  int cellijk[3];

  ug->GetDimensions( celldims );
  celldims[0]--; celldims[1]--; celldims[2]--;
  celldims[0] = (celldims[0] < 1)? 1 : celldims[0];
  celldims[1] = (celldims[1] < 1)? 1 : celldims[1];
  celldims[2] = (celldims[2] < 1)? 1 : celldims[2];

  std::cout << "cell dims: ";
  for( int i=0; i < 3; ++i )
    std::cout << celldims[ i ] << " ";
  std::cout << std::endl;
  std::cout.flush();

  for( cellijk[0]=0; cellijk[0] < celldims[0]; ++cellijk[0] )
    {
      for( cellijk[1]=0; cellijk[1] < celldims[1]; ++cellijk[1] )
        {
          for( cellijk[2]=0; cellijk[2] < celldims[2]; ++cellijk[2] )
             {
               // Since celldims consists of the cell dimensions,ComputePointId
               // is sufficient to get the corresponding linear cell index!
               vtkIdType cellIdx=
                vtkStructuredData::ComputePointId(celldims,cellijk);
               assert( "cell index out-of-bounds!" &&
                (cellIdx >= 0) && (cellIdx < ug->GetNumberOfCells()));

               // Skip cells that have been already extracted
               if( cellHistory.find(cellIdx) != cellHistory.end() )
                 continue;

               // Get the non-overlapping patch extent
               if( ug->IsCellVisible( cellIdx ) )
                 {

                   // Compute the patch extent
                   this->GetPatchExtent(
                       ug, celldims, cellijk,
                       patchExtent, cellHistory );

                   std::cout << "patchExtent: ";
                   for( int i=0; i < 3; ++i )
                     {
                       std::cout << patchExtent[ i*2 ] << "-";
                       std::cout << patchExtent[ i*2+1 ] << ", ";
                     }
                   std::cout << std::endl;
                   std::cout.flush();

                   vtkUniformGrid *patch = this->ExtractPatch(ug,patchExtent);

                   // Create point-based voi
//                   int voi[6];
//                   voi[0] = patchExtent[0];
//                   voi[1] = patchExtent[2];
//                   voi[2] = patchExtent[4];
//                   voi[3] = patchExtent[1]+1;
//                   voi[4] = patchExtent[3]+1;
//                   voi[5] = patchExtent[5]+1;
//
//                   std::cout << "point-based voi: ";
//                   for( int i=0; i < 3; ++i )
//                     {
//                       std::cout << voi[i] << "-";
//                       std::cout << voi[i+3] << " ";
//                     }
//                   std::cout << std::endl;
//                   std::cout.flush();

                   // Extract the patch extent & data from the grid
//                   gridExtractionFilter->SetVOI( voi );
//                   gridExtractionFilter->Update();
//                   vtkImageData *patch = gridExtractionFilter->GetOutput();
//                   vtkUniformGrid *patch=
//                    vtkUniformGrid::SafeDownCast(
//                     gridExtractionFilter->GetOutput() );
//                   assert( "extracted patch is NULL!" && (patch != NULL) );

                   unsigned int blockIdx = outAMR->GetNumberOfBlocks();
                   outAMR->SetBlock(blockIdx,patch);
                   patch->Delete();
                 }


             } // END for all k
        } // END for all j
    } // END for all i

//
//  gridExtractionFilter->Delete();
}

//------------------------------------------------------------------------------
void vtkAMRHomogenizationFilter::HomogenizeGrids(
    vtkHierarchicalBoxDataSet *inAMR, vtkMultiBlockDataSet *outAMR )
{
  assert( "pre: input AMR dataset is NULL" && (inAMR != NULL) );
  assert( "pre: output AMR dataset is NULL" );

  unsigned int level=0;
  for( ; level < inAMR->GetNumberOfLevels()-1; ++level )
    {
      unsigned int dataIdx = 0;
      for( ; dataIdx < inAMR->GetNumberOfDataSets( level ); ++dataIdx )
        {

          vtkUniformGrid *ug = inAMR->GetDataSet(level,dataIdx);
          if( ug != NULL )
            {
              this->ExtractNonOverlappingPatches( ug, level, outAMR );
            }

        } // END for all data
    } // END for all levels

  level = inAMR->GetNumberOfLevels()-1;
  unsigned int dataIdx = 0;
  for( ; dataIdx < inAMR->GetNumberOfDataSets(level); ++dataIdx )
    {
       vtkUniformGrid *patch = inAMR->GetDataSet( level, dataIdx );
       unsigned int blockIdx = outAMR->GetNumberOfBlocks();
       outAMR->SetBlock(blockIdx,patch);
       patch->Delete();
    }
//  vtkAMRUtilities::GenerateMetaData( outAMR, NULL );
}

//------------------------------------------------------------------------------
int vtkAMRHomogenizationFilter::RequestData(
    vtkInformation *info, vtkInformationVector **inputVector,
    vtkInformationVector *outputVector )
{
  vtkInformation *input  = inputVector[0]->GetInformationObject(0);
  assert( "pre: Null input information object!" && (input != NULL) );

  vtkInformation *output = outputVector->GetInformationObject(0);
  assert( "pre: Null output information object!" && (output != NULL) );

  vtkHierarchicalBoxDataSet *inAMR=
    vtkHierarchicalBoxDataSet::SafeDownCast(
      input->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input AMR dataset is NULL" && (inAMR != NULL) );

  vtkMultiBlockDataSet *outAMR=
    vtkMultiBlockDataSet::SafeDownCast(
      output->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: output AMR dataset is NULL" && (outAMR != NULL) );

  this->HomogenizeGrids( inAMR, outAMR );
  return 1;
}
