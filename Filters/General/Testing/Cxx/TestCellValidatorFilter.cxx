// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests vtkCellValidator as a filter

#include <vtkCellValidator.h>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkDataArray.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWeakPointer.h>

// #include <vtkXMLUnstructuredGridWriter.h>

#include <iostream>

//------------------------------------------------------------------------------
namespace
{

bool TestArray(vtkDataArray* stateArray, const std::vector<vtkCellStatus>& expectedValues)
{
  if (!stateArray)
  {
    return false;
  }

  vtkIdType size = static_cast<vtkIdType>(expectedValues.size());
  if (stateArray->GetNumberOfTuples() != size)
  {
    return false;
  }

  for (vtkIdType cellId = 0; cellId < size; cellId++)
  {
    auto state = static_cast<vtkCellStatus>(static_cast<short>(stateArray->GetTuple1(cellId)));
    if (state != vtkCellStatus::Valid && (state != expectedValues[cellId]))
    {
      std::cerr << "  ERROR: invalid cell state " << vtkCellStatus(state)
                << " found at id: " << cellId << ", expected " << expectedValues[cellId] << "\n";
      return false;
    }
  }

  return true;
}

int PolyDataTest()
{
  std::cout << "Testing validator on polydata\n";
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->Allocate(5);
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0.1, 0.1);
  polydata->SetPoints(points);

  std::vector<vtkCellStatus> cellsValidity;

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  cellsValidity.emplace_back(vtkCellStatus::Valid);

  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellStatus::Valid);

  vtkNew<vtkCellArray> polys;
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellStatus::Valid);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(3);
  polys->InsertCellPoint(2);
  cellsValidity.emplace_back(vtkCellStatus::IntersectingEdges | vtkCellStatus::Nonconvex);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(4);
  polys->InsertCellPoint(3);
  cellsValidity.emplace_back(vtkCellStatus::Nonconvex);

  polys->InsertNextCell(2);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  // line is not a poly: wrong number of points
  cellsValidity.emplace_back(vtkCellStatus::WrongNumberOfPoints);

  polydata->SetLines(lines);
  polydata->SetPolys(polys);

  auto validator = vtkSmartPointer<vtkCellValidator>::New();
  validator->SetInputData(polydata);
  validator->Update();

  // weak ref to output, so we will be able to delete them
  vtkWeakPointer<vtkPolyData> output = validator->GetPolyDataOutput();
  vtkWeakPointer<vtkCellData> cellData = output->GetCellData();

  // "hard" ref to array, to test its persistence on dataset deletion.
  vtkSmartPointer<vtkDataArray> stateArray = cellData->GetArray("ValidityState");
  if (!TestArray(stateArray, cellsValidity))
  {
    std::cout << "  Result: failure on initial pass\n";
    return EXIT_FAILURE;
  }

  // test that ValidityState array is persistent when dataset is deleted.
  // This is done because the filters create an implicit array that called the dataset
  // at each request. On dataset DeleteEvent, the array should instantiate an explicit
  // copy of the values to stay valid.
  //
  // see vtkDataSetImplicitBackendInterface for more.
  validator = nullptr;

  if (!stateArray)
  {
    std::cout << "  Result: Failure to retain state array.\n";
    return EXIT_FAILURE;
  }

  if (output)
  {
    std::cout << "  Result: Failure to destroy output dataset.\n";
    return EXIT_FAILURE;
  }

  if (!TestArray(stateArray, cellsValidity))
  {
    std::cout << "  Result: Failure to return expected test values.\n";
    return EXIT_FAILURE;
  }

  std::cout << "  Result: pass\n";
  return EXIT_SUCCESS;
}

