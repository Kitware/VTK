// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include <vtkCellData.h>
#include <vtkCellDataToPointData.h>
#include <vtkDataArray.h>
#include <vtkDataSet.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkDataSetTriangleFilter.h>
#include <vtkDoubleArray.h>
#include <vtkImageData.h>
#include <vtkIntArray.h>
#include <vtkPointData.h>
#include <vtkPointDataToCellData.h>
#include <vtkPoints.h>
#include <vtkPolyData.h>
#include <vtkPolyDataMapper.h>
#include <vtkRTAnalyticSource.h>
#include <vtkSmartPointer.h>
#include <vtkStringArray.h>
#include <vtkTestErrorObserver.h>
#include <vtkTestUtilities.h>
#include <vtkThreshold.h>
#include <vtkUniformGrid.h>
#include <vtkUnstructuredGrid.h>

#include <cmath>
#include <iostream>

// Replaces the old `PCellDataToPointData` test
int TestCellDataToPointDataPieceInvariant()
{
  int numberOfPieces = 2;

  vtkNew<vtkRTAnalyticSource> wavelet;
  vtkNew<vtkPointDataToCellData> pd2cd;
  vtkNew<vtkCellDataToPointData> cd2pd;
  vtkNew<vtkDataSetSurfaceFilter> toPolyData;
  vtkNew<vtkPolyDataMapper> mapper;

  pd2cd->SetInputConnection(wavelet->GetOutputPort());
  cd2pd->SetInputConnection(pd2cd->GetOutputPort());
  cd2pd->SetPieceInvariant(true);
  toPolyData->SetInputConnection(cd2pd->GetOutputPort());

  mapper->SetInputConnection(toPolyData->GetOutputPort());
  mapper->SetNumberOfPieces(numberOfPieces);

  int retVal = EXIT_SUCCESS;
  for (int i = 0; i < numberOfPieces; ++i)
  {
    mapper->SetPiece(i);
    mapper->Update();

    vtkIdType correct = 5292;
    if (vtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints() != correct)
    {
      std::cerr << "Wrong number of grid points on piece " << i << ". Should be " << correct
                << " but is " << vtkDataSet::SafeDownCast(cd2pd->GetOutput())->GetNumberOfPoints()
                << std::endl;
      retVal = EXIT_FAILURE;
    }
  }

  return retVal;
}

bool CheckValue(const char* test, double value, double expected)
{
  if (std::abs(value - expected) > 1e-5)
  {
    std::cerr << "WeightedAverage test: " << test << ": expected " << expected << ", got " << value
              << std::endl;
    return false;
  }
  return true;
}

// Weighted average of the cell data `name` of `input` at the point `pointId`, with the weight
// array "Weights".
double WeightedValue(vtkDataSet* input, const char* name, vtkIdType pointId,
  int contributingCellOption = vtkCellDataToPointData::All)
{
  vtkNew<vtkCellDataToPointData> cellToPoint;
  cellToPoint->SetInputData(input);
  cellToPoint->SetContributingCellOption(contributingCellOption);
  cellToPoint->SetEnableWeightedAverage(true);
  cellToPoint->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Weights");
  cellToPoint->Update();
  vtkDataArray* values = cellToPoint->GetOutput()->GetPointData()->GetArray(name);
  return values ? values->GetTuple1(pointId) : VTK_DOUBLE_MAX;
}

