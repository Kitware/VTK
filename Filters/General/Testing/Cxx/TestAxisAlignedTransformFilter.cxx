// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkAxisAlignedTransformFilter.h"
#include "vtkCellData.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
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

#define AssertMacro(b, data, reason)                                                               \
  if (!(b))                                                                                        \
  {                                                                                                \
    std::cerr << "Failed to transform " << data << ": " << reason << std::endl;                    \
    return EXIT_FAILURE;                                                                           \
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
  double scaling[3], int rotationAngle, vtkAxisAlignedTransformFilter::Axis axis)
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

int TestTransformUnstructuredGrid(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    ReadFile<vtkXMLUnstructuredGridReader>("Data/can.vtu", argc, argv);

  double translation[3] = { 1.0, 2.0, 3.0 };
  double scaling[3] = { 1.0, 2.0, 3.0 };
  vtkSmartPointer<vtkUnstructuredGrid> unstructGridOut =
    vtkUnstructuredGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT90, vtkAxisAlignedTransformFilter::Axis::X));

  vtkUnstructuredGrid* unstructGridIn = vtkUnstructuredGrid::SafeDownCast(reader->GetOutput());

  AssertMacro(unstructGridOut->GetNumberOfPoints() == unstructGridIn->GetNumberOfPoints(),
    unstructGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(unstructGridOut->GetNumberOfCells() == unstructGridIn->GetNumberOfCells(),
    unstructGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(unstructGridOut->GetPoint(10)[1] == 31.77126312255859375,
    unstructGridOut->GetClassName(), "Incorrect points");

  vtkNew<vtkIdList> ptsIn;
  unstructGridIn->GetCellPoints(5, ptsIn);
  vtkNew<vtkIdList> ptsOut;
  unstructGridOut->GetCellPoints(5, ptsOut);
  AssertMacro(ptsOut->GetId(4) == ptsIn->GetId(4) && ptsIn->GetId(4) == 20,
    unstructGridOut->GetClassName(), "Incorrect cell points");

  AssertMacro(unstructGridOut->GetPointData()->GetArray("ACCL")->GetTuple3(0)[0] == 2269740,
    unstructGridOut->GetClassName(), "Incorrect cell data");
  reader->Delete();
  return true;
}

int TestTransformImageData(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLImageDataReader> reader =
    ReadFile<vtkXMLImageDataReader>("Data/scalars.vti", argc, argv);

  double translation[3] = { 3.0, 2.0, -3.0 };
  double scaling[3] = { 0.5, 2.0, 1.5 };
  vtkSmartPointer<vtkImageData> imageData =
    vtkImageData::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT180, vtkAxisAlignedTransformFilter::Axis::X));

  AssertMacro(imageData->GetOrigin()[0] == 3 && imageData->GetOrigin()[1] == 2 &&
      imageData->GetOrigin()[2] == -3,
    imageData->GetClassName(), "Incorrect origin");
  AssertMacro(imageData->GetDirectionMatrix()->GetElement(0, 0) == 0.5 &&
      imageData->GetDirectionMatrix()->GetElement(1, 1) == -2 &&
      imageData->GetDirectionMatrix()->GetElement(2, 2) == -1.5,
    imageData->GetClassName(), "Incorrect direction matrix");
  AssertMacro(imageData->GetScalarComponentAsDouble(0, 0, 8, 0) == 8, imageData->GetClassName(),
    "Incorrect scalar component");

  reader->Delete();
  return true;
}

