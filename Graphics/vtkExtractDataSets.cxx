/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractDataSets.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtractDataSets.h"

#include "vtkObjectFactory.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkHierarchicalBoxDataSet.h"
#include "vtkUniformGrid.h"
#include "vtkAMRBox.h"
#include "vtkUnsignedCharArray.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"

#include <cassert>
#include <vtkstd/set>

class vtkExtractDataSets::vtkInternals
{
public:
  struct Node
    {
    unsigned int Level;
    unsigned int Index;
    
    bool operator() (const Node& n1, const Node& n2) const
      {
      if (n1.Level == n2.Level)
        {
        return (n1.Index < n2.Index);
        }
      return (n1.Level < n2.Level);
      }
    };


  typedef vtkstd::set<Node, Node> DatasetsType;
  DatasetsType Datasets;
};

vtkStandardNewMacro(vtkExtractDataSets);
//----------------------------------------------------------------------------
vtkExtractDataSets::vtkExtractDataSets()
{
  this->Internals = new vtkInternals();
}

//----------------------------------------------------------------------------
vtkExtractDataSets::~vtkExtractDataSets()
{
  delete this->Internals;
}

//----------------------------------------------------------------------------
void vtkExtractDataSets::AddDataSet(
  unsigned int level, unsigned int idx)
{
  vtkInternals::Node node;
  node.Level = level;
  node.Index = idx;
  this->Internals->Datasets.insert(node);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkExtractDataSets::ClearDataSetList()
{
//  std::cout << "Called clear datasets list!!!!\n";
//  std::cout.flush();

  this->Internals->Datasets.clear();
  this->Modified();
}

//------------------------------------------------------------------------------
int vtkExtractDataSets::FillInputPortInformation(
    int vtkNotUsed(port), vtkInformation* info )
{
  info->Set(
   vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(),"vtkHierarchicalBoxDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractDataSets::FillOutputPortInformation(
    int vtkNotUsed(port), vtkInformation *info )
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(),"vtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int vtkExtractDataSets::RequestData(
    vtkInformation *vtkNotUsed(request),
    vtkInformationVector **inputVector,
    vtkInformationVector *outputVector)
{
  // STEP 0: Get input
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  assert( "pre: input information object is NULL!" && (inInfo != NULL) );
  vtkHierarchicalBoxDataSet *input =
    vtkHierarchicalBoxDataSet::SafeDownCast(
        inInfo->Get( vtkDataObject::DATA_OBJECT() ) );
  assert( "pre: input dataset is NULL!" && (input != NULL) );

  // STEP 1: Get output
  vtkInformation* info = outputVector->GetInformationObject(0);
  assert( "pre: output information object is NULL!" && (info != NULL) );
  vtkMultiBlockDataSet *output =
   vtkMultiBlockDataSet::SafeDownCast(info->Get(vtkDataObject::DATA_OBJECT()));
  assert( "pre: output dataset is NULL!" && (output != NULL) );

  // STEP 2: Initialize structure
  output->SetNumberOfBlocks( input->GetNumberOfLevels() );
  unsigned int blk = 0;
  for( ; blk < output->GetNumberOfBlocks(); ++blk )
    {
      vtkMultiPieceDataSet *mpds = vtkMultiPieceDataSet::New();
//      mpds->SetNumberOfPieces( input->GetNumberOfDataSets( blk ) );
      output->SetBlock( blk, mpds );
    } // END for all blocks/levels

  // STEP 3: Loop over sected blocks
  vtkInternals::DatasetsType::iterator iter = this->Internals->Datasets.begin();
  for (;iter != this->Internals->Datasets.end(); ++iter)
    {
      vtkAMRBox box;
      vtkUniformGrid* inUG = input->GetDataSet(iter->Level, iter->Index, box);
      if( inUG )
        {
          vtkMultiPieceDataSet *mpds =
           vtkMultiPieceDataSet::SafeDownCast( output->GetBlock(iter->Level) );
          assert( "pre: mpds is NULL!" && (mpds!=NULL) );

          unsigned int out_index = mpds->GetNumberOfPieces();
          vtkUniformGrid* clone = inUG->NewInstance();
          clone->ShallowCopy(inUG);

          // Remove blanking from output datasets.
          clone->SetCellVisibilityArray(0);
          mpds->SetPiece( out_index, clone );
          clone->Delete();
        }

    } // END for all selected items

  return 1;
}

//int vtkExtractDataSets::RequestData(
//  vtkInformation *vtkNotUsed(request),
//  vtkInformationVector **inputVector,
//  vtkInformationVector *outputVector)
//{
//  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
//  vtkHierarchicalBoxDataSet *input = vtkHierarchicalBoxDataSet::SafeDownCast(
//    inInfo->Get(vtkDataObject::DATA_OBJECT()));
//
//  vtkInformation* info = outputVector->GetInformationObject(0);
//  vtkHierarchicalBoxDataSet *output = vtkHierarchicalBoxDataSet::SafeDownCast(
//    info->Get(vtkDataObject::DATA_OBJECT()));
//
//  unsigned int numLevels = input->GetNumberOfLevels();
//  output->SetNumberOfLevels(numLevels);
//
//  // copy levels meta-data.
//  for (unsigned int cc=0; cc < numLevels; cc++)
//    {
//    if (input->HasLevelMetaData(cc))
//      {
//      output->GetLevelMetaData(cc)->Copy(input->GetLevelMetaData(cc));
//      }
//    }
//
//
//  // Note that we need to ensure that all processess have the same tree
//  // structure in the output.
//  vtkInternals::DatasetsType::iterator iter;
//  for (iter = this->Internals->Datasets.begin();
//    iter != this->Internals->Datasets.end(); ++iter)
//    {
//    vtkAMRBox box;
//    vtkUniformGrid* inUG = input->GetDataSet(
//      iter->Level, iter->Index, box);
//    unsigned int out_index = output->GetNumberOfDataSets(iter->Level);
//    output->SetNumberOfDataSets(iter->Level, out_index+1);
//    if (input->HasMetaData(iter->Level, iter->Index))
//      {
//      output->GetMetaData(iter->Level, out_index)->Copy(
//        input->GetMetaData(iter->Level, iter->Index));
//      }
//
//    if (inUG)
//      {
//      vtkUniformGrid* clone = inUG->NewInstance();
//      clone->ShallowCopy(inUG);
//
//      // Remove blanking from output datasets.
//      clone->SetCellVisibilityArray(0);
//      output->SetDataSet(iter->Level,
//        out_index, box, clone);
//      clone->Delete();
//      }
//    }
//
//  // regenerate blanking.
//  output->GenerateVisibilityArrays();
//  return 1;
//}


//----------------------------------------------------------------------------
void vtkExtractDataSets::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

