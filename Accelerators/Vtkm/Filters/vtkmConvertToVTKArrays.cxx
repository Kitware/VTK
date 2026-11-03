// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkmConvertToVTKArrays.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkFieldData.h"
#include "vtkInformationVector.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkSmartPointer.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmDataArray.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN

vtkStandardNewMacro(vtkmConvertToVTKArrays);

namespace
{
//------------------------------------------------------------------------------
// An ordinary VTK copy of @a array if it is a vtkmDataArray, otherwise null.
vtkSmartPointer<vtkDataArray> NewVTKArray(vtkAbstractArray* array)
{
  auto* data = vtkDataArray::SafeDownCast(array);
  if (!data || data->GetArrayType() != vtkArrayTypes::VTKM_DATA_ARRAY)
  {
    return nullptr;
  }

  vtkSmartPointer<vtkDataArray> converted;
  switch (data->GetDataType())
  {
    vtkTemplateMacro(if (auto* typed = vtkmDataArray<VTK_TT>::SafeDownCast(data)) {
      converted.TakeReference(typed->NewVTKArray());
    });
  }
  if (converted)
  {
    converted->SetName(data->GetName());
    converted->CopyComponentNames(data);
  }
  return converted;
}

//------------------------------------------------------------------------------
// The array to use in place of @a array: its copy if it is a vtkmDataArray,
// otherwise itself.
vtkSmartPointer<vtkDataArray> Replacement(vtkDataArray* array)
{
  auto converted = NewVTKArray(array);
  return converted ? converted : vtkSmartPointer<vtkDataArray>(array);
}

//------------------------------------------------------------------------------
void ConvertFieldData(vtkFieldData* fields)
{
  auto* attributes = vtkDataSetAttributes::SafeDownCast(fields);

  // Replacing an array can move others, so walk a snapshot.
  std::vector<vtkAbstractArray*> arrays;
  for (int i = 0; i < fields->GetNumberOfArrays(); ++i)
  {
    arrays.push_back(fields->GetAbstractArray(i));
  }

  for (vtkAbstractArray* array : arrays)
  {
    auto converted = NewVTKArray(array);
    if (!converted)
    {
      continue;
    }
    if (array->GetName())
    {
      // Replaces the array of the same name in place, so any attribute that
      // points at it now points at the copy.
      fields->AddArray(converted);
      continue;
    }

    // An unnamed array cannot be replaced by name.
    int index = 0;
    while (fields->GetAbstractArray(index) != array)
    {
      ++index;
    }
    const int attribute = attributes ? attributes->IsArrayAnAttribute(index) : -1;
    if (attribute >= 0)
    {
      attributes->SetAttribute(converted, attribute);
    }
    else
    {
      fields->RemoveArray(index);
      fields->AddArray(converted);
    }
  }
}

//------------------------------------------------------------------------------
// A cell array over copies of @a cells' arrays, or null when it has no
// vtkmDataArray to replace.
vtkSmartPointer<vtkCellArray> ConvertCells(vtkCellArray* cells)
{
  if (!cells)
  {
    return nullptr;
  }
  vtkDataArray* offsets = cells->GetOffsetsArray();
  vtkDataArray* connectivity = cells->GetConnectivityArray();
  auto newOffsets = Replacement(offsets);
  auto newConnectivity = Replacement(connectivity);
  if (newOffsets == offsets && newConnectivity == connectivity)
  {
    return nullptr;
  }
  // A new cell array rather than SetData() on this one: after the shallow
  // copy, this one is still the input's.
  vtkNew<vtkCellArray> converted;
  converted->SetData(newOffsets.Get(), newConnectivity.Get());
  return converted;
}

//------------------------------------------------------------------------------
void ConvertPolyData(vtkPolyData* output)
{
  if (auto verts = ConvertCells(output->GetVerts()))
  {
    output->SetVerts(verts);
  }
  if (auto lines = ConvertCells(output->GetLines()))
  {
    output->SetLines(lines);
  }
  if (auto polys = ConvertCells(output->GetPolys()))
  {
    output->SetPolys(polys);
  }
  if (auto strips = ConvertCells(output->GetStrips()))
  {
    output->SetStrips(strips);
  }
}

//------------------------------------------------------------------------------
void ConvertUnstructuredGrid(vtkUnstructuredGrid* output)
{
  vtkCellArray* cells = output->GetCells();
  vtkDataArray* types = output->GetCellTypes();
  if (!cells || !types)
  {
    return;
  }
  auto newCells = ConvertCells(cells);
  auto newTypes = NewVTKArray(types);
  if (!newCells && !newTypes)
  {
    return;
  }
  vtkCellArray* useCells = newCells ? newCells.Get() : cells;
  vtkDataArray* useTypes = newTypes ? newTypes.Get() : types;
  // The polyhedra are passed as they are, so faces shared between cells stay shared.
  if (vtkPolyhedronCellArray* polyhedra = output->GetPolyhedronCellArray())
  {
    output->SetPolyhedralCells(useTypes, useCells, polyhedra);
  }
  else
  {
    output->SetCells(useTypes, useCells);
  }
}

//------------------------------------------------------------------------------
void ConvertRectilinearGrid(vtkRectilinearGrid* output)
{
  if (auto x = NewVTKArray(output->GetXCoordinates()))
  {
    output->SetXCoordinates(x);
  }
  if (auto y = NewVTKArray(output->GetYCoordinates()))
  {
    output->SetYCoordinates(y);
  }
  if (auto z = NewVTKArray(output->GetZCoordinates()))
  {
    output->SetZCoordinates(z);
  }
}
} // anonymous namespace

//------------------------------------------------------------------------------
vtkmConvertToVTKArrays::vtkmConvertToVTKArrays() = default;

//------------------------------------------------------------------------------
vtkmConvertToVTKArrays::~vtkmConvertToVTKArrays() = default;

//------------------------------------------------------------------------------
void vtkmConvertToVTKArrays::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkmConvertToVTKArrays::RequestData(
  vtkInformation*, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDataSet* input = vtkDataSet::GetData(inputVector[0]);
  vtkDataSet* output = vtkDataSet::GetData(outputVector);
  output->ShallowCopy(input);

  ConvertFieldData(output->GetPointData());
  ConvertFieldData(output->GetCellData());
  ConvertFieldData(output->GetFieldData());

  if (auto* pointSet = vtkPointSet::SafeDownCast(output))
  {
    vtkPoints* points = pointSet->GetPoints();
    if (auto data = points ? NewVTKArray(points->GetData()) : nullptr)
    {
      vtkNew<vtkPoints> converted;
      converted->SetData(data);
      pointSet->SetPoints(converted);
    }
  }
  if (auto* polyData = vtkPolyData::SafeDownCast(output))
  {
    ConvertPolyData(polyData);
  }
  else if (auto* grid = vtkUnstructuredGrid::SafeDownCast(output))
  {
    ConvertUnstructuredGrid(grid);
  }
  else if (auto* rectilinear = vtkRectilinearGrid::SafeDownCast(output))
  {
    ConvertRectilinearGrid(rectilinear);
  }
  return 1;
}

VTK_ABI_NAMESPACE_END
