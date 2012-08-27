/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractLevel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractLevel.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkUniformGrid.h"
#include "vtkUniformGridAMR.h"
#include "vtkOverlappingAMR.h"

#include <set>
#include <vector>

class vtkExtractLevel::vtkSet : public std::set<unsigned int>
{
};

vtkStandardNewMacro(vtkExtractLevel);
//----------------------------------------------------------------------------
vtkExtractLevel::vtkExtractLevel()
{
  this->Levels = new vtkExtractLevel::vtkSet();
}

//----------------------------------------------------------------------------
vtkExtractLevel::~vtkExtractLevel()
{
  delete this->Levels;
}

//----------------------------------------------------------------------------
void vtkExtractLevel::AddLevel(unsigned int level)
{
  this->Levels->insert(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractLevel::RemoveLevel(unsigned int level)
{
  this->Levels->erase(level);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractLevel::RemoveAllLevels()
{
  this->Levels->clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkExtractLevel::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkUniformGridAMR");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractLevel::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

int vtkExtractLevel::RequestUpdateExtent(vtkInformation* , vtkInformationVector** inputVector,vtkInformationVector* )
{
  vtkInformation* inInfo   = inputVector[0]->GetInformationObject(0);

  // Check if metadata are passed downstream
  if( inInfo->Has(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA() ) )
    {
    vtkOverlappingAMR *metadata = vtkOverlappingAMR::SafeDownCast(
      inInfo->Get(vtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    if(metadata)
      {
      // cout<<"Time dependent? "<<inInfo->Has(vtkStreamingDemandDrivenPipeline::TIME_DEPENDENT_INFORMATION())<<endl;
      // std::cout<<"Receive Meta Data: ";
      // for(int levelIdx=0 ; levelIdx < metadata->GetNumberOfLevels(); ++levelIdx )
      //   {
      //   std::cout << " \tL(" << levelIdx << ") = "
      //             << metadata->GetNumberOfDataSets( levelIdx ) << " ";
      //   std::cout.flush();
      //   } // END for levels
      // std::cout<<endl;

      // Tell reader to load all requested blocks.
      inInfo->Set( vtkCompositeDataPipeline::LOAD_REQUESTED_BLOCKS(), 1 );

      // request the blocks
      std::vector<int> blocksToLoad;
      for(vtkExtractLevel::vtkSet::iterator iter =this->Levels->begin(); iter!= this->Levels->end(); ++iter )
        {
        unsigned int level = (*iter);
        for(unsigned int dataIdx=0;dataIdx < metadata->GetNumberOfDataSets(level);++dataIdx )
          {
          blocksToLoad.push_back(metadata->GetCompositeIndex(level,dataIdx));
          }
        }

      inInfo->Set( vtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(),&blocksToLoad[0], static_cast<int>(blocksToLoad.size()));
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkExtractLevel::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector )
{
  // STEP 0: Get input object

  vtkInformation* inInfo   = inputVector[0]->GetInformationObject(0);
  vtkUniformGridAMR *input =
   vtkUniformGridAMR::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  if( input == NULL )
    {
    return( 0 );
    }

  // STEP 1: Get output object
  vtkInformation* info = outputVector->GetInformationObject(0);
  vtkMultiBlockDataSet *output =
      vtkMultiBlockDataSet::SafeDownCast(
          info->Get(vtkDataObject::DATA_OBJECT()));
  if( output == NULL )
    {
    return( 0 );
    }

  // STEP 2: Compute the total number of blocks to be loaded
  unsigned int numBlocksToLoad = 0;
  vtkExtractLevel::vtkSet::iterator iter;
  for( iter =this->Levels->begin(); iter != this->Levels->end(); ++iter )
    {
    unsigned int level = (*iter);
    numBlocksToLoad += input->GetNumberOfDataSets(level);
    } // END for all requested levels
  output->SetNumberOfBlocks( numBlocksToLoad );

  // STEP 3: Load the blocks at the selected levels
  if( numBlocksToLoad > 0 )
    {
    iter = this->Levels->begin();
    unsigned int blockIdx = 0;
    for( ;iter != this->Levels->end(); ++iter )
      {
      unsigned int level   = (*iter);
      unsigned int dataIdx = 0;
      for(; dataIdx < input->GetNumberOfDataSets(level); ++dataIdx )
        {
        vtkUniformGrid* data = input->GetDataSet(level,dataIdx);
        if( data != NULL )
          {
          vtkUniformGrid *copy = data->NewInstance();
          copy->ShallowCopy( data );
          output->SetBlock( blockIdx, copy );
          copy->Delete();
          ++blockIdx;
          } // END if data is not NULL
        } // END for all data at level l
      } // END for all requested levels
    } // END if numBlocksToLoad is greater than 0

  return( 1 );
}

//----------------------------------------------------------------------------
void vtkExtractLevel::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

