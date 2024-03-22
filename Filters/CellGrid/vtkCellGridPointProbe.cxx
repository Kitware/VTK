// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellGridPointProbe.h"

#include "vtkCellArray.h"
#include "vtkCellAttribute.h"
#include "vtkCellGrid.h"
#include "vtkCellGridEvaluator.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTypeUInt32Array.h"

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkCellGridPointProbe);

vtkCellGridPointProbe::vtkCellGridPointProbe()
{
  this->SetNumberOfInputPorts(2);
}

vtkCellGridPointProbe::~vtkCellGridPointProbe()
{
  this->SetAttributeName(nullptr);
}

void vtkCellGridPointProbe::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Request:\n";
  auto i2 = indent.GetNextIndent();
  this->Request->PrintSelf(os, i2);
  os << indent << "AttributeName: "
     << (this->AttributeName && this->AttributeName[0] ? this->AttributeName : "(null)") << "\n";
}

void vtkCellGridPointProbe::SetSourceConnection(vtkAlgorithmOutput* source)
{
  this->SetInputConnection(1, source);
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
  bool interpolate = true;
  if (!this->AttributeName || !this->AttributeName[0])
  {
    interpolate = false;
    vtkDebugMacro("No attribute name specified. Skipping interpolation.");
  }
  else
  {
    auto* attribute = cellgrid->GetCellAttributeByName(this->AttributeName);
    if (attribute)
    {
      this->Request->SetCellAttribute(attribute);
    }
    else
    {
      vtkErrorMacro("No cell-attribute \"" << this->AttributeName << "\" exists.\n");
      interpolate = false;
    }
  }

  auto* inputPoints = polydata->GetPoints()->GetData();
  if (interpolate)
  {
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

  auto* summaryCellType = this->Request->GetClassifierCellTypes();
  auto* summaryCellOffsets = this->Request->GetClassifierCellOffsets();
  auto* inputPointIds = this->Request->GetClassifierPointIDs();
  auto* outputCellIds = this->Request->GetClassifierCellIndices();
  auto* outputCellParams = this->Request->GetClassifierPointParameters();
  auto* outputValues = this->Request->GetInterpolatedValues();

  polydest->Initialize();
  vtkNew<vtkPoints> pts;
  vtkNew<vtkCellArray> vrt;
  vtkNew<vtkTypeUInt32Array> outputCellType;
  vtkIdType nn = outputCellIds->GetNumberOfTuples();
  pts->SetNumberOfPoints(nn);
  vrt->AllocateExact(nn, nn);
  outputCellType->SetNumberOfTuples(nn);
  outputCellType->SetName("CellType");
  vtkIdType sct = 0; // Where in summary{CellType/CellOffsets} are we?
  // Fill in arrays with point locations and the type of cell holding each point.
  for (vtkIdType ii = 0; ii < nn; ++ii)
  {
    pts->SetPoint(ii, inputPoints->GetTuple(inputPointIds->GetValue(ii)));
    vrt->InsertNextCell(1, &ii);
    if (ii >= static_cast<vtkIdType>(summaryCellOffsets->GetValue(sct)))
    {
      ++sct;
    }
    outputCellType->SetValue(ii, summaryCellType->GetValue(summaryCellOffsets->GetValue(sct)));
  }
  polydest->SetPoints(pts);
  polydest->SetVerts(vrt);
  auto* pd = polydest->GetPointData();
  pd->AddArray(outputCellType);
  pd->AddArray(outputCellIds);
  pd->AddArray(outputCellParams);
  if (interpolate)
  {
    pd->AddArray(outputValues);
    pd->SetScalars(outputValues);
  }

  return 1;
}

VTK_ABI_NAMESPACE_END
