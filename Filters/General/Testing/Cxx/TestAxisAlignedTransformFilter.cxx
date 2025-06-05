// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAxisAlignedTransformFilter.h"
#include "vtkCellData.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkMatrix3x3.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridToExplicitStructuredGrid.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

bool Assert(bool test, const std::string& msg)
{
  if (!test)
  {
    vtkLog(ERROR, "Test failed: " << msg);
  }
  return test;
}

template <typename ReaderType>
ReaderType* ReadFile(const char* path, int argc, char* argv[])
{
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, path);
  vtkSmartPointer<ReaderType> reader = ReaderType::New();
  reader->SetFileName(fileName);
  reader->Update();
  delete[] fileName;
  return reader;
}

vtkSmartPointer<vtkDataObject> Transform(vtkAlgorithmOutput* port, double translation[3],
  double scaling[3], vtkAxisAlignedTransformFilter::Angle rotationAngle,
  vtkAxisAlignedTransformFilter::Axis axis)
{
  vtkSmartPointer<vtkAxisAlignedTransformFilter> transform =
    vtkSmartPointer<vtkAxisAlignedTransformFilter>::New();
  transform->SetInputConnection(port);
  transform->SetTranslation(translation);
  transform->SetScale(scaling);
  transform->SetRotationAngle(rotationAngle);
  transform->SetRotationAxis(axis);
  transform->Update();

  vtkSmartPointer<vtkDataObject> output = vtkDataObject::SafeDownCast(transform->GetOutput());

  return output;
}

bool TestTransformUnstructuredGrid(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLUnstructuredGridReader>("Data/can.vtu", argc, argv));

  double translation[3] = { 1.0, 2.0, 3.0 };
  double scaling[3] = { 1.0, 2.0, 3.0 };
  vtkSmartPointer<vtkUnstructuredGrid> unstructGridOut =
    vtkUnstructuredGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT90, vtkAxisAlignedTransformFilter::Axis::X));

  vtkUnstructuredGrid* unstructGridIn = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(unstructGridOut->GetNumberOfPoints() == unstructGridIn->GetNumberOfPoints(),
    "vtkUnstructuredGrid, Incorrect number of points");
  test &= Assert(unstructGridOut->GetNumberOfCells() == unstructGridIn->GetNumberOfCells(),
    "vtkUnstructuredGrid, Incorrect number of cells");

  test &= Assert(unstructGridOut->GetPoint(10)[1] == 31.77126312255859375,
    "vtkUnstructuredGrid, Incorrect points");

  vtkNew<vtkIdList> ptsIn;
  unstructGridIn->GetCellPoints(5, ptsIn);
  vtkNew<vtkIdList> ptsOut;
  unstructGridOut->GetCellPoints(5, ptsOut);
  test &= Assert(ptsOut->GetId(4) == ptsIn->GetId(4) && ptsIn->GetId(4) == 20,
    "vtkUnstructuredGrid, Incorrect cell points");

  test &= Assert(unstructGridOut->GetPointData()->GetArray("ACCL")->GetTuple3(0)[0] == 2269740,
    "vtkUnstructuredGrid, Incorrect cell data");

  return test;
}

bool TestTransformImageData(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLImageDataReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLImageDataReader>("Data/scalars.vti", argc, argv));

  double translation[3] = { 3.0, 2.0, -3.0 };
  double scaling[3] = { 0.5, 2.0, 1.5 };
  vtkSmartPointer<vtkImageData> imageData =
    vtkImageData::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT180, vtkAxisAlignedTransformFilter::Axis::X));

  test &= Assert(imageData->GetOrigin()[0] == 3 && imageData->GetOrigin()[1] == 2 &&
      imageData->GetOrigin()[2] == -3,
    "vtkImageData, Incorrect origin");
  test &= Assert(imageData->GetDirectionMatrix()->GetElement(0, 0) == 0.5 &&
      imageData->GetDirectionMatrix()->GetElement(1, 1) == -2 &&
      imageData->GetDirectionMatrix()->GetElement(2, 2) == -1.5,
    "vtkImageData, Incorrect direction matrix");
  test &= Assert(imageData->GetScalarComponentAsDouble(0, 0, 8, 0) == 8,
    "vtkImageData, Incorrect scalar component");

  return test;
}

