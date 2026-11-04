// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// vtkmConvertToVTKArrays replaces every vtkmDataArray in its output with an
// ordinary VTK array holding the same values, leaves its input alone, and
// owns what it produces: the output stays valid after the input is gone.

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyhedronCellArray.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkStringFormatter.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmContour.h"
#include "vtkmConvertToVTKArrays.h"
#include "vtkmDataArray.h"
#include "vtkmPointElevation.h"
#include "vtkmThreshold.h"

#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <viskores/cont/ArrayHandleConstant.h>

namespace
{
bool IsVtkm(vtkDataArray* array)
{
  return array && array->GetArrayType() == vtkArrayTypes::VTKM_DATA_ARRAY;
}

std::vector<double> Values(vtkDataArray* array)
{
  std::vector<double> values;
  const int numComponents = array->GetNumberOfComponents();
  for (vtkIdType t = 0; t < array->GetNumberOfTuples(); ++t)
  {
    for (int c = 0; c < numComponents; ++c)
    {
      values.push_back(array->GetComponent(t, c));
    }
  }
  return values;
}

// Every array a dataset holds, by what it is.
std::vector<std::pair<std::string, vtkDataArray*>> Arrays(vtkDataSet* data)
{
  std::vector<std::pair<std::string, vtkDataArray*>> arrays;
  arrays.reserve(
    data->GetPointData()->GetNumberOfArrays() + data->GetCellData()->GetNumberOfArrays() + 4);
  for (int i = 0; i < data->GetPointData()->GetNumberOfArrays(); ++i)
  {
    arrays.emplace_back("point data " + vtk::to_string(i), data->GetPointData()->GetArray(i));
  }
  for (int i = 0; i < data->GetCellData()->GetNumberOfArrays(); ++i)
  {
    arrays.emplace_back("cell data " + vtk::to_string(i), data->GetCellData()->GetArray(i));
  }
  if (auto* pointSet = vtkPointSet::SafeDownCast(data))
  {
    arrays.emplace_back("points", pointSet->GetPoints()->GetData());
  }
  auto addCells = [&](const char* what, vtkCellArray* cells)
  {
    arrays.emplace_back(std::string(what) + " offsets", cells->GetOffsetsArray());
    arrays.emplace_back(std::string(what) + " connectivity", cells->GetConnectivityArray());
  };
  if (auto* polyData = vtkPolyData::SafeDownCast(data))
  {
    addCells("polys", polyData->GetPolys());
  }
  if (auto* grid = vtkUnstructuredGrid::SafeDownCast(data))
  {
    addCells("cells", grid->GetCells());
    arrays.emplace_back("cell types", grid->GetCellTypes());
  }
  return arrays;
}

// Check that @a converted has the arrays of @a original, as ordinary VTK
// arrays with the same values. Returns how many of the originals were
// vtkmDataArrays, or -1 on a mismatch.
int Compare(vtkDataSet* original, vtkDataSet* converted, const char* name)
{
  auto before = Arrays(original);
  auto after = Arrays(converted);
  if (before.size() != after.size())
  {
    std::cerr << name << ": expected " << before.size() << " arrays, got " << after.size() << "\n";
    return -1;
  }
  int wrapped = 0;
  for (std::size_t i = 0; i < before.size(); ++i)
  {
    const std::string& what = before[i].first;
    if (IsVtkm(after[i].second))
    {
      std::cerr << name << ": " << what << " is still a vtkmDataArray\n";
      return -1;
    }
    if (Values(before[i].second) != Values(after[i].second))
    {
      std::cerr << name << ": " << what << " changed value\n";
      return -1;
    }
    wrapped += IsVtkm(before[i].second);
  }
  return wrapped;
}

// Polyhedra are passed through as they are: the output holds the input's
// vtkPolyhedronCellArray itself, not one rebuilt from its faces, which would
// give every cell its own copy of a shared face.
bool TestPolyhedra()
{
  // Two unit cubes sharing the face x = 1.
  vtkNew<vtkPoints> points;
  for (int k = 0; k < 2; ++k)
  {
    for (int j = 0; j < 2; ++j)
    {
      for (int i = 0; i < 3; ++i)
      {
        points->InsertNextPoint(i, j, k);
      }
    }
  }
  auto id = [](int i, int j, int k) { return static_cast<vtkIdType>(i + 3 * (j + 2 * k)); };
  vtkNew<vtkUnstructuredGrid> grid;
  grid->SetPoints(points);
  for (int x = 0; x < 2; ++x)
  {
    const vtkIdType pts[8] = { id(x, 0, 0), id(x + 1, 0, 0), id(x + 1, 1, 0), id(x, 1, 0),
      id(x, 0, 1), id(x + 1, 0, 1), id(x + 1, 1, 1), id(x, 1, 1) };
    const vtkIdType faces[30] = { 4, pts[0], pts[3], pts[2], pts[1], 4, pts[4], pts[5], pts[6],
      pts[7], 4, pts[0], pts[1], pts[5], pts[4], 4, pts[3], pts[7], pts[6], pts[2], 4, pts[0],
      pts[4], pts[7], pts[3], 4, pts[1], pts[2], pts[6], pts[5] };
    grid->InsertNextCell(VTK_POLYHEDRON, 8, pts, 6, faces);
  }

  // Cell types held by Viskores, so the filter has something to convert.
  auto types = vtkSmartPointer<vtkmDataArray<unsigned char>>::New();
  types->SetVtkmArrayHandle(
    viskores::cont::make_ArrayHandleConstant(static_cast<viskores::UInt8>(VTK_POLYHEDRON), 2));
  vtkPolyhedronCellArray* polyhedra = grid->GetPolyhedronCellArray();
  grid->SetPolyhedralCells(types, grid->GetCells(), polyhedra);

  vtkNew<vtkmConvertToVTKArrays> convert;
  convert->SetInputData(grid);
  convert->Update();
  auto* output = vtkUnstructuredGrid::SafeDownCast(convert->GetOutput());

  bool ok = true;
  if (!output || IsVtkm(output->GetCellTypes()))
  {
    std::cerr << "polyhedra: cell types are still a vtkmDataArray\n";
    return false;
  }
  if (Values(output->GetCellTypes()) != std::vector<double>(2, VTK_POLYHEDRON))
  {
    std::cerr << "polyhedra: cell types changed value\n";
    ok = false;
  }
  if (output->GetPolyhedronCellArray() != polyhedra)
  {
    std::cerr << "polyhedra: the polyhedron cell array was rebuilt instead of passed as it is\n";
    ok = false;
  }
  if (output->GetNumberOfCells() != 2 || output->GetCell(1)->GetNumberOfFaces() != 6)
  {
    std::cerr << "polyhedra: the cells changed\n";
    ok = false;
  }
  return ok;
}
}

