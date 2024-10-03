// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellGridToUnstructuredGrid.h"

#include "vtkCellArray.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkIdTypeArray.h"
#include "vtkIncrementalOctreePointLocator.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPTools.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"

#include <sstream>

VTK_ABI_NAMESPACE_BEGIN

using namespace vtk::literals;

vtkStandardNewMacro(vtkCellGridToUnstructuredGrid);
vtkStandardNewMacro(vtkCellGridToUnstructuredGrid::Query);

void vtkCellGridToUnstructuredGrid::Query::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Output: " << this->Output << "\n";
  os << indent << "OutputOffsets: " << this->OutputOffsets.size() << " output cell types\n";
  vtkIndent i2 = indent.GetNextIndent();
  for (const auto& entry : this->OutputOffsets)
  {
    os << i2 << entry.first.Data() << " to cell type " << entry.second.CellType << ".\n";
  }
  os << indent << "AttributeMap: " << this->AttributeMap.size() << " entries\n";
}

bool vtkCellGridToUnstructuredGrid::Query::Initialize()
{
  this->Superclass::Initialize();
  this->OutputOffsets.clear();
  this->AttributeMap.clear();
  this->ConnectivityCount.clear();
  this->ConnectivityTransforms.clear();
  if (!this->Input || !this->Output)
  {
    vtkErrorMacro("Input or output grid is null.");
    return false;
  }

  // Create a new vtkPoints and initialize the point locator.
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> ugcells;
  vtkNew<vtkUnsignedCharArray> ugtypes;
  std::array<double, 6> bounds;
  points->SetDataTypeToDouble();
  this->Input->GetBounds(bounds.data());
  this->Output->SetPoints(points);
  this->Output->SetCells(ugtypes, ugcells);
  this->Locator->SetDataSet(this->Output);
  this->Locator->InitPointInsertion(points.GetPointer(), bounds.data());
  this->ConnectivityCount.clear();
  this->ConnectivityWeights.clear();

  this->AttributeMap[this->Input->GetShapeAttribute()] = points->GetData();

  for (const auto& inputAtt : this->Input->GetCellAttributeList())
  {
    if (this->Input->GetShapeAttribute() == inputAtt)
    {
      continue;
    }

    vtkNew<vtkDoubleArray> outputArr;
    outputArr->SetName(inputAtt->GetName().Data().c_str());
    outputArr->SetNumberOfComponents(inputAtt->GetNumberOfComponents());
    // Note that we do not allocate memory yet.
    this->Output->GetPointData()->AddArray(outputArr);
    this->AttributeMap[inputAtt] = outputArr;
  }
  return true;
}

void vtkCellGridToUnstructuredGrid::Query::StartPass()
{
  this->Superclass::StartPass();
  switch (this->GetPass())
  {
    case PassType::CountOutputs:
      // Do nothing.
      break;
    case PassType::GenerateConnectivity:
    {
      // Allocate storage for cells.
      auto* cellTypes = this->Output->GetCellTypesArray();
      auto* cellArray = this->Output->GetCells();
      vtkIdType totalCellCount = 0;
      vtkIdType totalConnCount = 0;
      for (auto& entry : this->OutputOffsets)
      {
        entry.second.CellOffset = totalCellCount;
        entry.second.ConnOffset = totalConnCount;
        totalCellCount += entry.second.NumberOfCells;
        totalConnCount += entry.second.NumberOfConnectivityEntries;
      }
      cellTypes->Allocate(totalCellCount);
      cellArray->AllocateExact(totalCellCount, totalConnCount);
    }
    break;
    case PassType::GeneratePointData:
    {
      auto* pointData = this->Output->GetPointData();
      auto numArrays = pointData->GetNumberOfArrays();
      vtkIdType nn = this->Output->GetPoints()->GetNumberOfPoints();
      for (int ii = 0; ii < numArrays; ++ii)
      {
        auto* array = pointData->GetArray(ii);
        int nc = array->GetNumberOfComponents();
        array->SetNumberOfTuples(nn);
        for (int jj = 0; jj < nc; ++jj)
        {
          array->FillComponent(jj, 0.);
        }
      }
      if (!this->ConnectivityCount.empty())
      {
        // Invert the connectivity counts into weights:
        vtkIdType np = this->ConnectivityCount.rbegin()->first + 1;
        this->ConnectivityWeights.resize(np, 0.);
        vtkSMPTools::For(0, np,
          [this](vtkIdType begin, vtkIdType end)
          {
            for (vtkIdType ii = begin; ii < end; ++ii)
            {
              auto it = this->ConnectivityCount.find(ii);
              this->ConnectivityWeights[ii] =
                (it == this->ConnectivityCount.end() ? 1.0 : 1.0 / it->second);
            }
          });
        this->ConnectivityCount.clear();
      }
    }
    break;
    default:
      vtkErrorMacro("Unknown pass " << this->GetPass());
      break;
  }
}

vtkDataArray* vtkCellGridToUnstructuredGrid::Query::GetOutputArray(vtkCellAttribute* inputAttribute)
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

bool vtkCellGridToUnstructuredGrid::Query::Finalize()
{
  return true;
}

void vtkCellGridToUnstructuredGrid::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Query:\n";
  vtkIndent i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
}

int vtkCellGridToUnstructuredGrid::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port == 0)
  {
    info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
    return 1;
  }
  return this->Superclass::FillInputPortInformation(port, info);
}

int vtkCellGridToUnstructuredGrid::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* input = vtkCellGrid::GetData(inInfo[0]);
  auto* output = vtkUnstructuredGrid::GetData(ouInfo);
  if (!input)
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!output)
  {
    vtkErrorMacro("Empty output.");
    return 0;
  }

  output->Initialize();
  this->Request->Input = input;
  this->Request->Output = output;
  // Run the cell-center query on the request.
  if (!input->Query(this->Request))
  {
    vtkErrorMacro("Input failed to respond to query.");
    return 0;
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