int TestTransformRectilinearGrid(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLRectilinearGridReader> reader =
    ReadFile<vtkXMLRectilinearGridReader>("Data/rectGrid.vtr", argc, argv);

  double translation[3] = { -1.0, -2.0, -3.0 };
  double scaling[3] = { 1.0, 1.0, 1.0 };
  vtkSmartPointer<vtkRectilinearGrid> rectGridOut =
    vtkRectilinearGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::X));

  vtkRectilinearGrid* rectGridIn = vtkRectilinearGrid::SafeDownCast(reader->GetOutput());

  AssertMacro(rectGridOut->GetNumberOfPoints() == rectGridIn->GetNumberOfPoints(),
    rectGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(rectGridOut->GetNumberOfCells() == rectGridIn->GetNumberOfCells(),
    rectGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(rectGridOut->GetYCoordinates()->GetTuple1(3) == -0.61370563507080078125,
    rectGridOut->GetClassName(), "Incorrect Y coordinates");

  AssertMacro(rectGridOut->GetCellData()->GetArray(0)->GetTuple3(5)[0] ==
        rectGridIn->GetCellData()->GetArray(0)->GetTuple3(229)[0] &&
      rectGridOut->GetCellData()->GetArray(0)->GetTuple3(5)[0] == 229,
    rectGridOut->GetClassName(), "Incorrect cell data");

  reader->Delete();
  return true;
}

int TestTransformExplicitStructuredGrid(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    ReadFile<vtkXMLUnstructuredGridReader>("Data/explicitStructuredGrid.vtu", argc, argv);

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

  AssertMacro(esgOut->GetNumberOfPoints() == esgIn->GetNumberOfPoints(), esgOut->GetClassName(),
    "Incorrect number of points");
  AssertMacro(esgOut->GetNumberOfCells() == esgIn->GetNumberOfCells(), esgOut->GetClassName(),
    "Incorrect number of cells");

  AssertMacro(esgOut->GetPoint(0)[2] == -esgIn->GetPoint(0)[2] + translation[2],
    esgOut->GetClassName(), "Incorrect points");

  AssertMacro(esgOut->GetCellPoints(5)[0] == esgIn->GetCellPoints(5)[0], esgOut->GetClassName(),
    "Incorrect cell points");

  reader->Delete();
  return true;
}

int TestTransformStructuredGrid(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLStructuredGridReader> reader =
    ReadFile<vtkXMLStructuredGridReader>("Data/structGrid.vts", argc, argv);

  double translation[3] = { -1.0, 0.8, 1.4 };
  double scaling[3] = { 1.3, 0.8, 0.9 };
  vtkSmartPointer<vtkStructuredGrid> structGridOut =
    vtkStructuredGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT0, vtkAxisAlignedTransformFilter::Axis::Z));

  vtkStructuredGrid* structGridIn = vtkStructuredGrid::SafeDownCast(reader->GetOutput());

  AssertMacro(structGridOut->GetNumberOfPoints() == structGridIn->GetNumberOfPoints(),
    structGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(structGridOut->GetNumberOfCells() == structGridIn->GetNumberOfCells(),
    structGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(structGridOut->GetPoint(1429)[2] == 1.9800000190734863281,
    structGridOut->GetClassName(), "Incorrect points");

  AssertMacro(structGridOut->GetPointData()->GetArray(0)->GetTuple3(5)[2] == 5,
    structGridOut->GetClassName(), "Incorrect cell data");

  reader->Delete();
  return true;
}

int TestTransformPolyData(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    ReadFile<vtkXMLPolyDataReader>("Data/cow.vtp", argc, argv);

  double translation[3] = { -1.0, 2, 1.0 };
  double scaling[3] = { 0.2, 0.8, 0.9 };
  vtkSmartPointer<vtkPolyData> polyDataOut =
    vtkPolyData::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::Z));

  vtkPolyData* polyDataIn = vtkPolyData::SafeDownCast(reader->GetOutput());

  AssertMacro(polyDataOut->GetNumberOfPoints() == polyDataIn->GetNumberOfPoints(),
    polyDataOut->GetClassName(), "Incorrect number of points");
  AssertMacro(polyDataOut->GetNumberOfCells() == polyDataIn->GetNumberOfCells(),
    polyDataOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(polyDataOut->GetPoint(10)[0] == 0.16154038906097412109, polyDataOut->GetClassName(),
    "Incorrect points");

  vtkNew<vtkIdList> cellPtsIn;
  polyDataIn->GetPolys()->GetCell(0, cellPtsIn);
  vtkNew<vtkIdList> cellPtsOut;
  polyDataOut->GetPolys()->GetCell(0, cellPtsOut);

  AssertMacro(cellPtsIn->GetId(1) == cellPtsOut->GetId(1) && cellPtsOut->GetId(3) == 252,
    polyDataOut->GetClassName(), "Incorrect cells");

  reader->Delete();
  return true;
}

