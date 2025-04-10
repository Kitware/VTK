// SPDX-FileCopyrightText: Copyright (c) Kitware Inc.
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkXMLMultiBlockDataReader.h>

#include "vtkAxisAlignedReflectionFilter.h"
#include "vtkCellData.h"
#include "vtkDataAssembly.h"
#include "vtkExplicitStructuredGrid.h"
#include "vtkHyperTreeGrid.h"
#include "vtkImageData.h"
#include "vtkMatrix3x3.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPlane.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkRectilinearGrid.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridToExplicitStructuredGrid.h"
#include "vtkXMLHyperTreeGridReader.h"
#include "vtkXMLImageDataReader.h"
#include "vtkXMLPartitionedDataSetCollectionReader.h"
#include "vtkXMLPolyDataReader.h"
#include "vtkXMLRectilinearGridReader.h"
#include "vtkXMLStructuredGridReader.h"
#include "vtkXMLUnstructuredGridReader.h"

#define AssertMacro(b, data, reason)                                                               \
  if (!(b))                                                                                        \
  {                                                                                                \
    std::cerr << "Failed to reflect " << data << ": " << reason << std::endl;                      \
    return EXIT_FAILURE;                                                                           \
  }

#define ReadFileMacro(path, readerClass)                                                           \
  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, path);                         \
  vtkNew<readerClass> reader;                                                                      \
  reader->SetFileName(fileName);                                                                   \
  reader->Update();                                                                                \
  delete[] fileName;

struct PlaneParams
{
  double normal[3];
  double origin[3];
};

vtkSmartPointer<vtkPartitionedDataSetCollection> Reflect(vtkAlgorithmOutput* port, bool copyInput,
  bool flipAll, vtkAxisAlignedReflectionFilter::PlaneModes planeMode,
  const PlaneParams* planeParams = nullptr)
{
  vtkSmartPointer<vtkAxisAlignedReflectionFilter> reflect =
    vtkSmartPointer<vtkAxisAlignedReflectionFilter>::New();
  reflect->SetInputConnection(port);
  reflect->SetCopyInput(copyInput);
  reflect->SetReflectAllInputArrays(flipAll);

  reflect->SetPlaneMode(planeMode);
  if (planeMode == vtkAxisAlignedReflectionFilter::PLANE)
  {
    vtkNew<vtkPlane> plane;
    plane->SetNormal(planeParams->normal);
    plane->SetOrigin(planeParams->origin);
    plane->SetAxisAligned(true);
    plane->SetOffset(0);
    reflect->SetReflectionPlane(plane);
  }
  reflect->Update();

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    vtkPartitionedDataSetCollection::SafeDownCast(reflect->GetOutput());

  return output;
}