bool TestTransformRectilinearGrid(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLRectilinearGridReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLRectilinearGridReader>("Data/rectGrid.vtr", argc, argv));

  double translation[3] = { -1.0, -2.0, -3.0 };
  double scaling[3] = { 1.0, 1.0, 1.0 };
  vtkSmartPointer<vtkRectilinearGrid> rectGridOut =
    vtkRectilinearGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::X));

  vtkRectilinearGrid* rectGridIn = vtkRectilinearGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(rectGridOut->GetNumberOfPoints() == rectGridIn->GetNumberOfPoints(),
    "vtkRectilinearGrid, Incorrect number of points");
  test &= Assert(rectGridOut->GetNumberOfCells() == rectGridIn->GetNumberOfCells(),
    "vtkRectilinearGrid, Incorrect number of cells");

  test &= Assert(rectGridOut->GetYCoordinates()->GetTuple1(3) == -0.61370563507080078125,
    "vtkRectilinearGrid, Incorrect Y coordinates");

  test &= Assert(rectGridOut->GetCellData()->GetArray(0)->GetTuple3(5)[0] ==
        rectGridIn->GetCellData()->GetArray(0)->GetTuple3(229)[0] &&
      rectGridOut->GetCellData()->GetArray(0)->GetTuple3(5)[0] == 229,
    "vtkRectilinearGrid, Incorrect cell data");

  return test;
}

bool TestTransformExplicitStructuredGrid(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader = vtk::TakeSmartPointer(
    ReadFile<vtkXMLUnstructuredGridReader>("Data/explicitStructuredGrid.vtu", argc, argv));

  vtkSmartPointer<vtkUnstructuredGridToExplicitStructuredGrid> UgToEsg =
    vtkSmartPointer<vtkUnstructuredGridToExplicitStructuredGrid>::New();
  UgToEsg->SetInputConnection(reader->GetOutputPort());
  UgToEsg->SetWholeExtent(0, 5, 0, 13, 0, 3);
  UgToEsg->SetInputArrayToProcess(0, 0, 0, 1, "BLOCK_I");
  UgToEsg->SetInputArrayToProcess(1, 0, 0, 1, "BLOCK_J");
  UgToEsg->SetInputArrayToProcess(2, 0, 0, 1, "BLOCK_K");
  UgToEsg->Update();

  double translation[3] = { -1.0, 2.0, -3.0 };
  double scaling[3] = { 3.0, 0.5, 1.0 };
  vtkSmartPointer<vtkExplicitStructuredGrid> esgOut = vtkExplicitStructuredGrid::SafeDownCast(
    Transform(UgToEsg->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT180, vtkAxisAlignedTransformFilter::Axis::Y));

  vtkExplicitStructuredGrid* esgIn = vtkExplicitStructuredGrid::SafeDownCast(UgToEsg->GetOutput());

  test &= Assert(esgOut->GetNumberOfPoints() == esgIn->GetNumberOfPoints(),
    "vtkExplicitStructuredGrid, Incorrect number of points");
  test &= Assert(esgOut->GetNumberOfCells() == esgIn->GetNumberOfCells(),
    "vtkExplicitStructuredGrid, Incorrect number of cells");

  test &= Assert(esgOut->GetPoint(0)[2] == -esgIn->GetPoint(0)[2] + translation[2],
    "vtkExplicitStructuredGrid, Incorrect points");

  test &= Assert(esgOut->GetCellPoints(5)[0] == esgIn->GetCellPoints(5)[0],
    "vtkExplicitStructuredGrid, Incorrect cell points");

  return test;
}

