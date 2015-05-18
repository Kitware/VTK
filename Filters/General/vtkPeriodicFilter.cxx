/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkPeriodicFilter.h"

#include "vtkDataObjectTreeIterator.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkMultiPieceDataSet.h"

#include <sstream>
//----------------------------------------------------------------------------
vtkPeriodicFilter::vtkPeriodicFilter()
{
  this->IterationMode = VTK_ITERATION_MODE_MAX;
  this->NumberOfPeriods = 1;
}

//----------------------------------------------------------------------------
vtkPeriodicFilter::~vtkPeriodicFilter()
{
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  if (this->IterationMode == VTK_ITERATION_MODE_DIRECT_NB)
    {
    os << indent << "Iteration Mode: Direct Number" << endl;
    os << indent << "Number of Periods: " << this->NumberOfPeriods << endl;
    }
  else
    {
    os << indent << "Iteration Mode: Maximum" << endl;
    }
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::AddIndex(unsigned int index)
{
  this->Indices.insert(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::RemoveIndex(unsigned int index)
{
  this->Indices.erase(index);
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::RemoveAllIndices()
{
  this->Indices.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::CreatePeriodicSubTree(vtkDataObjectTreeIterator* loc,
                                              vtkMultiBlockDataSet* output,
                                              vtkMultiBlockDataSet* input)
{
  vtkDataObject* inputNode = input->GetDataSet(loc);
  if (!inputNode)
    {
    return;
    }
  if (!inputNode->IsA("vtkCompositeDataSet"))
    {
    // We are on a leaf, process it
    this->CreatePeriodicDataSet(loc, output, input);
    }
  else
    {
    // Recursively process the composite tree
    vtkCompositeDataSet* cinput = vtkCompositeDataSet::SafeDownCast(inputNode);
    vtkCompositeDataSet* coutput =
      vtkCompositeDataSet::SafeDownCast(output->GetDataSet(loc));
    if (coutput == NULL)
      {
      return;
      }
    vtkCompositeDataIterator* iter = cinput->NewIterator();
    vtkDataObjectTreeIterator* treeIter =
      vtkDataObjectTreeIterator::SafeDownCast(iter);
    if (treeIter)
      {
      treeIter->VisitOnlyLeavesOff();
      }
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
      vtkDataObject* inputNode2 = cinput->GetDataSet(iter);
      if (inputNode2 == NULL)
        {
        break;
        }
      if (!inputNode2->IsA("vtkCompositeDataSet"))
        {
        this->CreatePeriodicDataSet(iter, coutput, cinput);
        }

      this->ActiveIndices.erase(
        loc->GetCurrentFlatIndex() + iter->GetCurrentFlatIndex());
      }
    iter->Delete();
    }
}

//----------------------------------------------------------------------------
int vtkPeriodicFilter::RequestData(vtkInformation *vtkNotUsed(request),
                                   vtkInformationVector **inputVector,
                                   vtkInformationVector *outputVector)
{
  vtkMultiBlockDataSet *input = vtkMultiBlockDataSet::GetData(inputVector[0], 0);
  vtkMultiBlockDataSet *output = vtkMultiBlockDataSet::GetData(outputVector, 0);

  if (this->Indices.empty())
    {
    // Trivial case
    output->ShallowCopy(input);
    return 1;
    }

  output->CopyStructure(input);

  this->ActiveIndices = this->Indices;

  // Copy selected blocks over to the output.
  vtkDataObjectTreeIterator* iter = input->NewTreeIterator();

  iter->VisitOnlyLeavesOff();
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal() && this->ActiveIndices.size() > 0)
    {
    const unsigned int index = iter->GetCurrentFlatIndex();
    if (this->ActiveIndices.find(index) != this->ActiveIndices.end())
      {
      this->ActiveIndices.erase(index);

      // This removed the visited indices from this->ActiveIndices.
      this->CreatePeriodicSubTree(iter, output, input);
      }
    iter->GoToNextItem();
    }
  iter->Delete();

  // Now shallow copy leaves from the input that were not selected
  // Note: this is OK to share iterator between input and output here
  iter = output->NewTreeIterator();
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOff();
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal())
    {
    if (!output->GetDataSet(iter))
      {
      vtkDataObject* inputLeaf = input->GetDataSet(iter);
      if (inputLeaf)
        {
        vtkDataObject* newLeaf = inputLeaf->NewInstance();
        newLeaf->ShallowCopy(inputLeaf);
        output->SetDataSet(iter, newLeaf);
        newLeaf->Delete();
        }
      }
    iter->GoToNextItem();
    }
  iter->Delete();

  this->ActiveIndices.clear();

  return 1;
}

//----------------------------------------------------------------------------
void vtkPeriodicFilter::GeneratePieceName(vtkCompositeDataSet* input,
  vtkCompositeDataIterator* inputLoc, vtkMultiPieceDataSet* output, vtkIdType outputId)
{
  vtkDataObjectTree* inputTree = vtkDataObjectTree::SafeDownCast(input);
  if (!inputTree)
    {
    return;
    }
  std::ostringstream ss;
  const char* parentName =
    inputTree->GetMetaData(inputLoc)->Get(vtkCompositeDataSet::NAME());
  if (parentName)
    {
    ss << parentName;
    }
  else
    {
    ss << "Piece";
    }
  ss << "_period" << outputId;
  output->GetMetaData(outputId)->Set(vtkCompositeDataSet::NAME(), ss.str().c_str());
}
