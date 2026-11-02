// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Hide VTK_DEPRECATED_IN_9_8_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellGridPointProbe.h"

#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkCommand.h"
#include "vtkDataArraySelection.h"
#include "vtkDataSetAttributes.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"
#include "vtkTypeUInt32Array.h"
#include "vtkTypeUInt64Array.h"

#include <numeric>
#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridPointProbe);

vtkCellGridPointProbe::vtkCellGridPointProbe()
{
  this->SetNumberOfInputPorts(2);
  this->AttributeSelection->AddObserver(
    vtkCommand::ModifiedEvent, this, &vtkCellGridPointProbe::Modified);
}

vtkCellGridPointProbe::~vtkCellGridPointProbe() = default;

void vtkCellGridPointProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Request:\n";
  auto i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
  os << indent << "AttributeSelection:\n";
  this->AttributeSelection->PrintSelf(os, indent.GetNextIndent());
}

void vtkCellGridPointProbe::SetSourceConnection(vtkAlgorithmOutput* source)
{
  this->SetInputConnection(1, source);
}

void vtkCellGridPointProbe::SetAttributeName(const char* name)
{
  this->AttributeSelection->DisableAllArrays();
  if (name && name[0])
  {
    this->AttributeSelection->EnableArray(name);
  }
}

const char* vtkCellGridPointProbe::GetAttributeName()
{
  for (int ii = 0; ii < this->AttributeSelection->GetNumberOfArrays(); ++ii)
  {
    if (this->AttributeSelection->GetArraySetting(ii))
    {
      return this->AttributeSelection->GetArrayName(ii);
    }
  }
  return nullptr;
}

int vtkCellGridPointProbe::FillInputPortInformation(int port, vtkInformation* info)
{
  if (port != 1)
  {
    return this->Superclass::FillInputPortInformation(port, info);
  }

  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

int vtkCellGridPointProbe::RequestData(
  vtkInformation* vtkNotUsed(request), vtkInformationVector** inInfo, vtkInformationVector* ouInfo)
{
  auto* polydata = vtkPolyData::GetData(inInfo[0]);
  auto* cellgrid = vtkCellGrid::GetData(inInfo[1]);
  auto* polydest = vtkPolyData::GetData(ouInfo);
  if (!polydata || !polydata->GetPoints() || !polydata->GetPoints()->GetNumberOfPoints() ||
    !cellgrid || !cellgrid->GetNumberOfCells())
  {
    vtkWarningMacro("Empty input.");
    return 1;
  }
  if (!polydest)
  {
    vtkErrorMacro("Output not allocated.");
    return 0;
  }
  std::vector<vtkCellAttribute*> attributes;
  auto* shape = cellgrid->GetShapeAttribute();
  for (const auto& attribute : cellgrid->GetCellAttributeList())
  {
    if (attribute != shape &&
      this->AttributeSelection->ArrayIsEnabled(attribute->GetName().Data().c_str()))
    {
      attributes.push_back(attribute);
    }
  }

  // Without attributes, the points are only classified. A single attribute is
  // interpolated along with the classification. Several are each interpolated
  // afterwards from one classification.
  auto* inputPoints = polydata->GetPoints()->GetData();
  if (attributes.size() == 1)
  {
    this->Request->SetCellAttribute(attributes.front());
    this->Request->InterpolatePoints(inputPoints);
  }
  else
  {
    this->Request->ClassifyPoints(inputPoints);
  }

  if (!cellgrid->Query(this->Request))
  {
    vtkErrorMacro("Some input cells could not be probed.");
  }

  // Hold the classification, as interpolating releases some of it.
  auto* summaryCellType = this->Request->GetClassifierCellTypes();
  auto* summaryCellOffsets = this->Request->GetClassifierCellOffsets();
  auto* inputPointIds = this->Request->GetClassifierPointIDs();
  auto* outputCellIds = this->Request->GetClassifierCellIndices();
  auto* outputCellParams = this->Request->GetClassifierPointParameters();

  vtkIdType numCells = outputCellIds->GetNumberOfTuples();
  // create points
  vtkNew<vtkPoints> pts;
  pts->SetDataType(inputPoints->GetDataType());
  pts->SetNumberOfPoints(numCells);
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      double point[3];
      auto inputPointsRange = vtk::DataArrayValueRange<1>(inputPointIds);
      for (vtkIdType ii = begin; ii < end; ++ii)
      {
        inputPoints->GetTuple(inputPointsRange[ii], point);
        pts->SetPoint(ii, point);
      }
    });
  // create vertices
  vtkNew<vtkCellArray> vertices;
  vertices->UseFixedSizeDefaultStorage(1);
  vertices->ResizeExact(numCells, numCells);
  auto connectivity = vertices->GetConnectivityArray();
  vtkSMPTools::For(0, numCells,
    [&](vtkIdType begin, vtkIdType end)
    {
      auto connRange = vtk::DataArrayValueRange<1, vtkIdType>(connectivity, begin, end);
      std::iota(connRange.begin(), connRange.end(), begin);
    });
  // create cell types
  vtkNew<vtkTypeUInt32Array> outputCellType;
  outputCellType->SetNumberOfTuples(numCells);
  outputCellType->SetName("CellType");
  // Each cell type's points are contiguous; the last summary entry only marks
  // the end of the others.
  const vtkIdType numCellTypes = summaryCellType->GetNumberOfTuples() - 1;
  for (vtkIdType sct = 0; sct < numCellTypes; ++sct)
  {
    vtkSMPTools::Fill(outputCellType->GetPointer(summaryCellOffsets->GetValue(sct)),
      outputCellType->GetPointer(summaryCellOffsets->GetValue(sct + 1)),
      summaryCellType->GetValue(sct));
  }
  // store everything in output
  polydest->Initialize();
  polydest->SetPoints(pts);
  polydest->SetVerts(vertices);
  auto* pd = polydest->GetPointData();
  pd->AddArray(outputCellType);
  pd->AddArray(inputPointIds);
  pd->AddArray(outputCellIds);
  pd->AddArray(outputCellParams);
  if (attributes.empty())
  {
    return 1;
  }
  if (attributes.size() == 1)
  {
    auto* outputValues = this->Request->GetInterpolatedValues();
    pd->SetScalars(outputValues);
    return 1;
  }
  for (auto* attribute : attributes)
  {
    // A new array per attribute, as the evaluator would otherwise overwrite
    // the values of the previous one.
    vtkNew<vtkDoubleArray> outputValues;
    this->Request->SetInterpolatedValues(outputValues);
    this->Request->SetCellAttribute(attribute);
    this->Request->InterpolateCellParameters(
      summaryCellType, summaryCellOffsets, outputCellIds, outputCellParams);
    if (!cellgrid->Query(this->Request))
    {
      vtkErrorMacro(
        "Cell-attribute \"" << attribute->GetName().Data() << "\" could not be interpolated.");
      continue;
    }
    pd->AddArray(outputValues);
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