int TestUnstructuredGrid(int argc, char* argv[])
{
  ReadFileMacro("Data/can.vtu", vtkXMLUnstructuredGridReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MIN);

  vtkUnstructuredGrid* unstructGridIn =
    vtkUnstructuredGrid::SafeDownCast(output->GetPartition(0, 0));
  vtkUnstructuredGrid* unstructGridOut =
    vtkUnstructuredGrid::SafeDownCast(output->GetPartition(1, 0));

  AssertMacro(unstructGridOut->GetNumberOfPoints() == unstructGridIn->GetNumberOfPoints(),
    unstructGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(unstructGridOut->GetNumberOfCells() == unstructGridIn->GetNumberOfCells(),
    unstructGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(unstructGridOut->GetPoint(10)[1] == -3.055955171585083,
    unstructGridOut->GetClassName(), "Incorrect points");

  vtkNew<vtkIdList> ptsIn;
  unstructGridIn->GetCellPoints(5, ptsIn);
  vtkNew<vtkIdList> ptsOut;
  unstructGridOut->GetCellPoints(5, ptsOut);
  AssertMacro(ptsOut->GetId(0) == ptsIn->GetId(4) && ptsIn->GetId(4) == 20,
    unstructGridOut->GetClassName(), "Incorrect cell points");

  AssertMacro(unstructGridOut->GetPointData()->GetArray("ACCL")->GetTuple3(0)[0] == -2269740,
    unstructGridOut->GetClassName(), "Incorrect cell data");

  return EXIT_SUCCESS;
}

int TestImageData(int argc, char* argv[])
{
  ReadFileMacro("Data/scalars.vti", vtkXMLImageDataReader);

  PlaneParams planeParams;
  planeParams.normal[0] = 1.0;
  planeParams.normal[1] = 0.0;
  planeParams.normal[2] = 0.0;
  planeParams.origin[0] = 1.0;
  planeParams.origin[1] = 0.0;
  planeParams.origin[2] = 0.0;

  vtkSmartPointer<vtkPartitionedDataSetCollection> output = Reflect(
    reader->GetOutputPort(), false, false, vtkAxisAlignedReflectionFilter::PLANE, &planeParams);

  vtkImageData* imageData = vtkImageData::SafeDownCast(output->GetPartition(0, 0));

  AssertMacro(imageData->GetOrigin()[0] == 2 && imageData->GetOrigin()[1] == 0 &&
      imageData->GetOrigin()[2] == 0,
    imageData->GetClassName(), "Incorrect origin");
  AssertMacro(imageData->GetDirectionMatrix()->GetElement(0, 0) == -1 &&
      imageData->GetDirectionMatrix()->GetElement(1, 1) == 1 &&
      imageData->GetDirectionMatrix()->GetElement(2, 2) == 1,
    imageData->GetClassName(), "Incorrect direction matrix");
  AssertMacro(imageData->GetScalarComponentAsDouble(0, 0, 8, 0) == 8, imageData->GetClassName(),
    "Incorrect scalar component");

  return EXIT_SUCCESS;
}

int TestRectilinearGrid(int argc, char* argv[])
{
  ReadFileMacro("Data/rectGrid.vtr", vtkXMLRectilinearGridReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, false, vtkAxisAlignedReflectionFilter::Y_MAX);

  vtkRectilinearGrid* rectGridIn = vtkRectilinearGrid::SafeDownCast(output->GetPartition(0, 0));
  vtkRectilinearGrid* rectGridOut = vtkRectilinearGrid::SafeDownCast(output->GetPartition(1, 0));

  AssertMacro(rectGridOut->GetNumberOfPoints() == rectGridIn->GetNumberOfPoints(),
    rectGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(rectGridOut->GetNumberOfCells() == rectGridIn->GetNumberOfCells(),
    rectGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(rectGridOut->GetYCoordinates()->GetTuple1(3) == 3.5065579414367676,
    rectGridOut->GetClassName(), "Incorrect Y coordinates");

  AssertMacro(rectGridOut->GetCellData()->GetArray(0)->GetTuple3(5)[0] == 6258,
    rectGridOut->GetClassName(), "Incorrect cell data");

  return EXIT_SUCCESS;
}

int TestExplicitStructuredGrid(int argc, char* argv[])
{
  ReadFileMacro("Data/explicitStructuredGrid.vtu", vtkXMLUnstructuredGridReader);
  vtkSmartPointer<vtkUnstructuredGridToExplicitStructuredGrid> UgToEsg =
    vtkSmartPointer<vtkUnstructuredGridToExplicitStructuredGrid>::New();
  UgToEsg->SetInputConnection(reader->GetOutputPort());
  UgToEsg->SetWholeExtent(0, 5, 0, 13, 0, 3);
  UgToEsg->SetInputArrayToProcess(0, 0, 0, 1, "BLOCK_I");
  UgToEsg->SetInputArrayToProcess(1, 0, 0, 1, "BLOCK_J");
  UgToEsg->SetInputArrayToProcess(2, 0, 0, 1, "BLOCK_K");
  UgToEsg->Update();

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(UgToEsg->GetOutputPort(), true, false, vtkAxisAlignedReflectionFilter::Z_MIN);

  vtkExplicitStructuredGrid* esgIn =
    vtkExplicitStructuredGrid::SafeDownCast(output->GetPartition(0, 0));
  vtkExplicitStructuredGrid* esgOut =
    vtkExplicitStructuredGrid::SafeDownCast(output->GetPartition(1, 0));

  AssertMacro(esgOut->GetNumberOfPoints() == esgIn->GetNumberOfPoints(), esgOut->GetClassName(),
    "Incorrect number of points");
  AssertMacro(esgOut->GetNumberOfCells() == esgIn->GetNumberOfCells(), esgOut->GetClassName(),
    "Incorrect number of cells");

  AssertMacro(
    esgOut->GetPoint(0)[2] == 1419.0244140625, esgOut->GetClassName(), "Incorrect points");

  AssertMacro(esgOut->GetCellPoints(5)[0] == esgIn->GetCellPoints(5)[3], esgOut->GetClassName(),
    "Incorrect cell points");

  return EXIT_SUCCESS;
}

int TestStructuredGrid(int argc, char* argv[])
{
  ReadFileMacro("Data/structGrid.vts", vtkXMLStructuredGridReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::Z_MAX);

  vtkStructuredGrid* structGridIn = vtkStructuredGrid::SafeDownCast(output->GetPartition(0, 0));
  vtkStructuredGrid* structGridOut = vtkStructuredGrid::SafeDownCast(output->GetPartition(1, 0));

  AssertMacro(structGridOut->GetNumberOfPoints() == structGridIn->GetNumberOfPoints(),
    structGridOut->GetClassName(), "Incorrect number of points");
  AssertMacro(structGridOut->GetNumberOfCells() == structGridIn->GetNumberOfCells(),
    structGridOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(structGridOut->GetPoint(1429)[2] == 2.7999999523162842, structGridOut->GetClassName(),
    "Incorrect points");

  AssertMacro(structGridOut->GetPointData()->GetArray(0)->GetTuple3(5)[2] == -1567,
    structGridOut->GetClassName(), "Incorrect cell data");

  return EXIT_SUCCESS;
}

int TestPolyData(int argc, char* argv[])
{
  ReadFileMacro("Data/cow.vtp", vtkXMLPolyDataReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MAX);

  vtkPolyData* polyDataIn = vtkPolyData::SafeDownCast(output->GetPartition(0, 0));
  vtkPolyData* polyDataOut = vtkPolyData::SafeDownCast(output->GetPartition(1, 0));

  AssertMacro(polyDataOut->GetNumberOfPoints() == polyDataIn->GetNumberOfPoints(),
    polyDataOut->GetClassName(), "Incorrect number of points");
  AssertMacro(polyDataOut->GetNumberOfCells() == polyDataIn->GetNumberOfCells(),
    polyDataOut->GetClassName(), "Incorrect number of cells");

  AssertMacro(polyDataOut->GetPoint(10)[0] == 9.6565790176391602, polyDataOut->GetClassName(),
    "Incorrect points");

  vtkNew<vtkIdList> cellPtsIn;
  polyDataIn->GetPolys()->GetCell(0, cellPtsIn);
  vtkNew<vtkIdList> cellPtsOut;
  polyDataOut->GetPolys()->GetCell(0, cellPtsOut);

  AssertMacro(cellPtsIn->GetId(1) == cellPtsOut->GetId(3) && cellPtsOut->GetId(3) == 251,
    polyDataOut->GetClassName(), "Incorrect cells");

  return EXIT_SUCCESS;
}

int TestHyperTreeGrid(int argc, char* argv[])
{
  ReadFileMacro("Data/HTG/shell_3d.htg", vtkXMLHyperTreeGridReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::Y_MIN);

  vtkHyperTreeGrid* htgIn = vtkHyperTreeGrid::SafeDownCast(output->GetPartitionAsDataObject(0, 0));
  vtkHyperTreeGrid* htgOut = vtkHyperTreeGrid::SafeDownCast(output->GetPartitionAsDataObject(1, 0));

  AssertMacro(htgOut->GetNumberOfCells() == htgIn->GetNumberOfCells(), htgOut->GetClassName(),
    "Incorrect number of cells");

  AssertMacro(
    htgOut->GetYCoordinates()->GetTuple1(2) == -3, htgOut->GetClassName(), "Incorrect coordinates");

  AssertMacro(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(5)[1] ==
      0.93704257133163638,
    htgOut->GetClassName(), "Incorrect normals");
  AssertMacro(
    htgOut->GetCellData()->GetArray("outNormals")->GetTuple3(5)[1] == -0.93704257133163638,
    htgOut->GetClassName(), "Incorrect normals");

  AssertMacro(
    htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(5)[1] ==
      0.47808688094430307,
    htgOut->GetClassName(), "Incorrect intercepts");
  AssertMacro(
    htgOut->GetCellData()->GetArray("outIntercepts")->GetTuple3(5)[1] == -1.3959982617189697,
    htgOut->GetClassName(), "Incorrect intercepts");

  AssertMacro(
    htgIn->GetCellData()->GetArray(htgIn->GetInterfaceInterceptsName())->GetTuple3(5)[0] ==
      1.0780868809443032,
    htgOut->GetClassName(), "Incorrect intercepts");
  AssertMacro(
    htgOut->GetCellData()->GetArray("outIntercepts")->GetTuple3(5)[0] == -0.79599826171896959,
    htgOut->GetClassName(), "Incorrect intercepts");

  AssertMacro(htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(5)[1] ==
        -htgOut->GetCellData()->GetArray("outNormals")->GetTuple3(5)[1] &&
      htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(5)[0] ==
        htgOut->GetCellData()->GetArray("outNormals")->GetTuple3(5)[0] &&
      htgIn->GetCellData()->GetArray(htgIn->GetInterfaceNormalsName())->GetTuple3(5)[2] ==
        htgOut->GetCellData()->GetArray("outNormals")->GetTuple3(5)[2],
    htgOut->GetClassName(), "Incorrect normals");

  return EXIT_SUCCESS;
}

int TestPartitionedDataSetCollection(int argc, char* argv[])
{
  ReadFileMacro("Data/sphereMirror.vtpc", vtkXMLPartitionedDataSetCollectionReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MIN);

  vtkDataAssembly* assembly = output->GetDataAssembly();

  AssertMacro(
    strcmp(assembly->GetRootNodeName(), "Root") == 0, output->GetClassName(), "Incorrect assembly");

  int rootId = assembly->GetRootNode();

  int inputId = assembly->GetChild(rootId, 0);
  int reflectionId = assembly->GetChild(rootId, 1);
  AssertMacro(strcmp(assembly->GetNodeName(inputId), "Input") == 0, output->GetClassName(),
    "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(reflectionId), "Reflection") == 0,
    output->GetClassName(), "Incorrect assembly");

  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(inputId, 0)), "Input") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(inputId, 1)), "Reflection") == 0,
    output->GetClassName(), "Incorrect assembly");

  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, 0)), "Input") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, 1)), "Reflection") == 0,
    output->GetClassName(), "Incorrect assembly");

  AssertMacro(output->GetNumberOfPartitionedDataSets() == 4, output->GetClassName(),
    "Incorrect number of partitioned data sets");

  for (unsigned int i = 0; i < output->GetNumberOfPartitionedDataSets(); i++)
  {
    AssertMacro(output->GetNumberOfPartitions(i) == 1, output->GetClassName(),
      "Incorrect number of partitions");
  }

  return EXIT_SUCCESS;
}

