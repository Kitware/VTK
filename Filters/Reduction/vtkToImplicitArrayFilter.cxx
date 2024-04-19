// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkToImplicitArrayFilter.h"

#include "vtkCommand.h"
#include "vtkDataArray.h"
#include "vtkDataArraySelection.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"
#include "vtkToImplicitStrategy.h"

#include <algorithm>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN
//-------------------------------------------------------------------------
struct vtkToImplicitArrayFilter::vtkInternals
{
  vtkInternals()
    : Strategy(nullptr)
    , ArraySelections(vtkDataObject::NUMBER_OF_ASSOCIATIONS)
  {
    for (auto& sel : this->ArraySelections)
    {
      sel = vtkSmartPointer<vtkDataArraySelection>::New();
    }
  }

  vtkSmartPointer<vtkToImplicitStrategy> Strategy;
  std::vector<vtkSmartPointer<vtkDataArraySelection>> ArraySelections;
};

//-------------------------------------------------------------------------
vtkObjectFactoryNewMacro(vtkToImplicitArrayFilter);

//-------------------------------------------------------------------------
vtkToImplicitArrayFilter::vtkToImplicitArrayFilter()
  : Internals(std::unique_ptr<vtkInternals>(new vtkInternals()))
{
  for (auto& sel : this->Internals->ArraySelections)
  {
    sel->AddObserver(vtkCommand::ModifiedEvent, this, &vtkToImplicitArrayFilter::Modified);
  }
}

vtkToImplicitArrayFilter::~vtkToImplicitArrayFilter() = default;

//-------------------------------------------------------------------------
void vtkToImplicitArrayFilter::PrintSelf(std::ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent
     << (this->UseMaxNumberOfDegreesOfFreedom ? "MaxNumberOfDegreesOfFreedom: "
                                              : "TargetReduction: ")
     << (this->UseMaxNumberOfDegreesOfFreedom ? this->MaxNumberOfDegreesOfFreedom
                                              : this->TargetReduction)
     << "\n";
  os << indent << "Strategy:";
  if (this->Internals->Strategy)
  {
    os << "\n";
    this->Internals->Strategy->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "nullptr\n";
  }
  os << indent << "PointDataArraySelection: \n";
  this->GetPointDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "CellDataArraySelection: \n";
  this->GetCellDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "FieldDataArraySelection: \n";
  this->GetFieldDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "PointsThenCellsDataArraySelection: \n";
  this->GetPointsThenCellsDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "VertexDataArraySelection: \n";
  this->GetVertexDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "EdgeDataArraySelection: \n";
  this->GetEdgeDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << indent << "RowDataArraySelection: \n";
  this->GetRowDataArraySelection()->PrintSelf(os, indent.GetNextIndent());
  os << std::flush;
}

//-------------------------------------------------------------------------
void vtkToImplicitArrayFilter::SetStrategy(vtkToImplicitStrategy* strat)
{
  if (strat && strat != this->Internals->Strategy)
  {
    if (this->Internals->Strategy)
    {
      this->Internals->Strategy->RemoveObserver(vtkCommand::ModifiedEvent);
    }
    this->Internals->Strategy = strat;
    if (this->Internals->Strategy)
    {
      this->Internals->Strategy->AddObserver(
        vtkCommand::ModifiedEvent, this, &vtkToImplicitArrayFilter::Modified);
    }
    this->Modified();
  }
}

//-------------------------------------------------------------------------
const vtkToImplicitStrategy* vtkToImplicitArrayFilter::GetStrategy() const
{
  return this->Internals->Strategy;
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetArraySelection(int association)
{
  if (association < 0 || association >= vtkDataObject::NUMBER_OF_ASSOCIATIONS)
  {
    vtkErrorMacro("Attempt to get an array selection that is out of bounds");
    return nullptr;
  }
  return this->Internals->ArraySelections[association];
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetPointDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_POINTS);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetCellDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_CELLS);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetFieldDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_NONE);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetPointsThenCellsDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_POINTS_THEN_CELLS);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetVertexDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_VERTICES);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetEdgeDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_EDGES);
}

//-------------------------------------------------------------------------
vtkDataArraySelection* vtkToImplicitArrayFilter::GetRowDataArraySelection()
{
  return this->GetArraySelection(vtkDataObject::FIELD_ASSOCIATION_ROWS);
}

//-------------------------------------------------------------------------
int vtkToImplicitArrayFilter::RequestData(vtkInformation* vtkNotUsed(info),
  vtkInformationVector** inputInfo, vtkInformationVector* outputInfo)
{
  auto input = vtkDataObject::GetData(inputInfo[0], 0);
  auto output = vtkDataObject::GetData(outputInfo, 0);
  output->ShallowCopy(input);

  if (!this->Internals->Strategy)
  {
    vtkWarningMacro("No strategy set in vtkToImplicitArrayFilter");
    return 1;
  }

  for (int association = 0; association < vtkDataObject::NUMBER_OF_ASSOCIATIONS; ++association)
  {
    this->UpdateProgress(association / vtkDataObject::NUMBER_OF_ASSOCIATIONS);

    if (this->CheckAbort())
    {
      break;
    }

    auto outFD = output->GetAttributesAsFieldData(association);
    auto outAttr = vtkDataSetAttributes::SafeDownCast(outFD);

    if (!outAttr)
    {
      continue;
    }

    auto select = this->GetArraySelection(association);
    if (!select)
    {
      vtkWarningMacro(
        "Selection is nullptr for an attribute type when it should not be for any association.");
      continue;
    }

    const int nArrays = outAttr->GetNumberOfArrays();
    double progress = association / vtkDataObject::NUMBER_OF_ASSOCIATIONS;
    double progressIncr = 1 / (vtkDataObject::NUMBER_OF_ASSOCIATIONS * (nArrays ? nArrays : 1));
    std::vector<int> arraysToRemove;
    std::vector<vtkSmartPointer<vtkDataArray>> arraysToAdd;
    for (int iArr = 0; iArr < nArrays; ++iArr)
    {
      vtkDataArray* arr = outAttr->GetArray(iArr);
      if (!arr)
      {
        continue;
      }
      if (!select->ArrayIsEnabled(arr->GetName()))
      {
        continue;
      }

      vtkToImplicitStrategy::Optional estimatedReduction =
        this->Internals->Strategy->EstimateReduction(arr);

      // check can reduce and that the reduction is sufficient
      if (!estimatedReduction.IsSome ||
        (this->UseMaxNumberOfDegreesOfFreedom ? this->MaxNumberOfDegreesOfFreedom <
              (estimatedReduction.Value * arr->GetNumberOfValues())
                                              : this->TargetReduction < estimatedReduction.Value))
      {
        this->Internals->Strategy->ClearCache();
        continue;
      }

      arraysToRemove.emplace_back(iArr);
      arraysToAdd.emplace_back(this->Internals->Strategy->Reduce(arr));

      this->Internals->Strategy->ClearCache();

      arraysToAdd.back()->SetName(arr->GetName());

      progress += progressIncr;
      this->UpdateProgress(progress);
    }
    std::for_each(arraysToRemove.rbegin(), arraysToRemove.rend(),
      [&outAttr](int iArr) { outAttr->RemoveArray(iArr); });
    std::for_each(arraysToAdd.begin(), arraysToAdd.end(),
      [&outAttr](vtkDataArray* arr) { outAttr->AddArray(arr); });
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