int TestVTKMConvertToVTKArrays(int, char*[])
{
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);

  vtkNew<vtkmPointElevation> elevation;
  elevation->SetInputConnection(wavelet->GetOutputPort());
  elevation->SetLowPoint(-10, -10, -10);
  elevation->SetHighPoint(10, 10, 10);
  elevation->ForceVTKmOn();

  vtkNew<vtkmContour> contour;
  contour->SetInputConnection(elevation->GetOutputPort());
  contour->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "elevation");
  contour->SetValue(0, 0.5);
  contour->ComputeScalarsOn();
  contour->ForceVTKmOn();

  vtkNew<vtkmThreshold> threshold;
  threshold->SetInputConnection(elevation->GetOutputPort());
  threshold->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "elevation");
  threshold->SetLowerThreshold(0.4);
  threshold->SetUpperThreshold(0.6);
  threshold->ForceVTKmOn();

  bool ok = true;
  int wrapped = 0;
  for (vtkAlgorithm* source :
    { static_cast<vtkAlgorithm*>(contour), static_cast<vtkAlgorithm*>(threshold) })
  {
    const char* name = source->GetClassName();
    source->Update();
    vtkDataSet* original = vtkDataSet::SafeDownCast(source->GetOutputDataObject(0));
    if (original->GetNumberOfCells() == 0)
    {
      std::cerr << name << ": output is empty\n";
      ok = false;
      continue;
    }

    // Which of the input's arrays are vtkmDataArrays, to show that the filter
    // does not replace them.
    std::vector<bool> wasVtkm;
    for (auto& array : Arrays(original))
    {
      wasVtkm.push_back(IsVtkm(array.second));
    }

    vtkNew<vtkmConvertToVTKArrays> convert;
    convert->SetInputConnection(source->GetOutputPort());
    convert->Update();
    vtkDataSet* converted = convert->GetOutput();

    const int count = Compare(original, converted, name);
    if (count < 0)
    {
      ok = false;
      continue;
    }
    wrapped += count;
    auto arrays = Arrays(original);
    for (std::size_t i = 0; i < arrays.size(); ++i)
    {
      if (IsVtkm(arrays[i].second) != wasVtkm[i])
      {
        std::cerr << name << ": input " << arrays[i].first << " was replaced\n";
        ok = false;
      }
    }

    vtkDataArray* scalars = converted->GetPointData()->GetScalars();
    if (!scalars || std::string(scalars->GetName()) != "elevation")
    {
      std::cerr << name << ": active scalars were lost\n";
      ok = false;
    }

    // The output owns its arrays: it keeps its values once the input is gone.
    vtkSmartPointer<vtkDataSet> kept = converted;
    std::vector<std::vector<double>> expected;
    for (auto& array : Arrays(kept))
    {
      expected.push_back(Values(array.second));
    }
    source->GetOutputDataObject(0)->Initialize();
    elevation->GetOutputDataObject(0)->Initialize();
    auto after = Arrays(kept);
    for (std::size_t i = 0; i < after.size(); ++i)
    {
      if (Values(after[i].second) != expected[i])
      {
        std::cerr << name << ": " << after[i].first << " changed after the input was released\n";
        ok = false;
      }
    }
    elevation->Modified();
  }

  // Cell arrays come out of Viskores as vtkmDataArrays on every device, so
  // there is always something to convert.
  if (wrapped == 0)
  {
    std::cerr << "No vtkmDataArray was converted\n";
    ok = false;
  }
  ok = TestPolyhedra() && ok;
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