int TestTransformHyperTreeGrid3D(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLHyperTreeGridReader> reader =
    ReadFile<vtkXMLHyperTreeGridReader>("Data/HTG/shell_3d.htg", argc, argv);

  double translation[3] = { -2.1, 0.0, 1.2 };
  double scaling[3] = { 2.0, 0.5, 1.5 };
  vtkSmartPointer<vtkHyperTreeGrid> htgOut =
    vtkHyperTreeGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT90, vtkAxisAlignedTransformFilter::Axis::Y));

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  AssertMacro(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(), htgOut->GetClassName(),
    "Incorrect number of cells");

  AssertMacro(htgOut->GetYCoordinates()->GetTuple1(2) == 0.5, htgOut->GetClassName(),
    "Incorrect coordinates");

  AssertMacro(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(57)[1] ==
      scaling[1] *
        htgOut->GetCellData()->GetArray(htgOut->GetInterfaceNormalsName())->GetTuple3(6383)[1],
    htgOut->GetClassName(), "Incorrect normals");

  AssertMacro(
    htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(5)[1] ==
      0.47808688094430307203,
    htgOut->GetClassName(), "Incorrect intercepts");
  AssertMacro(
    htgOut->GetCellData()->GetArray(htgOut->GetInterfaceInterceptsName())->GetTuple3(5)[1] == 0.5,
    htgOut->GetClassName(), "Incorrect intercepts");

  AssertMacro(htgIn->GetCellData()->GetArray(0)->GetTuple1(57) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(6383),
    htgOut->GetClassName(), "Incorrect cells");

  reader->Delete();
  return true;
}

int TestTransformHyperTreeGrid2D(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLHyperTreeGridReader> reader =
    ReadFile<vtkXMLHyperTreeGridReader>("Data/HTG/donut_XZ_shift_2d.htg", argc, argv);

  double translation[3] = { 1.5, -1.0, 2.0 };
  double scaling[3] = { 1.2, 0.8, 1.0 };
  vtkSmartPointer<vtkHyperTreeGrid> htgOut =
    vtkHyperTreeGrid::SafeDownCast(Transform(reader->GetOutputPort(), translation, scaling,
      vtkAxisAlignedTransformFilter::Angle::ROT270, vtkAxisAlignedTransformFilter::Axis::Y));

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(reader->GetOutput());

  AssertMacro(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(), htgOut->GetClassName(),
    "Incorrect number of cells");

  AssertMacro(htgOut->GetXCoordinates()->GetTuple1(2) == 2.5, htgOut->GetClassName(),
    "Incorrect coordinates");

  AssertMacro(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(0)[1] ==
      scaling[1] *
        htgOut->GetCellData()->GetArray(htgOut->GetInterfaceNormalsName())->GetTuple3(93)[1],
    htgOut->GetClassName(), "Incorrect normals");

  AssertMacro(
    htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(15)[1] ==
      -0.37878773115802955029,
    htgOut->GetClassName(), "Incorrect intercepts");
  AssertMacro(
    htgOut->GetCellData()->GetArray(htgOut->GetInterfaceInterceptsName())->GetTuple3(55)[1] ==
      1.598185390529741845,
    htgOut->GetClassName(), "Incorrect intercepts");

  AssertMacro(htgIn->GetCellData()->GetArray(0)->GetTuple1(0) ==
      htgOut->GetCellData()->GetArray(0)->GetTuple1(93),
    htgOut->GetClassName(), "Incorrect cells");

  reader->Delete();
  return true;
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

  return testVal ? EXIT_SUCCESS : EXIT_FAILURE;
}
