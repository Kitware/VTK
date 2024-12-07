// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkHyperTreeGridEvaluateCoarse.h"
#include "vtkBitArray.h"
#include "vtkCellData.h"
#include "vtkHyperTree.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkThreadedTaskQueue.h"

#include "vtkUniformHyperTreeGrid.h"

#include "vtkHyperTreeGridNonOrientedCursor.h"

#include <cmath>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkHyperTreeGridEvaluateCoarse);

//------------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::vtkHyperTreeGridEvaluateCoarse()
{
  // In order to output a mesh of the same type as that given as input
  this->AppropriateOutput = true;
}

//------------------------------------------------------------------------------
vtkHyperTreeGridEvaluateCoarse::~vtkHyperTreeGridEvaluateCoarse() = default;

//------------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::FillOutputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkDataObject::DATA_TYPE_NAME(), "vtkHyperTreeGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkHyperTreeGridEvaluateCoarse::ProcessTrees(vtkHyperTreeGrid* input, vtkDataObject* outputDO)
{
  // Downcast output data object to HyperTreeGrid
  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(outputDO);
  if (!output)
  {
    vtkErrorMacro("Incorrect type of output: " << outputDO->GetClassName());
    return 0;
  }

  output->ShallowCopy(input);

  // OPERATOR_DON_T_CHANGE_FAST is a no-op
  if (this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE_FAST)
  {
    return 1;
  }

  this->Mask = output->HasMask() ? output->GetMask() : nullptr;

  this->SplattingFactor = std::pow(output->GetBranchFactor(), output->GetDimension() - 1);
  this->NumberOfChildren = output->GetNumberOfChildren();
  this->InData = input->GetCellData();
  this->OutData = output->GetCellData();
  this->OutData->CopyAllocate(this->InData);

  // Iterate over all input and output hyper trees
  vtkIdType index;
  vtkHyperTreeGrid::vtkHyperTreeGridIterator in;
  output->InitializeTreeIterator(in);
  vtkNew<vtkHyperTreeGridNonOrientedCursor> outCursor;
  while (in.GetNextTree(index))
  {
    if (this->CheckAbort())
    {
      break;
    }

    // Initialize new cursor at root of current output tree
    output->InitializeNonOrientedCursor(outCursor, index);

    // Process tree recursively
    if (this->Operator == vtkHyperTreeGridEvaluateCoarse::OPERATOR_DON_T_CHANGE)
    {
      this->ProcessNodeNoChange(outCursor);
    }
    else
    {
      this->ProcessNode(outCursor);
    }
  }

  this->UpdateProgress(1.);
  return 1;
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::ProcessNodeNoChange(
  vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  vtkIdType id = outCursor->GetGlobalNodeIndex();
  this->OutData->CopyData(this->InData, id, id);

  if (outCursor->IsLeaf() || outCursor->IsMasked())
  {
    return;
  }

  // Coarse
  for (unsigned int ichild = 0; ichild < this->NumberOfChildren; ++ichild)
  {
    if (this->CheckAbort())
    {
      break;
    }
    outCursor->ToChild(ichild);
    // We go through the children's cells
    this->ProcessNodeNoChange(outCursor);
    outCursor->ToParent();
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::ProcessNode(vtkHyperTreeGridNonOrientedCursor* outCursor)
{
  if (this->CheckAbort())
  {
    return;
  }

  vtkIdType currentId = outCursor->GetGlobalNodeIndex();

  // Leaf/Masked cell: data does not change
  if (outCursor->IsLeaf() || outCursor->IsMasked())
  {
    this->OutData->CopyData(this->InData, currentId, currentId);
    return;
  }

  // Coarse cell: recurse and retrieve values
  int nbArray = this->InData->GetNumberOfArrays();
  std::vector<std::vector<std::vector<double>>> childrenValues(outCursor->GetNumberOfChildren());
  if (outCursor->GetLevel() <= 2)
  {
    // Create a new thread for every child, when we're not too deep into the tree
    vtkThreadedTaskQueue<void, int> queue(
      [this, outCursor, &childrenValues](int ichild)
      {
        vtkSmartPointer<vtkHyperTreeGridNonOrientedCursor> childCursor =
          vtk::TakeSmartPointer(outCursor->CloneFromCurrentEntry());
        this->ProcessChild(childCursor, ichild, childrenValues[ichild]);
      },
      false);

    for (unsigned char ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      queue.Push(static_cast<int>(ichild));
    }
    queue.Flush();
  }
  else
  {
    // Otherwise, process the child serially
    for (int ichild = 0; ichild < outCursor->GetNumberOfChildren(); ++ichild)
    {
      this->ProcessChild(outCursor, ichild, childrenValues[ichild]);
    }
  }

  // Reduction operation over the resulting array
  for (int arrayId = 0; arrayId < nbArray; ++arrayId)
  {
    if (this->CheckAbort())
    {
      break;
    }
    vtkDataArray* arr = this->OutData->GetArray(arrayId);
    int nbComponents = arr->GetNumberOfComponents();
    for (int componentID = 0; componentID < nbComponents; ++componentID)
    {
      // Copy child values from every child array
      std::vector<double> childVals(outCursor->GetNumberOfChildren(), 0.0);
      for (unsigned char ichild = 0; ichild < outCursor->GetNumberOfChildren(); ichild++)
      {
        childVals[ichild] = childrenValues[ichild][arrayId][componentID];
      }
      arr->SetComponent(currentId, componentID, EvalCoarse(childVals));
    }
  }
}

//------------------------------------------------------------------------------
void vtkHyperTreeGridEvaluateCoarse::ProcessChild(vtkHyperTreeGridNonOrientedCursor* outCursor,
  int ichild, std::vector<std::vector<double>>& childrenValues)
{
  assert("pre: has child ichild" && ichild < outCursor->GetNumberOfChildren());

  // Process child: this cell values depends on the child's values
  outCursor->ToChild(ichild);
  this->ProcessNode(outCursor);

  // Collect children values
  childrenValues.resize(this->InData->GetNumberOfArrays());
  vtkIdType idChild = outCursor->GetGlobalNodeIndex();
  for (int arrayId = 0; arrayId < static_cast<int>(childrenValues.size()); ++arrayId)
  {
    vtkDataArray* arr = this->OutData->GetArray(arrayId);
    int nbComponents = arr->GetNumberOfComponents();
    childrenValues[arrayId].resize(nbComponents);
    if (!this->Mask || !this->Mask->GetTuple1(idChild))
    {
      std::vector<double> tupleValue(nbComponents, 0.0);
      arr->GetTuple(idChild, tupleValue.data());
      for (int componentId = 0; componentId < nbComponents; ++componentId)
      {
        childrenValues[arrayId][componentId] = tupleValue[componentId];
      }
    }
  }
  outCursor->ToParent();
}

//------------------------------------------------------------------------------
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

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Min(const std::vector<double>& array)
{
  if (array.empty())
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

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Max(const std::vector<double>& array)
{
  if (array.empty())
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

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Sum(const std::vector<double>& array)
{
  double val = array[0];
  for (std::vector<double>::const_iterator it = array.begin() + 1; it != array.end(); ++it)
  {
    val += *it;
  }
  return val;
}

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::Average(const std::vector<double>& array)
{
  if (array.empty())
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

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::UnmaskedAverage(const std::vector<double>& array)
{
  if (array.empty())
  {
    return NAN;
  }
  return Sum(array) / array.size();
}

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::ElderChild(const std::vector<double>& array)
{
  if (array.empty())
  {
    return NAN;
  }
  return array[0];
}

//------------------------------------------------------------------------------
double vtkHyperTreeGridEvaluateCoarse::SplattingAverage(const std::vector<double>& array)
{
  if (array.empty())
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
VTK_ABI_NAMESPACE_END