int TestMultiBlockMultiPiece(int argc, char* argv[])
{
  ReadFileMacro("Data/mb-of-mps.vtm", vtkXMLMultiBlockDataReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MIN);

  vtkDataAssembly* assembly = output->GetDataAssembly();

  int rootId = assembly->GetRootNode();

  int inputId = assembly->GetChild(rootId, 0);
  int reflectionId = assembly->GetChild(rootId, 1);

  AssertMacro(strcmp(assembly->GetNodeName(inputId), "Input") == 0, output->GetClassName(),
    "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(reflectionId), "Reflection") == 0,
    output->GetClassName(), "Incorrect assembly");

  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(inputId, 0)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(inputId, 1)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(inputId, 2)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, 0)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, 1)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, 2)), "Composite") == 0,
    output->GetClassName(), "Incorrect assembly");

  return EXIT_SUCCESS;
}

int TestMultiBlockOnlyDataSets(int argc, char* argv[])
{
  ReadFileMacro("Data/distTest.vtm", vtkXMLMultiBlockDataReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MIN);

  vtkDataAssembly* assembly = output->GetDataAssembly();

  int rootId = assembly->GetRootNode();

  int inputId = assembly->GetChild(rootId, 0);
  int reflectionId = assembly->GetChild(rootId, 1);

  AssertMacro(strcmp(assembly->GetNodeName(inputId), "Input") == 0, output->GetClassName(),
    "Incorrect assembly");
  AssertMacro(strcmp(assembly->GetNodeName(reflectionId), "Reflection") == 0,
    output->GetClassName(), "Incorrect assembly");

  for (int i = 0; i < 10; i++)
  {
    std::string inputCorrect = "Input_" + std::to_string(i);
    AssertMacro(
      strcmp(assembly->GetNodeName(assembly->GetChild(inputId, i)), inputCorrect.c_str()) == 0,
      output->GetClassName(), "Incorrect assembly");
    std::string reflectionCorrect = "Reflection_" + std::to_string(i);
    AssertMacro(strcmp(assembly->GetNodeName(assembly->GetChild(reflectionId, i)),
                  reflectionCorrect.c_str()) == 0,
      output->GetClassName(), "Incorrect assembly");
  }

  return EXIT_SUCCESS;
}

int TestMultiBlockEmptyPiece(int argc, char* argv[])
{
  ReadFileMacro("Data/mb_single_piece_empty_data.vtm", vtkXMLMultiBlockDataReader);

  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    Reflect(reader->GetOutputPort(), true, true, vtkAxisAlignedReflectionFilter::X_MIN);

  AssertMacro(output->GetNumberOfPartitionedDataSets() == 2, output->GetClassName(),
    "Incorrect number of partitioned datasets")

    return EXIT_SUCCESS;
}

// This function tests all the input types, and each input type will test a different plane mode.
int TestAxisAlignedReflectionFilter(int argc, char* argv[])
{
  return TestUnstructuredGrid(argc, argv) || TestImageData(argc, argv) ||
    TestRectilinearGrid(argc, argv) || TestExplicitStructuredGrid(argc, argv) ||
    TestStructuredGrid(argc, argv) || TestPolyData(argc, argv) || TestHyperTreeGrid(argc, argv) ||
    TestPartitionedDataSetCollection(argc, argv) || TestMultiBlockMultiPiece(argc, argv) ||
    TestMultiBlockOnlyDataSets(argc, argv) || TestMultiBlockEmptyPiece(argc, argv);
}