bool TestTransformStructuredGrid(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLStructuredGridReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLStructuredGridReader>("Data/structGrid.vts", argc, argv));

  double translation[3] = { -1.0, 0.8, 1.4 };
  double scaling[3] = { 1.3, 0.8, 0.9 };
  vtkSmartPointer<vtkStructuredGrid> structGridOut =
    vtkStructuredGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT0, vtkAxisAlignedTransformFilter::Axis::Z));

  vtkStructuredGrid* structGridIn = vtkStructuredGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(structGridOut->GetNumberOfPoints() == structGridIn->GetNumberOfPoints(),
    "vtkStructuredGrid, Incorrect number of points");
  test &= Assert(structGridOut->GetNumberOfCells() == structGridIn->GetNumberOfCells(),
    "vtkStructuredGrid, Incorrect number of cells");

  test &= Assert(structGridOut->GetPoint(1429)[2] == 1.9800000190734863281,
    "vtkStructuredGrid, Incorrect points");

  test &= Assert(structGridOut->GetPointData()->GetArray(0)->GetTuple3(5)[2] == 5,
    "vtkStructuredGrid, Incorrect cell data");

  return test;
}

bool TestTransformPolyData(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLPolyDataReader>("Data/cow.vtp", argc, argv));

  double translation[3] = { -1.0, 2, 1.0 };
  double scaling[3] = { 0.2, 0.8, 0.9 };
  vtkSmartPointer<vtkPolyData> polyDataOut =
    vtkPolyData::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::Z));

  vtkPolyData* polyDataIn = vtkPolyData::SafeDownCast(reader->GetOutput());

  test &= Assert(polyDataOut->GetNumberOfPoints() == polyDataIn->GetNumberOfPoints(),
    "vtkPolyData, Incorrect number of points");
  test &= Assert(polyDataOut->GetNumberOfCells() == polyDataIn->GetNumberOfCells(),
    "vtkPolyData, Incorrect number of cells");

  test &=
    Assert(polyDataOut->GetPoint(10)[0] == 0.16154038906097412109, "vtkPolyData, Incorrect points");

  vtkNew<vtkIdList> cellPtsIn;
  polyDataIn->GetPolys()->GetCell(0, cellPtsIn);
  vtkNew<vtkIdList> cellPtsOut;
  polyDataOut->GetPolys()->GetCell(0, cellPtsOut);

  test &= Assert(cellPtsIn->GetId(1) == cellPtsOut->GetId(1) && cellPtsOut->GetId(3) == 252,
    "vtkPolyData, Incorrect cells");

  return test;
}

bool TestTransformHyperTreeGrid3D(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLHyperTreeGridReader> reader =
    vtk::TakeSmartPointer(ReadFile<vtkXMLHyperTreeGridReader>("Data/HTG/shell_3d.htg", argc, argv));

  double translation[3] = { -2.1, 0.0, 1.2 };
  double scaling[3] = { 2.0, 0.5, 1.5 };
  vtkSmartPointer<vtkHyperTreeGrid> htgOut =
    vtkHyperTreeGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT90, vtkAxisAlignedTransformFilter::Axis::Y));

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(),
    "vtkHyperTreeGrid, Incorrect number of cells");

  test &= Assert(
    htgOut->GetYCoordinates()->GetTuple1(2) == 0.5, "vtkHyperTreeGrid, Incorrect coordinates");

  test &=
    Assert(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(57)[1] ==
        scaling[1] *
          htgOut->GetCellData()->GetArray(htgOut->GetInterfaceNormalsName())->GetTuple3(6383)[1],
      "vtkHyperTreeGrid, Incorrect normals");

  test &=
    Assert(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(5)[1] ==
        0.47808688094430307203,
      "vtkHyperTreeGrid, Incorrect intercepts");
  test &= Assert(
    htgOut->GetCellData()->GetArray(htgOut->GetInterfaceInterceptsName())->GetTuple3(5)[1] == 0.5,
    "vtkHyperTreeGrid, Incorrect intercepts");

  test &= Assert(htgIn->GetCellData()->GetArray(0)->GetTuple1(57) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(6383),
    "vtkHyperTreeGrid, Incorrect cells");

  return test;
}