// Tests EnableWeightedAverage and SetInputArrayToProcess with cell weight array
int TestWeightedAverage()
{
  vtkNew<vtkPolyData> polyData;
  vtkNew<vtkPoints> points;
  points->InsertNextPoint(0, 0, 0);  // Point 0 (shared by both triangles)
  points->InsertNextPoint(1, 0, 0);  // Point 1
  points->InsertNextPoint(0, 1, 0);  // Point 2
  points->InsertNextPoint(-1, 0, 0); // Point 3
  polyData->SetPoints(points);

  vtkNew<vtkCellArray> polys;
  vtkIdType tri1[3] = { 0, 1, 2 };
  vtkIdType tri2[3] = { 0, 2, 3 };
  polys->InsertNextCell(3, tri1); // Cell 0
  polys->InsertNextCell(3, tri2); // Cell 1
  polyData->SetPolys(polys);

  // Cell scalar values to interpolate
  vtkNew<vtkDoubleArray> cellScalars;
  cellScalars->SetName("Scalar");
  cellScalars->InsertNextValue(10.0); // Cell 0 scalar
  cellScalars->InsertNextValue(20.0); // Cell 1 scalar
  polyData->GetCellData()->AddArray(cellScalars);

  // Integer values, whose weighted sums must not be truncated
  vtkNew<vtkIntArray> cellIntegers;
  cellIntegers->SetName("Integer");
  cellIntegers->InsertNextValue(10);
  cellIntegers->InsertNextValue(30);
  polyData->GetCellData()->AddArray(cellIntegers);

  // Weight array
  vtkNew<vtkDoubleArray> weights;
  weights->SetName("Weights");
  weights->InsertNextValue(0.25); // Cell 0 weight
  weights->InsertNextValue(0.75); // Cell 1 weight
  polyData->GetCellData()->AddArray(weights);

  // 1. Test unweighted mode (EnableWeightedAverage = false, default)
  // Expected value at Point 0: (10.0 + 20.0) / 2 = 15.0
  vtkNew<vtkCellDataToPointData> unweightedC2P;
  unweightedC2P->SetInputData(polyData);
  unweightedC2P->Update();

  vtkDataArray* unweightedPointScalars =
    unweightedC2P->GetOutput()->GetPointData()->GetArray("Scalar");
  if (!unweightedPointScalars)
  {
    std::cerr << "WeightedAverage test: Unweighted point scalars missing." << std::endl;
    return EXIT_FAILURE;
  }
  if (!CheckValue("unweighted", unweightedPointScalars->GetTuple1(0), 15.0))
  {
    return EXIT_FAILURE;
  }

  // 2. Test weighted average mode (EnableWeightedAverage = true), with all the contributing cell
  // options. Expected value at Point 0: 10.0 * 0.25 + 20.0 * 0.75 = 17.5, and 10 * 0.25 + 30 *
  // 0.75 = 25 for the integers. Point 1 only uses cell 0.
  for (int option : { vtkCellDataToPointData::All, vtkCellDataToPointData::Patch,
         vtkCellDataToPointData::DataSetMax })
  {
    if (!CheckValue("weighted", WeightedValue(polyData, "Scalar", 0, option), 17.5) ||
      !CheckValue("weighted integers", WeightedValue(polyData, "Integer", 0, option), 25.0) ||
      !CheckValue("single cell", WeightedValue(polyData, "Scalar", 1, option), 10.0))
    {
      std::cerr << "WeightedAverage test: failed with ContributingCellOption " << option
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  // 3. The general path, with an image of 3 pixels, and its blanked version, whose pixel 2 is
  // blanked. Point 1 is shared by the pixels 0 and 1, and point 2 by the pixels 1 and 2.
  vtkNew<vtkUniformGrid> image;
  image->SetDimensions(4, 2, 1);
  vtkNew<vtkDoubleArray> imageScalars;
  imageScalars->SetName("Scalar");
  vtkNew<vtkDoubleArray> imageWeights;
  imageWeights->SetName("Weights");
  for (double value : { 10.0, 20.0, 30.0 })
  {
    imageScalars->InsertNextValue(value);
    imageWeights->InsertNextValue(value / 10.0);
  }
  image->GetCellData()->AddArray(imageScalars);
  image->GetCellData()->AddArray(imageWeights);
  // (10 * 1 + 20 * 2) / 3 at point 1, (20 * 2 + 30 * 3) / 5 at point 2
  if (!CheckValue("image", WeightedValue(image, "Scalar", 1), 50.0 / 3.0) ||
    !CheckValue("image", WeightedValue(image, "Scalar", 2), 26.0))
  {
    return EXIT_FAILURE;
  }
  image->BlankCell(2);
  if (!CheckValue("blanked image", WeightedValue(image, "Scalar", 1), 50.0 / 3.0) ||
    !CheckValue("blanked image", WeightedValue(image, "Scalar", 2), 20.0))
  {
    return EXIT_FAILURE;
  }

  // 4. A point whose cells have a total weight of 0 gets the null value.
  weights->SetValue(0, 0.0);
  weights->SetValue(1, 0.0);
  weights->Modified();
  for (int option : { vtkCellDataToPointData::All, vtkCellDataToPointData::Patch,
         vtkCellDataToPointData::DataSetMax })
  {
    if (!CheckValue("zero weights", WeightedValue(polyData, "Scalar", 0, option), 0.0))
    {
      return EXIT_FAILURE;
    }
  }

  // 5. A point array is not a weight array, even with as many tuples as there are cells: here 2
  // vertices (cells 0 and 1) and the 2 triangles (cells 2 and 3) on the 4 points. The average is
  // not weighted, with a warning: 15 at point 0 instead of (10 * 1 + 20 * 3) / 4 = 17.5.
  vtkNew<vtkPolyData> withVertices;
  withVertices->SetPoints(points);
  withVertices->SetPolys(polys);
  vtkNew<vtkCellArray> vertices;
  for (vtkIdType vertex : { 1, 3 })
  {
    vertices->InsertNextCell(1, &vertex);
  }
  withVertices->SetVerts(vertices);
  vtkNew<vtkDoubleArray> vertexScalars;
  vertexScalars->SetName("Scalar");
  for (double value : { 0.0, 0.0, 10.0, 20.0 })
  {
    vertexScalars->InsertNextValue(value);
  }
  withVertices->GetCellData()->AddArray(vertexScalars);
  vtkNew<vtkDoubleArray> pointWeights;
  pointWeights->SetName("Weights");
  for (double weight : { 1.0, 1.0, 1.0, 3.0 })
  {
    pointWeights->InsertNextValue(weight);
  }
  withVertices->GetPointData()->AddArray(pointWeights);

  vtkNew<vtkCellDataToPointData> pointWeightsC2P;
  vtkNew<vtkTest::ErrorObserver> observer;
  pointWeightsC2P->AddObserver(vtkCommand::WarningEvent, observer);
  pointWeightsC2P->SetInputData(withVertices);
  pointWeightsC2P->SetEnableWeightedAverage(true);
  pointWeightsC2P->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Weights");
  pointWeightsC2P->Update();
  vtkDataArray* pointScalars = pointWeightsC2P->GetOutput()->GetPointData()->GetArray("Scalar");
  if (!pointScalars || !CheckValue("point weights", pointScalars->GetTuple1(0), 15.0))
  {
    return EXIT_FAILURE;
  }
  if (!observer->GetWarning())
  {
    std::cerr << "WeightedAverage test: point weights: expected a warning" << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}

int TestCellDataToPointData(int, char*[])
{
  constexpr char name[] = "RTData";
  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-2, 2, -2, 2, -2, 2);
  wavelet->SetCenter(0, 0, 0);
  wavelet->SetMaximum(255);
  wavelet->SetStandardDeviation(.5);
  wavelet->SetXFreq(60);
  wavelet->SetYFreq(30);
  wavelet->SetZFreq(40);
  wavelet->SetXMag(10);
  wavelet->SetYMag(18);
  wavelet->SetZMag(5);
  wavelet->SetSubsampleRate(1);
  wavelet->Update();

  vtkNew<vtkDoubleArray> dist;
  dist->SetNumberOfComponents(1);
  dist->SetName("Dist");

  vtkImageData* original = wavelet->GetOutput();
  for (vtkIdType i = 0; i < original->GetNumberOfPoints(); ++i)
  {
    double p[3];
    original->GetPoint(i, p);
    dist->InsertNextValue(p[0] * p[0] + p[1] * p[1] + p[2] * p[2]);
  }
  original->GetPointData()->AddArray(dist);

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputData(original);
  p2c->PassPointDataOff();

  vtkNew<vtkCellDataToPointData> selectiveC2P;
  selectiveC2P->SetInputConnection(p2c->GetOutputPort());
  selectiveC2P->SetProcessAllArrays(false);
  selectiveC2P->AddCellDataArray(name);
  selectiveC2P->Update();

  vtkNew<vtkCellDataToPointData> sc2p;
  sc2p->SetInputConnection(p2c->GetOutputPort());
  sc2p->PassCellDataOff();
  sc2p->Update();

  vtkNew<vtkDataSetTriangleFilter> c2g;
  c2g->SetInputConnection(p2c->GetOutputPort());

  vtkNew<vtkCellDataToPointData> uc2p;
  uc2p->SetInputConnection(c2g->GetOutputPort());

  vtkDataArray* const x = sc2p->GetOutput()->GetPointData()->GetArray(name);

  // test if selective CellDataToPointData operates on the correct
  int outNumPArrays = selectiveC2P->GetOutput()->GetPointData()->GetNumberOfArrays(); // should be 1
  int outNumCArrays = selectiveC2P->GetOutput()->GetCellData()->GetNumberOfArrays();  // should be 0
  std::string pArrayName =
    selectiveC2P->GetOutput()->GetPointData()->GetArrayName(0); // should be RTData

  if (outNumPArrays != 1)
  {
    std::cerr << "Wrong number of PointData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (outNumCArrays != 0)
  {
    std::cerr << "Wrong number of CellData arrays." << std::endl;
    return EXIT_FAILURE;
  }

  if (pArrayName != name)
  {
    std::cerr << "Array name not matching original name." << std::endl;
    return EXIT_FAILURE;
  }

  // iterate through the options for which cells contribute to the result
  // for the cell data to point data filter. since all cells are 3D the
  // result should be the same.
  for (int opt = 0; opt < 3; opt++)
  {
    uc2p->SetContributingCellOption(opt);
    uc2p->Update();

    vtkDataArray* const y = uc2p->GetOutput()->GetPointData()->GetArray(name);

    vtkIdType const nvalues = x->GetNumberOfTuples() * x->GetNumberOfComponents();
    double mean = 0, variance = 0;

    // mean
    for (vtkIdType i = 0; i < nvalues; ++i)
    {
      mean += x->GetTuple1(i) - y->GetTuple1(i);
    }
    mean /= nvalues;

    // variance
    for (vtkIdType i = 0; i < nvalues; ++i)
    {
      double z = x->GetTuple1(i) - y->GetTuple1(i);
      variance += z * z;
    }
    variance /= nvalues;

    if (!(fabs(mean) < 1e-4 && fabs(variance) < 1e-4))
    {
      std::cerr << "Failure on option " << opt << std::endl;
      return EXIT_FAILURE;
    }
  }

  // set up a test to check that the cell data of the input is preserved
  // vtkCellDataToPointData removed vtkStringArrays from cell data of the
  // input data set which broke the pipeline concept
  vtkSmartPointer<vtkPolyData> data = vtkSmartPointer<vtkPolyData>::New();
  vtkSmartPointer<vtkPoints> pts = vtkSmartPointer<vtkPoints>::New();
  data->SetPoints(pts);
  data->Allocate();

  vtkSmartPointer<vtkIdList> lst = vtkSmartPointer<vtkIdList>::New();
  lst->InsertNextId(pts->InsertNextPoint(0, 0, 0));
  data->InsertNextCell(VTK_VERTEX, lst);

  vtkSmartPointer<vtkStringArray> array = vtkSmartPointer<vtkStringArray>::New();
  array->SetName("test-strings");
  array->InsertNextValue("A");
  data->GetCellData()->AddArray(array);

  vtkSmartPointer<vtkCellDataToPointData> c2p = vtkSmartPointer<vtkCellDataToPointData>::New();
  c2p->SetInputData(data);
  c2p->SetProcessAllArrays(true);
  c2p->Update();

  vtkStringArray* test =
    vtkStringArray::SafeDownCast(data->GetCellData()->GetAbstractArray("test-strings"));
  if (!test)
  {
    std::cerr << "vtkCellDataToPointData has removed string array from its input dataset."
              << std::endl;
    return EXIT_FAILURE;
  }

  // Test PieceInvariant setting
  if (TestCellDataToPointDataPieceInvariant() != EXIT_SUCCESS)
  {
    std::cerr << "Piece invariant test failed." << std::endl;
    return EXIT_FAILURE;
  }

  // Test EnableWeightedAverage setting and weighted average computation
  if (TestWeightedAverage() != EXIT_SUCCESS)
  {
    std::cerr << "Weighted average test failed." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
