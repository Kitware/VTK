// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkConstantArray.h"
#include "vtkFieldData.h"
#include "vtkIdTypeArray.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkPolyDataToUnstructuredGrid.h"
#include "vtkUnstructuredGrid.h"

#include <algorithm>
#include <iostream>
#include <vector>

namespace
{
vtkSmartPointer<vtkPolyData> Create()
{
  auto pd = vtkSmartPointer<vtkPolyData>::New();
  vtkNew<vtkPoints> points;
  for (int i = 0; i < 12; ++i)
  {
    points->InsertNextPoint(i, i * i, 0);
  }
  pd->SetPoints(points);
  return pd;
}

struct Cell
{
  int Type;
  std::vector<vtkIdType> Ids;
};

// Converts `pd` and checks that the cells of the output are `expected` in order, that the cell data
// is kept, that the cells of `pd` are shared if it has a single cell array, and whether the cell
// types and the offsets are implicit.
bool Check(const char* name, vtkPolyData* pd, const std::vector<Cell>& expected, bool implicitTypes,
  bool implicitOffsets)
{
  vtkNew<vtkIdTypeArray> cellIds;
  cellIds->SetName("CellIds");
  for (vtkIdType i = 0; i < pd->GetNumberOfCells(); ++i)
  {
    cellIds->InsertNextValue(100 + i);
  }
  pd->GetCellData()->AddArray(cellIds);
  vtkNew<vtkIdTypeArray> field;
  field->SetName("Field");
  pd->GetFieldData()->AddArray(field);

  vtkNew<vtkPolyDataToUnstructuredGrid> filter;
  filter->SetInputData(pd);
  filter->Update();
  vtkUnstructuredGrid* ug = filter->GetOutput();

  if (ug->GetNumberOfCells() != static_cast<vtkIdType>(expected.size()))
  {
    std::cerr << name << ": wrong number of cells " << ug->GetNumberOfCells() << "\n";
    return false;
  }
  for (vtkIdType c = 0; c < ug->GetNumberOfCells(); ++c)
  {
    vtkIdType npts;
    const vtkIdType* pts;
    ug->GetCellPoints(c, npts, pts);
    if (ug->GetCellType(c) != expected[c].Type ||
      npts != static_cast<vtkIdType>(expected[c].Ids.size()) ||
      !std::equal(pts, pts + npts, expected[c].Ids.begin()))
    {
      std::cerr << name << ": wrong cell " << c << "\n";
      return false;
    }
  }
  vtkDataArray* out = ug->GetCellData()->GetArray("CellIds");
  if (!out || out->GetNumberOfTuples() != ug->GetNumberOfCells() || out->GetTuple1(0) != 100 ||
    !ug->GetFieldData()->GetArray("Field"))
  {
    std::cerr << name << ": cell or field data lost\n";
    return false;
  }
  std::vector<vtkCellArray*> cellArrays;
  for (vtkCellArray* cells : { pd->GetVerts(), pd->GetLines(), pd->GetPolys(), pd->GetStrips() })
  {
    if (cells->GetNumberOfCells() > 0)
    {
      cellArrays.push_back(cells);
    }
  }
  if (cellArrays.size() == 1 && ug->GetCells() != cellArrays[0])
  {
    std::cerr << name << ": the cells of the input should be shared\n";
    return false;
  }
  if (implicitTypes !=
    (vtkConstantArray<unsigned char>::SafeDownCast(ug->GetCellTypes()) != nullptr))
  {
    std::cerr << name << ": the cell types should " << (implicitTypes ? "" : "not ")
              << "be implicit\n";
    return false;
  }
  if (implicitOffsets != ug->GetCells()->IsStorageFixedSize())
  {
    std::cerr << name << ": the offsets should " << (implicitOffsets ? "" : "not ")
              << "be implicit\n";
    return false;
  }
  return true;
}
} // namespace

