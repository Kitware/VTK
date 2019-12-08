/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridEvaluateCoarse.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkHyperTreeGridEvaluateCoarse.h"
#include "vtkBitArray.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

#include "vtkUniformHyperTreeGrid.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <cmath>

vtkStandardNewMacro(vtkHyperTreeGridEvaluateCoarse);

//-----------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::vtkHyperTreeGridEvaluateCoarse()
{
  this->Operator = vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE;
  this->Mask = nullptr;

  this->Default = 0.;

  this->BranchFactor = 0;
  this->Dimension = 0;
  this->SplattingFactor = 1;

  // In order to output a mesh of the same type as that given as input
  this->AppropriateOutput = true;
}

//-----------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::~vtkHyperTreeGridEvaluateCoarse() {}

//----------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to hypertree grid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  output->ShallowCopy(input);

  if (this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE_FAST)
  {
    return 1;
  }

  this->Mask = output->HasMask() ? output->GetMask() : nullptr;

  this->BranchFactor = output->GetBranchFactor();
  this->Dimension = output->GetDimension();
  this->SplattingFactor = std::pow(this->BranchFactor, this->Dimension - 1);
  this->NumberOfChildren = output->GetNumberOfChildren();

  this->NbChilds = input->GetNumberOfChildren();
  this->InData = input->GetPointData();
  this->OutData = output->GetPointData();
  this->OutData->CopyAllocate(this->InData);
  // Iterate over all input and output hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator in;
  output->InitializeTreeIterator(in);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (in.GetNextTree(index))
  {
    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, index);
    // Recursively
    this->ProcessNode(outCursor);
    // Clean up
  }
  this->UpdateProgress(1.);
  return 1;
}

//----------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::ProcessNode(vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  vtkIdType id = outCursor->GetGlobalNodeIndex();
  if (outCursor->IsLeaf())
  {
    this->OutData->CopyData(this->InData, id, id);
    return;
  }
  // If not operation
  if (this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE)
  {
    this->OutData->CopyData(this->InData, id, id);
    // Coarse
    for (int ichild = 0; ichild < this->NbChilds; ++ichild)
    {
      outCursor->ToChild(ichild);
      // We go through the children's cells
      ProcessNode(outCursor);
      outCursor->ToParent();
    }
    return;
  }
  //
  int nbArray = this->InData->GetNumberOfArrays();
  //
  std::vector<std::vector<std::vector<double> > > values(nbArray);
  // Coarse
  for (int ichild = 0; ichild < this->NbChilds; ++ichild)
  {
    outCursor->ToChild(ichild);
    // Iterate children
    ProcessNode(outCursor);
    // Memorize children values
    vtkIdType idChild = outCursor->GetGlobalNodeIndex();
    for (int i = 0; i < nbArray; ++i)
    {
      vtkDataArray* arr = this->OutData->GetArray(i);
      int nbC = arr->GetNumberOfComponents();
      values[i].resize(nbC);
      if (!this->Mask || !this->Mask->GetTuple1(idChild))
      {
        double* tmp = arr->GetTuple(idChild);
        for (int iC = 0; iC < nbC; ++iC)
        {
          values[i][iC].push_back(tmp[iC]);
        }
      }
    }
    outCursor->ToParent();
  }
  // Reduction operation
  for (int i = 0; i < nbArray; ++i)
  {
    vtkDataArray* arr = this->OutData->GetArray(i);
    int nbC = arr->GetNumberOfComponents();
    for (int iC = 0; iC < nbC; ++iC)
    {
      arr->SetComponent(id, iC, EvalCoarse(values[i][iC]));
    }
    values[i].clear();
  }
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::EvalCoarse(const std::vector<double>& array)
{
  switch (this->Operator)
  {
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_ELDER_CHILD:
    {
      return this->ElderChild(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_MIN:
    {
      return this->Min(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_MAX:
    {
      return this->Max(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_SUM:
    {
      return this->Sum(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_AVERAGE:
    {
      return this->Average(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_UNMASKED_AVERAGE:
    {
      return this->UnmaskedAverage(array);
    }
    case vtkHyperTreeGridEvaluateCoarse::OPERATOR_SPLATTING_AVERAGE:
    {
      return this->SplattingAverage(array);
    }
    default:
    {
      break;
    }
  }
  return NAN;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Min(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    if (*it < val)
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Max(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    if (*it > val)
    {
      val = *it;
    }
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Sum(const std::vector<double>& array)
{
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    val += *it;
  }
  return val;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Average(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return this->Default;
  }
  double sum = Sum(array);
  if (this->Default != 0.)
  {
    sum += this->Default * (this->NumberOfChildren - array.size());
  }
  return sum / this->NumberOfChildren;
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::UnmaskedAverage(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  return Sum(array) / array.size();
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::ElderChild(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return NAN;
  }
  return array[0];
}

//----------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::SplattingAverage(const std::vector<double>& array)
{
  if (array.size() == 0)
  {
    return this->Default;
  }
  double sum = Sum(array);
  if (this->Default != 0.)
  {
    sum += this->Default * (this->NumberOfChildren - array.size());
  }
  return sum / this->SplattingFactor;
}
