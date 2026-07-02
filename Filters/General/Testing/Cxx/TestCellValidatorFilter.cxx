// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests vtkCellValidator as a filter
#include <vtkCellValidator.h>

#include <vtkCellArray.h>
#include <vtkCellData.h>
#include <vtkCellStatus.h>
#include <vtkDataArray.h>
#include <vtkImageData.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkSmartPointer.h>
#include <vtkUnstructuredGrid.h>
#include <vtkWeakPointer.h>

#include <vector>

//------------------------------------------------------------------------------
namespace
{

//------------------------------------------------------------------------------
bool TestArray(vtkDataArray* stateArray, const std::vector<vtkCellStatus>& expectedValues)
{
  if (!stateArray)
  {
    vtkLog(ERROR, "no input data array");
    return false;
  }

  vtkIdType size = static_cast<vtkIdType>(expectedValues.size());
  if (stateArray->GetNumberOfTuples() != size)
  {
    vtkLog(
      ERROR, "Number of values mismatched: " << stateArray->GetNumberOfTuples() << " " << size);
    return false;
  }

  bool ret = true;
  for (vtkIdType cellId = 0; cellId < size; cellId++)
  {
    auto state = static_cast<vtkCellStatus>((stateArray->GetTuple1(cellId)));
    if (state != vtkCellStatus::Valid && (state != expectedValues[cellId]))
    {
      vtkLog(ERROR, << "  ERROR: invalid cell state " << vtkCellStatus(state)
                    << " found at id: " << cellId << ", expected " << expectedValues[cellId]);
      ret = false;
    }
  }

  return ret;
}

//------------------------------------------------------------------------------
bool PolyDataTest()
{
  vtkLogScopeFunction(INFO);
  vtkNew<vtkPolyData> polydata;
  vtkNew<vtkPoints> points;
  points->Reserve(5);
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(0, 0, 1);
  points->InsertNextPoint(0, 1, 1);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(0, 0.1, 0.1);
  polydata->SetPoints(points);

  std::vector<vtkCellStatus> expectedCellsValidity;

  vtkNew<vtkCellArray> lines;
  lines->InsertNextCell(2);
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  expectedCellsValidity.emplace_back(vtkCellStatus::Valid);

  lines->InsertNextCell(2);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  expectedCellsValidity.emplace_back(vtkCellStatus::Valid);

  vtkNew<vtkCellArray> polys;
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  expectedCellsValidity.emplace_back(vtkCellStatus::Valid);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(3);
  polys->InsertCellPoint(2);
  expectedCellsValidity.emplace_back(vtkCellStatus::IntersectingEdges | vtkCellStatus::Nonconvex);

  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(4);
  polys->InsertCellPoint(3);
  expectedCellsValidity.emplace_back(vtkCellStatus::Nonconvex);

  polys->InsertNextCell(2);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  // line is not a poly: wrong number of points
  expectedCellsValidity.emplace_back(vtkCellStatus::WrongNumberOfPoints);

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
  bool arrayOk = TestArray(stateArray, expectedCellsValidity);
  vtkLogIf(ERROR, !arrayOk, "Computed array has not the expected values.");

  // test that ValidityState array is persistent when dataset is deleted.
  // This is done because the filters create an implicit array that called the dataset
  // at each request. On dataset DeleteEvent, the array should instantiate an explicit
  // copy of the values to stay valid.
  //
  // see vtkDataSetImplicitBackendInterface for more.
  validator = nullptr;

  bool ret = true;
  bool dataDestroyed = output == nullptr;
  vtkLogIf(ERROR, !dataDestroyed, "Failure to destroy output dataset");
  ret &= dataDestroyed;

  bool persistentArray = stateArray != nullptr;
  vtkLogIf(ERROR, !persistentArray, "Failure to retain state array");
  ret &= persistentArray;

  bool cachedArrayOk = TestArray(stateArray, expectedCellsValidity);
  vtkLogIf(ERROR, !cachedArrayOk, "Cached array has not the expected values.");
  ret &= cachedArrayOk;

  return ret;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkUnstructuredGrid> CreateUGrid(bool inverted)
{
  vtkNew<vtkUnstructuredGrid> ugrid;
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

//------------------------------------------------------------------------------
bool TestUGrid(bool inverted, bool autoTol)
{
  vtkLogScopeFunction(INFO);
  vtkLog(INFO, << (inverted ? "inverted" : "properly-oriented") << " cell with " << (autoTol ? "automatic" : "manual") << " tolerance.");

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
  return TestArray(status, expectedStatus);
}

//------------------------------------------------------------------------------
bool UnstructuredGridTest()
{
  bool ok = true;
  ok &= TestUGrid(true, true);
  ok &= TestUGrid(false, true);
  ok &= TestUGrid(true, false);
  ok &= TestUGrid(false, false);

  return ok;
}

//------------------------------------------------------------------------------
// if useVoxel is true, create 3D grid (cells will be vtkVoxel)
// if useVoxel is false, create 2D grid (cells will be vtkPixel)
bool ImageDataTest(bool useVoxel)
{
  vtkLogScopeFunction(INFO);
  vtkLog(INFO, << "Use " << (useVoxel ? "vtkVoxel" : "vtkPixel") << " cells");
  vtkNew<vtkImageData> image;
  int zDim = useVoxel ? 10 : 1;
  image->SetDimensions(10, 10, zDim);
  vtkNew<vtkCellValidator> validator;
  validator->SetInputData(image);
  validator->Update();

  std::vector<vtkCellStatus> expectedStatus(image->GetNumberOfCells(), vtkCellStatus::Valid);

  auto status = validator->GetOutput()->GetCellData()->GetArray("ValidityState");
  return TestArray(status, expectedStatus);
}

} // anonymous namespace

//------------------------------------------------------------------------------
int TestCellValidatorFilter(int, char*[])
{
  bool result = ::PolyDataTest();
  result &= ::UnstructuredGridTest();
  result &= ::ImageDataTest(true);
  result &= ::ImageDataTest(false);
  return result ? EXIT_SUCCESS : EXIT_FAILURE;
}
