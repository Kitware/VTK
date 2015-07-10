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

  // Copy selected blocks over to the output.
  vtkDataObjectTreeIterator* iter = input->NewTreeIterator();

  // Generate leaf multipieces
  iter->VisitOnlyLeavesOn();
  iter->SkipEmptyNodesOff();
  iter->InitTraversal();
  while (!iter->IsDoneWithTraversal() && this->Indices.size() > 0)
    {
    const unsigned int index = iter->GetCurrentFlatIndex();
    if (this->Indices.find(index) != this->Indices.end())
      {
      this->CreatePeriodicDataSet(iter, output, input);
      }
    else
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
