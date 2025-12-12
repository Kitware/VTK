// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridSampleQuery.h"

#include "vtkDoubleArray.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkTypeUInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridSampleQuery);

void vtkCellGridSampleQuery::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Output: " << this->Output << "\n";
  os << indent << "IncludeSourceCellInfo: " << (this->IncludeSourceCellInfo ? "T" : "F") << "\n";
  os << indent << "IncludeSourceCellSite: " << (this->IncludeSourceCellSite ? "T" : "F") << "\n";
  os << indent << "MaximumParametricDimension: " << this->MaximumParametricDimension << "\n";
  os << indent << "OutputOffsets: " << this->OutputOffsets.size() << "\n";
  os << indent << "AttributeMap: " << this->AttributeMap.size() << "\n";
  os << indent << "SourceCellType: " << this->SourceCellType << "\n";
  os << indent << "SourceCellId: " << this->SourceCellId << "\n";
  os << indent << "SourceCellSite: " << this->SourceCellSite << "\n";
}

bool vtkCellGridSampleQuery::Initialize()
{
  this->Superclass::Initialize();

  this->OutputOffsets.clear();
  this->AttributeMap.clear();
  this->MaximumParametricDimension = 0;
  this->SourceCellId = nullptr;
  this->SourceCellType = nullptr;
  this->SourceCellSite = nullptr;
  if (!this->Input || !this->Output)
  {
    return false;
  }

  if (this->IncludeSourceCellInfo)
  {
    this->SourceCellType =
      vtkTypeUInt32Array::SafeDownCast(this->Output->GetColumnByName("SourceCellType"));
    if (!this->SourceCellType)
    {
      this->SourceCellType = vtkSmartPointer<vtkTypeUInt32Array>::New();
      this->SourceCellType->SetName("SourceCellType");
      this->Output->AddColumn(this->SourceCellType);
    }

    this->SourceCellId =
      vtkIdTypeArray::SafeDownCast(this->Output->GetColumnByName("vtkOriginalCellIds"));
    if (!this->SourceCellId)
    {
      this->SourceCellId = vtkSmartPointer<vtkIdTypeArray>::New();
      this->SourceCellType->SetName("vtkOriginalCellIds");
      this->Output->AddColumn(this->SourceCellId);
    }
  }

  if (this->IncludeSourceCellSite)
  {
    this->SourceCellSite =
      vtkDataArray::SafeDownCast(this->Output->GetColumnByName("SourceCellSite"));
    if (!this->SourceCellSite)
    {
      this->SourceCellSite = vtkSmartPointer<vtkDoubleArray>::New();
      this->SourceCellSite->SetName("SourceCellSite");
      this->Output->AddColumn(this->SourceCellSite);
    }
  }

  for (const auto& inputAtt : this->Input->GetCellAttributeList())
  {
    vtkNew<vtkDoubleArray> outputCol;
    outputCol->SetNumberOfComponents(inputAtt->GetNumberOfComponents());
    outputCol->SetName(inputAtt->GetName().Data().c_str());
    this->Output->AddColumn(outputCol);
    this->AttributeMap[inputAtt] = outputCol;
  }

  return true;
}

void vtkCellGridSampleQuery::StartPass()
{
  this->Superclass::StartPass();
  if (this->Pass == PassType::GenerateOutputs)
  {
    // Turn OutputOffsets into a cumulative sum.
    vtkIdType next = 0;
    for (auto& entry : this->OutputOffsets)
    {
      vtkIdType curr = next;
      next += entry.second;
      entry.second = curr;
    }
    // Before we allocate table rows, make sure the cell-site array
    // has the proper number of components (or remove it if unused).
    if (this->MaximumParametricDimension)
    {
      if (this->SourceCellSite)
      {
        this->SourceCellSite->SetNumberOfComponents(this->MaximumParametricDimension);
      }
    }
    else
    {
      if (this->SourceCellSite)
      {
        this->Output->RemoveColumnByName("SourceCellSite");
        this->SourceCellSite = nullptr;
      }
    }
    // Allocate table rows.
    this->Output->SetNumberOfRows(next);
    // If we are storing source cell types, populate the array
    // now because it is allocated and we know the offsets.
    if (this->SourceCellType)
    {
      vtkStringToken hash;
      vtkIdType prev = 0;
      for (const auto& entry : this->OutputOffsets)
      {
        if (hash.IsValid() && prev != entry.second)
        {
          vtkSMPTools::For(prev, entry.second,
            [hash, this](vtkIdType begin, vtkIdType end)
            {
              for (vtkIdType ii = begin; ii < end; ++ii)
              {
                this->SourceCellType->SetValue(ii, hash.GetId());
              }
            });
        }
        prev = entry.second;
        hash = entry.first;
      }
      if (prev != next)
      {
        vtkSMPTools::For(prev, next,
          [hash, this](vtkIdType begin, vtkIdType end)
          {
            for (vtkIdType ii = begin; ii < end; ++ii)
            {
              this->SourceCellType->SetValue(ii, hash.GetId());
            }
          });
      }
    }
  }
}

void vtkCellGridSampleQuery::AddOutputSamples(
  vtkStringToken inputCellType, vtkIdType numberOfOutputs)
{
  this->OutputOffsets[inputCellType] = numberOfOutputs;
}

vtkIdType vtkCellGridSampleQuery::GetSampleOffset(vtkStringToken inputCellType)
{
  auto it = this->OutputOffsets.find(inputCellType.GetId());
  if (it == this->OutputOffsets.end())
  {
    return -1;
  }
  return it->second;
}

vtkDataArray* vtkCellGridSampleQuery::GetOutputAttributeColumn(vtkCellAttribute* inputAttribute)
{
  if (!inputAttribute)
  {
    return nullptr;
  }

  auto nit = this->AttributeMap.find(inputAttribute);
  if (nit == this->AttributeMap.end())
  {
    return nullptr;
  }
  return nit->second;
}

VTK_ABI_NAMESPACE_END