int TestPolyDataToUnstructuredGrid(int, char*[])
{
  bool ok = true;
  {
    // one homogeneous array: shared cells and implicit types
    auto pd = Create();
    vtkNew<vtkCellArray> polys;
    vtkIdType q0[4] = { 0, 1, 2, 3 }, q1[4] = { 4, 5, 6, 7 };
    polys->InsertNextCell(4, q0);
    polys->InsertNextCell(4, q1);
    pd->SetPolys(polys);
    ok &= Check(
      "quads", pd, { { VTK_QUAD, { 0, 1, 2, 3 } }, { VTK_QUAD, { 4, 5, 6, 7 } } }, true, false);
  }
  {
    // one array with cells of different sizes
    auto pd = Create();
    vtkNew<vtkCellArray> polys;
    vtkIdType t[3] = { 0, 1, 2 }, q[4] = { 3, 4, 5, 6 }, p[5] = { 6, 7, 8, 9, 10 };
    polys->InsertNextCell(3, t);
    polys->InsertNextCell(4, q);
    polys->InsertNextCell(5, p);
    pd->SetPolys(polys);
    ok &= Check("mixed polys", pd,
      { { VTK_TRIANGLE, { 0, 1, 2 } }, { VTK_QUAD, { 3, 4, 5, 6 } },
        { VTK_POLYGON, { 6, 7, 8, 9, 10 } } },
      false, false);
  }
  {
    // strips are always of the same type
    auto pd = Create();
    vtkNew<vtkCellArray> strips;
    vtkIdType s0[4] = { 0, 1, 2, 3 }, s1[5] = { 4, 5, 6, 7, 8 };
    strips->InsertNextCell(4, s0);
    strips->InsertNextCell(5, s1);
    pd->SetStrips(strips);
    ok &= Check("strips", pd,
      { { VTK_TRIANGLE_STRIP, { 0, 1, 2, 3 } }, { VTK_TRIANGLE_STRIP, { 4, 5, 6, 7, 8 } } }, true,
      false);
  }
  {
    // all the cell arrays, with cells of different sizes
    auto pd = Create();
    vtkNew<vtkCellArray> verts, lines, polys, strips;
    vtkIdType v[1] = { 11 }, pv[2] = { 1, 2 }, l0[2] = { 0, 1 }, l1[3] = { 1, 2, 3 },
              t[3] = { 0, 1, 2 }, q[4] = { 3, 4, 5, 6 }, s[4] = { 4, 5, 6, 7 };
    verts->InsertNextCell(1, v);
    verts->InsertNextCell(2, pv);
    lines->InsertNextCell(2, l0);
    lines->InsertNextCell(3, l1);
    polys->InsertNextCell(3, t);
    polys->InsertNextCell(4, q);
    strips->InsertNextCell(4, s);
    pd->SetVerts(verts);
    pd->SetLines(lines);
    pd->SetPolys(polys);
    pd->SetStrips(strips);
    ok &= Check("all arrays", pd,
      { { VTK_VERTEX, { 11 } }, { VTK_POLY_VERTEX, { 1, 2 } }, { VTK_LINE, { 0, 1 } },
        { VTK_POLY_LINE, { 1, 2, 3 } }, { VTK_TRIANGLE, { 0, 1, 2 } }, { VTK_QUAD, { 3, 4, 5, 6 } },
        { VTK_TRIANGLE_STRIP, { 4, 5, 6, 7 } } },
      false, false);
  }
  {
    // several homogeneous arrays with different cell sizes
    auto pd = Create();
    vtkNew<vtkCellArray> lines, polys;
    vtkIdType l0[2] = { 0, 1 }, l1[2] = { 2, 3 }, q[4] = { 3, 4, 5, 6 };
    lines->InsertNextCell(2, l0);
    lines->InsertNextCell(2, l1);
    polys->InsertNextCell(4, q);
    pd->SetLines(lines);
    pd->SetPolys(polys);
    ok &= Check("lines and quad", pd,
      { { VTK_LINE, { 0, 1 } }, { VTK_LINE, { 2, 3 } }, { VTK_QUAD, { 3, 4, 5, 6 } } }, false,
      false);
  }
  {
    // several arrays whose cells have the same size, so the offsets of the output are implicit
    auto pd = Create();
    vtkNew<vtkCellArray> polys, strips;
    vtkIdType t[3] = { 0, 1, 2 }, s[3] = { 3, 4, 5 };
    polys->InsertNextCell(3, t);
    strips->InsertNextCell(3, s);
    pd->SetPolys(polys);
    pd->SetStrips(strips);
    ok &= Check("triangles and strip", pd,
      { { VTK_TRIANGLE, { 0, 1, 2 } }, { VTK_TRIANGLE_STRIP, { 3, 4, 5 } } }, false, true);
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
