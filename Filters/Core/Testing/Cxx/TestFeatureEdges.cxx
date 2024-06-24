// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#define _USE_MATH_DEFINES

#include "vtkAppendPolyData.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkFeatureEdges.h"
#include "vtkGhostCellsGenerator.h"
#include "vtkImageData.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <cmath>

//----------------------------------------------------------------------------
double GetGridValue(double i, double j, double k)
{
  return std::cos(i * M_PI / 5 + 1.0) * std::sin(j * M_PI / 5 + 1.0) *
    std::exp(-(k - 1.0) * (k - 1.0) / 11.0);
}

//----------------------------------------------------------------------------
void FillImage(vtkImageData* image)
{
  const int* extent = image->GetExtent();
  vtkNew<vtkDoubleArray> array;
  array->SetNumberOfComponents(1);
  array->SetNumberOfTuples(
    (extent[1] - extent[0] + 1) * (extent[3] - extent[2] + 1) * (extent[5] - extent[4] + 1));
  array->SetName("Grid_data");
  image->GetPointData()->AddArray(array);
  for (int k = extent[4]; k <= extent[5]; ++k)
  {
    for (int j = extent[2]; j <= extent[3]; ++j)
    {
      for (int i = extent[0]; i <= extent[1]; ++i)
      {
        int ijk[3] = { i, j, k };
        vtkIdType cellId = vtkStructuredData::ComputePointIdForExtent(extent, ijk);
        array->SetValue(cellId, GetGridValue(i, j, k));
      }
    }
  }
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> Convert1DImageToPolyData(vtkImageData* input)
{
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();

  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetNumberOfPoints(input->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < points->GetNumberOfPoints(); ++pointId)
  {
    points->SetPoint(pointId, input->GetPoint(pointId));
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();

  using ArrayType32 = vtkCellArray::ArrayType32;
  vtkNew<vtkCellArray> lines;
  lines->Use32BitStorage();

  ArrayType32* offsets = lines->GetOffsetsArray32();
  offsets->SetNumberOfValues(numberOfCells + 1);
  for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
  {
    offsets->SetValue(id, 2 * id);
  }

  ArrayType32* connectivity = lines->GetConnectivityArray32();
  connectivity->SetNumberOfValues(numberOfCells * 2);

  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    connectivity->SetValue(2 * cellId, cellId);
    connectivity->SetValue(2 * cellId + 1, cellId + 1);
  }

  output->SetLines(lines);
  output->GetPointData()->ShallowCopy(input->GetPointData());

  return output;
}

//----------------------------------------------------------------------------
vtkSmartPointer<vtkPolyData> Convert2DImageToPolyData(vtkImageData* input)
{
  vtkSmartPointer<vtkPolyData> output = vtkSmartPointer<vtkPolyData>::New();

  vtkNew<vtkPoints> points;
  output->SetPoints(points);
  points->SetNumberOfPoints(input->GetNumberOfPoints());
  for (vtkIdType pointId = 0; pointId < points->GetNumberOfPoints(); ++pointId)
  {
    points->SetPoint(pointId, input->GetPoint(pointId));
  }

  vtkIdType numberOfCells = input->GetNumberOfCells();

  using ArrayType32 = vtkCellArray::ArrayType32;
  vtkNew<vtkCellArray> polys, strips;
  polys->Use32BitStorage();
  strips->Use32BitStorage();

  {
    ArrayType32* offsets = polys->GetOffsetsArray32();
    offsets->SetNumberOfValues(numberOfCells / 2 + numberOfCells % 2 + 1);
    for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
    {
      offsets->SetValue(id, 4 * id);
    }
  }
  {
    ArrayType32* offsets = strips->GetOffsetsArray32();
    offsets->SetNumberOfValues(numberOfCells / 2 + 1);
    for (vtkIdType id = 0; id < offsets->GetNumberOfValues(); ++id)
    {
      offsets->SetValue(id, 4 * id);
    }
  }

  const int* extent = input->GetExtent();
  constexpr vtkIdType pixel2hexMap[4] = { 0, 1, 3, 2 };

  ArrayType32* polyConnectivity = polys->GetConnectivityArray32();
  polyConnectivity->SetNumberOfValues((polys->GetOffsetsArray()->GetNumberOfValues() - 1) * 4);
  ArrayType32* stripConnectivity = strips->GetConnectivityArray32();
  stripConnectivity->SetNumberOfValues((strips->GetOffsetsArray()->GetNumberOfValues() - 1) * 4);

  int ijkCell[3] = { 0, 0, 0 };
  int ijkPoint[3] = { 0, 0, 0 };
  vtkIdType polyConnectivityId = 0, stripConnectivityId = 0;

  for (vtkIdType cellId = 0; cellId < numberOfCells; ++cellId)
  {
    vtkStructuredData::ComputeCellStructuredCoordsForExtent(cellId, extent, ijkCell);

    if (!(cellId % 2))
    {
      int counter = 0;
      for (ijkPoint[0] = ijkCell[0]; ijkPoint[0] <= ijkCell[0] + 1; ++ijkPoint[0])
      {
        for (ijkPoint[1] = ijkCell[1]; ijkPoint[1] <= ijkCell[1] + 1; ++ijkPoint[1], ++counter)
        {
          vtkIdType id = vtkStructuredData::ComputePointIdForExtent(extent, ijkPoint);
          polyConnectivity->SetValue(polyConnectivityId + pixel2hexMap[counter], id);
        }
      }
      polyConnectivityId += 4;
    }
    else
    {
      int counter = 0;
      for (ijkPoint[0] = ijkCell[0]; ijkPoint[0] <= ijkCell[0] + 1; ++ijkPoint[0])
      {
        for (ijkPoint[1] = ijkCell[1]; ijkPoint[1] <= ijkCell[1] + 1; ++ijkPoint[1], ++counter)
        {
          vtkIdType id = vtkStructuredData::ComputePointIdForExtent(extent, ijkPoint);
          stripConnectivity->SetValue(stripConnectivityId + counter, id);
        }
      }
      stripConnectivityId += 4;
    }
  }

  output->SetStrips(strips);
  output->SetPolys(polys);

  output->GetPointData()->ShallowCopy(input->GetPointData());

  return output;
}

//----------------------------------------------------------------------------
bool TestMixedTypes()
{
  int maxExtent = 5;
  int extent[6] = { 0, maxExtent, 0, maxExtent, 0, 0 };
  vtkNew<vtkImageData> image;
  image->SetExtent(extent);
  FillImage(image);

  vtkSmartPointer<vtkPolyData> pd = Convert2DImageToPolyData(image);

  int extentLine[6] = { 0, maxExtent, 1, 1, 1, 1 };
  vtkNew<vtkImageData> lineImage;
  lineImage->SetExtent(extentLine);
  FillImage(lineImage);

  vtkSmartPointer<vtkPolyData> pdLines = Convert1DImageToPolyData(lineImage);

  vtkNew<vtkAppendPolyData> appendPD;
  appendPD->AddInputData(pd);
  appendPD->AddInputData(pdLines);
  appendPD->Update();
  pd = appendPD->GetOutput();

  vtkNew<vtkPointDataToCellData> imagePointToCell;
  imagePointToCell->SetInputData(image);
  imagePointToCell->Update();

  vtkNew<vtkPointDataToCellData> lineImagePointToCell;
  lineImagePointToCell->SetInputData(lineImage);
  lineImagePointToCell->Update();

  vtkNew<vtkPointDataToCellData> pdPointToCell;
  pdPointToCell->SetInputData(pd);
  pdPointToCell->Update();

  vtkImageData* imageRef = vtkImageData::SafeDownCast(imagePointToCell->GetOutputDataObject(0));
  vtkImageData* lineImageRef =
    vtkImageData::SafeDownCast(lineImagePointToCell->GetOutputDataObject(0));

  vtkDataArray* imageRefArray =
    vtkArrayDownCast<vtkDataArray>(imageRef->GetCellData()->GetAbstractArray(0));
  vtkDataArray* lineRefArray =
    vtkArrayDownCast<vtkDataArray>(lineImageRef->GetCellData()->GetAbstractArray(0));

  vtkNew<vtkFeatureEdges> edges;
  edges->BoundaryEdgesOn();
  edges->FeatureEdgesOff();
  edges->NonManifoldEdgesOff();
  edges->PassLinesOn();
  edges->ColoringOff();
  edges->SetInputConnection(pdPointToCell->GetOutputPort());
  edges->Update();

  vtkPolyData* out = vtkPolyData::SafeDownCast(edges->GetOutputDataObject(0));

  if (out->GetNumberOfLines() != maxExtent * 5)
  {
    vtkLog(ERROR,
      "Feature edges generated the wrong number of output lines: it generated "
        << out->GetNumberOfLines() << " lines instead of " << (maxExtent * 5));
    return false;
  }

  vtkNew<vtkIdList> outputMapToGrid;
  outputMapToGrid->SetNumberOfIds(out->GetNumberOfLines() - lineImage->GetNumberOfCells());

  // The mapping from the output cell data array to the original one is not trivial, because
  // vtkPolyData sorts its cells in a certain way (vertices, lines, polys, then strips).
  outputMapToGrid->SetId(0, 0);
  outputMapToGrid->SetId(1, 0);
  outputMapToGrid->SetId(2, 2);
  outputMapToGrid->SetId(3, 4);
  outputMapToGrid->SetId(4, 4);
  outputMapToGrid->SetId(5, 10);
  outputMapToGrid->SetId(6, 14);
  outputMapToGrid->SetId(7, 20);
  outputMapToGrid->SetId(8, 20);
  outputMapToGrid->SetId(9, 22);
  outputMapToGrid->SetId(10, 24);
  outputMapToGrid->SetId(11, 24);
  outputMapToGrid->SetId(12, 1);
  outputMapToGrid->SetId(13, 3);
  outputMapToGrid->SetId(14, 5);
  outputMapToGrid->SetId(15, 9);
  outputMapToGrid->SetId(16, 15);
  outputMapToGrid->SetId(17, 19);
  outputMapToGrid->SetId(18, 21);
  outputMapToGrid->SetId(19, 23);

  vtkDataArray* outArray = vtkArrayDownCast<vtkDataArray>(out->GetCellData()->GetAbstractArray(0));

  for (vtkIdType id = 0; id < out->GetNumberOfCells(); ++id)
  {
    bool error = false;
    // 1D grid
    if (id < maxExtent)
    {
      if (lineRefArray->GetTuple1(id) != outArray->GetTuple1(id))
      {
        error = true;
      }
    }
    // 2D grid
    else
    {
      if (std::abs(
            imageRefArray->GetTuple1(outputMapToGrid->GetId(id - lineImage->GetNumberOfCells())) -
            outArray->GetTuple1(id)) > 0.001)
      {
        error = true;
      }
    }

    if (error)
    {
      vtkLog(ERROR, "Error when copying cell data into output when using vtkFeatureEdge.");
      return false;
    }
  }

  // Creating a second image next to the first image.
  // We're going to test how the feature edge filter reacts to ghost cells.
  vtkNew<vtkImageData> image2;
  int extent2[6] = { -maxExtent, 0, 0, maxExtent, 0, 0 };
  image2->SetExtent(extent2);
  FillImage(image2);

  vtkSmartPointer<vtkPolyData> imagePD = Convert2DImageToPolyData(image);
  vtkSmartPointer<vtkPolyData> imagePD2 = Convert2DImageToPolyData(image2);

  vtkNew<vtkPartitionedDataSet> pds;
  pds->SetNumberOfPartitions(2);
  pds->SetPartition(0, imagePD);
  pds->SetPartition(1, imagePD2);

  vtkNew<vtkGhostCellsGenerator> ghostGenerator;
  ghostGenerator->SetInputData(pds);
  ghostGenerator->SetNumberOfGhostLayers(1);
  ghostGenerator->BuildIfRequiredOff();
  ghostGenerator->Update();

  vtkPolyData* pdWithGhosts = vtkPolyData::SafeDownCast(
    vtkPartitionedDataSet::SafeDownCast(ghostGenerator->GetOutputDataObject(0))->GetPartition(0));

  edges->SetInputData(pdWithGhosts);
  edges->RemoveGhostInterfacesOn();
  edges->Update();
  out = vtkPolyData::SafeDownCast(edges->GetOutputDataObject(0));
  if (out->GetNumberOfCells() != maxExtent * 3)
  {
    vtkLog(ERROR,
      "Feature edges failed at generating edges with ghost cells. it generated "
        << out->GetNumberOfCells() << " cells instead of " << (maxExtent * 3));
  }

  edges->RemoveGhostInterfacesOff();
  edges->Update();
  out = vtkPolyData::SafeDownCast(edges->GetOutputDataObject(0));
  if (out->GetNumberOfCells() != maxExtent * 4)
  {
    vtkLog(ERROR,
      "Feature edges failed at generating edges with ghost cells. it generated "
        << out->GetNumberOfCells() << " cells instead of " << (maxExtent * 4));
  }

  return true;
}

//----------------------------------------------------------------------------
bool TestDegenerateCell()
{
  // Create two quads, the second one being degenerated into a triangle
  // This test make sure that the edge between the two cells is removed by vtkFeatureEdges

  vtkSmartPointer<vtkPolyData> polydata = vtkSmartPointer<vtkPolyData>::New();
  polydata->AllocateExact(2, 4);

  auto points = vtkSmartPointer<vtkPoints>::New();
  points->InsertNextPoint(0, 0, 0);
  points->InsertNextPoint(1, 0, 0);
  points->InsertNextPoint(0, 1, 0);
  points->InsertNextPoint(1, 1, 0);
  points->InsertNextPoint(1.5, 0.5, 0);
  polydata->SetPoints(points);

  vtkIdType connectivityQuad1[4] = { 0, 1, 3, 2 };
  vtkIdType connectivityQuad2[4] = { 4, 3, 1, 1 };
  polydata->InsertNextCell(VTK_QUAD, 4, connectivityQuad1);
  polydata->InsertNextCell(VTK_QUAD, 4, connectivityQuad2);

  vtkNew<vtkFeatureEdges> edges;
  edges->BoundaryEdgesOn();
  edges->FeatureEdgesOn();
  edges->NonManifoldEdgesOff();
  edges->PassLinesOff();
  edges->ColoringOff();
  edges->ManifoldEdgesOff();
  edges->SetInputData(polydata);
  edges->Update();

  vtkPolyData* out = vtkPolyData::SafeDownCast(edges->GetOutputDataObject(0));

  if (out->GetNumberOfLines() != 5)
  {
    vtkLog(ERROR,
      "Feature edges generated the wrong number of output lines: it generated "
        << out->GetNumberOfLines() << " lines instead of " << 5);
    return false;
  }
  return true;
}

//----------------------------------------------------------------------------
void InitializePolyData(vtkPolyData* polyData, int dataType)
{
  vtkSmartPointer<vtkPoints> points = vtkSmartPointer<vtkPoints>::New();
  points->SetDataType(dataType);
  points->InsertNextPoint(-1.40481710, -0.03868163, -1.01241910);
  points->InsertNextPoint(-1.41186166, 0.29086590, 0.96023101);
  points->InsertNextPoint(-0.13218975, -1.22439861, 1.21793830);
  points->InsertNextPoint(-0.12514521, -1.55394614, -0.75471181);
  points->InsertNextPoint(0.13218975, 1.22439861, -1.21793830);
  points->InsertNextPoint(0.12514521, 1.55394614, 0.75471181);
  points->InsertNextPoint(1.40481710, 0.03868163, 1.01241910);
  points->InsertNextPoint(1.41186166, -0.29086590, -0.96023101);
  points->Squeeze();

  polyData->SetPoints(points);

  vtkSmartPointer<vtkCellArray> verts = vtkSmartPointer<vtkCellArray>::New();
  verts->InsertNextCell(8);
  for (unsigned int i = 0; i < 8; ++i)
  {
    verts->InsertCellPoint(i);
  }
  verts->Squeeze();

  polyData->SetVerts(verts);

  vtkSmartPointer<vtkCellArray> polys = vtkSmartPointer<vtkCellArray>::New();
  vtkIdType pointIds[3];
  pointIds[0] = 0;
  pointIds[1] = 1;
  pointIds[2] = 2;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 2;
  pointIds[2] = 3;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 3;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 4;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 5;
  pointIds[2] = 1;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 0;
  pointIds[1] = 7;
  pointIds[2] = 4;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 2;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 1;
  pointIds[1] = 6;
  pointIds[2] = 5;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 2;
  pointIds[1] = 3;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 3;
  pointIds[1] = 7;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 5;
  pointIds[2] = 6;
  polys->InsertNextCell(3, pointIds);
  pointIds[0] = 4;
  pointIds[1] = 6;
  pointIds[2] = 7;
  polys->InsertNextCell(3, pointIds);
  polys->Squeeze();

  polyData->SetPolys(polys);
}

//----------------------------------------------------------------------------
int FeatureEdges(int dataType, int outputPointsPrecision)
{
  vtkSmartPointer<vtkPolyData> inputPolyData = vtkSmartPointer<vtkPolyData>::New();
  InitializePolyData(inputPolyData, dataType);

  vtkSmartPointer<vtkFeatureEdges> featureEdges = vtkSmartPointer<vtkFeatureEdges>::New();
  featureEdges->SetOutputPointsPrecision(outputPointsPrecision);
  featureEdges->SetInputData(inputPolyData);

  featureEdges->Update();

  vtkSmartPointer<vtkPolyData> outputPolyData = featureEdges->GetOutput();
  vtkSmartPointer<vtkPoints> points = outputPolyData->GetPoints();

  return points->GetDataType();
}

//----------------------------------------------------------------------------
int TestFeatureEdges(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  if (!TestMixedTypes())
  {
    return EXIT_FAILURE;
  }

  if (!TestDegenerateCell())
  {
    return EXIT_FAILURE;
  }

  int dataType = FeatureEdges(VTK_FLOAT, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FeatureEdges(VTK_DOUBLE, vtkAlgorithm::DEFAULT_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FeatureEdges(VTK_FLOAT, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FeatureEdges(VTK_DOUBLE, vtkAlgorithm::SINGLE_PRECISION);

  if (dataType != VTK_FLOAT)
  {
    return EXIT_FAILURE;
  }

  dataType = FeatureEdges(VTK_FLOAT, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  dataType = FeatureEdges(VTK_DOUBLE, vtkAlgorithm::DOUBLE_PRECISION);

  if (dataType != VTK_DOUBLE)
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

#undef _USE_MATH_DEFINES