vtkSmartPointer<vtkUnstructuredGrid> CreateUGrid(bool inverted)
{
  auto ugrid = vtkSmartPointer<vtkUnstructuredGrid>::New();
  vtkNew<vtkPoints> pts;
  pts->SetDataTypeToDouble();
  pts->SetNumberOfPoints(4);
  // clang-format off
  double coords[] = {
    -0.18037003450393677, -0.14971267614364622, 0,
    -0.1857256755943274,  -0.1493290023589112,  0.005645338974757973,
    -0.18676666440963743, -0.15362494945526123, 0,
    -0.18691087799072265, -0.1459635227203369,  0
  };
  // clang-format on
  for (int ii = 0; ii < 4; ++ii)
  {
    // pts->SetPoint(inverted ? 3 - ii : ii, coords + 3 * ii);
    pts->SetPoint(ii, coords + 3 * ii);
  }
  ugrid->SetPoints(pts);

  // clang-format off
  std::array<std::array<vtkIdType, 3>, 4> faceConn{ {
    { 0, 1, 2, },
    { 3, 1, 0, },
    { 2, 1, 3, },
    { 0, 2, 3, },
  } };
  std::array<std::array<vtkIdType, 3>, 4> invertedFaceConn{ {
    { 0, 2, 1, },
    { 3, 0, 1, },
    { 2, 3, 1, },
    { 0, 3, 2, },
  } };
  // clang-format off
  vtkNew<vtkCellArray> faces;
  for (int ii = 0; ii < 4; ++ii)
  {
    faces->InsertNextCell(3, inverted ? invertedFaceConn[ii].data() : faceConn[ii].data());
  }
  vtkNew<vtkCellArray> faceLocations;
  std::array<vtkIdType, 4> faceIds{ { 0, 1, 2, 3 } };
  faceLocations->InsertNextCell(4, faceIds.data());

  vtkNew<vtkCellArray> cells;
  std::array<vtkIdType, 4> pointIds{ { 0, 1, 2, 3 } };
  cells->InsertNextCell(4, pointIds.data());
  vtkNew<vtkUnsignedCharArray> cellTypes;
  cellTypes->InsertNextValue(VTK_POLYHEDRON);

  ugrid->SetPolyhedralCells(cellTypes, cells, faceLocations, faces);

  // Enable this for debugging.
#if 0
  vtkNew<vtkXMLUnstructuredGridWriter> wri;
  wri->SetInputDataObject(0, ugrid);
  static int cnt = 65;
  char fname[] = "/tmp/fooX.vtu";
  fname[8] = cnt++;
  wri->SetFileName(fname);
  wri->SetDataModeToAscii();
  wri->Write();
#endif

  return ugrid;
}

bool TestUGrid(bool inverted, bool autoTol)
{
  std::cout << "Testing " << (inverted ? "inverted" : "properly-oriented") << " cell with "
            << (autoTol ? "automatic" : "manual") << " tolerance.\n";

  auto ugrid = CreateUGrid(inverted);
  std::vector<vtkCellStatus> expectedStatus(1,
    (inverted ? vtkCellStatus::FacesAreOrientedIncorrectly : vtkCellStatus::Valid) |
      (autoTol ? vtkCellStatus::Valid : vtkCellStatus::CoincidentPoints));

  // Tolerance larger than cell so that if autoTol is false, a flag is set:
  auto validator = vtkSmartPointer<vtkCellValidator>::New();
  validator->SetTolerance(0.3);
  validator->SetAutoTolerance(autoTol);
  validator->SetInputDataObject(0, ugrid);
  validator->Update();

  auto* status = validator->GetOutput()->GetCellData()->GetArray("ValidityState");
  bool ok = TestArray(status, expectedStatus);
  std::cout << "  Result: " << (ok ? "pass" : "fail") << "\n";
  return ok;
}

int UnstructuredGridTest()
{
  bool ok = true;
  ok &= TestUGrid(true, true);
  ok &= TestUGrid(false, true);
  ok &= TestUGrid(true, false);
  ok &= TestUGrid(false, false);

  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}

} // anonymous namespace
//------------------------------------------------------------------------------
int TestCellValidatorFilter(int, char*[])
{
  int result = PolyDataTest();
  if (result == EXIT_FAILURE)
  {
    return EXIT_FAILURE;
  }

  result = UnstructuredGridTest();
  return result;
}