bool TestTransformHyperTreeGrid2D(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLHyperTreeGridReader> reader = vtk::TakeSmartPointer(
    ReadFile<vtkXMLHyperTreeGridReader>("Data/HTG/donut_XZ_shift_2d.htg", argc, argv));

  double translation[3] = { 1.5, -1.0, 2.0 };
  double scaling[3] = { 1.2, 0.8, 1.0 };
  vtkSmartPointer<vtkHyperTreeGrid> htgOut =
    vtkHyperTreeGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::Y));

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(),
    "vtkHyperTreeGrid, Incorrect number of cells");

  test &= Assert(
    htgOut->GetXCoordinates()->GetTuple1(2) == 2.5, "vtkHyperTreeGrid, Incorrect coordinates");

  test &=
    Assert(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(0)[1] ==
        scaling[1] *
          htgOut->GetCellData()->GetArray(htgOut->GetInterfaceNormalsName())->GetTuple3(93)[1],
      "vtkHyperTreeGrid, Incorrect normals");

  test &=
    Assert(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(15)[1] ==
        -0.37878773115802955029,
      "vtkHyperTreeGrid, Incorrect intercepts");
  test &= Assert(
    htgOut->GetCellData()->GetArray(htgOut->GetInterfaceInterceptsName())->GetTuple3(55)[1] ==
      1.598185390529741845,
    "vtkHyperTreeGrid, Incorrect intercepts");

  test &= Assert(htgIn->GetCellData()->GetArray(0)->GetTuple1(0) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(93),
    "vtkHyperTreeGrid, Incorrect cells");

  return test;
}

// This test uses a data which is in IJK indexing mode and which has some hyper trees pointing to
// nullptr
bool TestTransformHyperTreeGrid2DIJK(int argc, char* argv[])
{
  bool test = true;
  vtkSmartPointer<vtkXMLHyperTreeGridReader> reader = vtk::TakeSmartPointer(
    ReadFile<vtkXMLHyperTreeGridReader>("Data/HTG/random_partial_ZX.htg", argc, argv));

  double translation[3] = { 0.0, 0.0, 2.0 };
  double scaling[3] = { 0.5, 1.0, 1.0 };
  vtkSmartPointer<vtkHyperTreeGrid> htgOut =
    vtkHyperTreeGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT180, vtkAxisAlignedTransformFilter::Axis::Z));

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  test &= Assert(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(),
    "vtkHyperTreeGrid, Incorrect number of cells");

  test &= Assert(htgIn->GetCellData()->GetArray(0)->GetTuple1(14) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(56),
    "vtkHyperTreeGrid, Incorrect cells");

  test &= Assert(htgIn->GetCellData()->GetArray(0)->GetTuple1(0) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(42),
    "vtkHyperTreeGrid, Incorrect cells");

  test &= Assert(htgIn->GetCellData()->GetArray(0)->GetTuple1(17) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(25),
    "vtkHyperTreeGrid, Incorrect cells");

  return test;
}

// This function tests all the input types, and each input type will test different translation
// scaling, rotation angles and axis.
int TestAxisAlignedTransformFilter(int argc, char* argv[])
{
  bool testVal = TestTransformUnstructuredGrid(argc, argv);
  testVal &= TestTransformExplicitStructuredGrid(argc, argv);
  testVal &= TestTransformStructuredGrid(argc, argv);
  testVal &= TestTransformPolyData(argc, argv);
  testVal &= TestTransformImageData(argc, argv);
  testVal &= TestTransformRectilinearGrid(argc, argv);
  testVal &= TestTransformHyperTreeGrid3D(argc, argv);
  testVal &= TestTransformHyperTreeGrid2D(argc, argv);
  testVal &= TestTransformHyperTreeGrid2DIJK(argc, argv);

  return testVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
