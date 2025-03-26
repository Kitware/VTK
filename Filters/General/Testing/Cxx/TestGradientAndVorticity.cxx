// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCell.h"
#include "vtkCellCenters.h"
#include "vtkCellData.h"
#include "vtkCellType.h"
#include "vtkCellTypeSource.h"
#include "vtkDoubleArray.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkGeneralTransform.h"
#include "vtkGradientFilter.h"
#include "vtkHigherOrderHexahedron.h"
#include "vtkHigherOrderQuadrilateral.h"
#include "vtkHigherOrderWedge.h"
#include "vtkIOSSReader.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkResampleToImage.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredData.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkThreshold.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include <limits>
#include <vector>

#define VTK_CREATE(type, var) vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define VTK_MAXIMUM_NUMBER_OF_POINTS 216

namespace
{
double Tolerance = 0.00001;

bool ArePointsWithinTolerance(double v1, double v2)
{
  if (v1 == v2 || fabs(v1) + fabs(v2) < Tolerance)
  {
    return true;
  }

  if (v1 == 0.0)
  {
    if (fabs(v2) < Tolerance)
    {
      return true;
    }
    std::cout << fabs(v2) << " (fabs(v2)) should be less than " << Tolerance << std::endl;
    return false;
  }
  if (fabs(1. - v1 / v2) < Tolerance)
  {
    return true;
  }
  std::cout << fabs(1. - v1 / v2) << " (fabs(1 - v1/v2)) should be less than " << Tolerance
            << std::endl;
  return false;
}

//------------------------------------------------------------------------------
void CreateCellData(vtkDataSet* grid, int numberOfComponents, int offset, const char* arrayName)
{
  vtkIdType numberOfCells = grid->GetNumberOfCells();
  VTK_CREATE(vtkDoubleArray, array);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfCells);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3], parametricCenter[3], weights[VTK_MAXIMUM_NUMBER_OF_POINTS];
  for (vtkIdType i = 0; i < numberOfCells; i++)
  {
    vtkCell* cell = grid->GetCell(i);
    cell->GetParametricCenter(parametricCenter);
    int subId = 0;
    cell->EvaluateLocation(subId, parametricCenter, point, weights);
    for (int j = 0; j < numberOfComponents; j++)
    { // +offset makes the curl/vorticity nonzero
      tupleValues[j] = point[(j + offset) % 3];
    }
    array->SetTypedTuple(i, tupleValues.data());
  }
  array->SetName(arrayName);
  grid->GetCellData()->AddArray(array);
}

//------------------------------------------------------------------------------
void CreatePointData(vtkDataSet* grid, int numberOfComponents, int offset, const char* arrayName)
{
  vtkIdType numberOfPoints = grid->GetNumberOfPoints();
  VTK_CREATE(vtkDoubleArray, array);
  array->SetNumberOfComponents(numberOfComponents);
  array->SetNumberOfTuples(numberOfPoints);
  std::vector<double> tupleValues(numberOfComponents);
  double point[3];
  for (vtkIdType i = 0; i < numberOfPoints; i++)
  {
    grid->GetPoint(i, point);
    for (int j = 0; j < numberOfComponents; j++)
    { // +offset makes the curl/vorticity nonzero
      tupleValues[j] = point[(j + offset) % 3];
    }
    array->SetTypedTuple(i, tupleValues.data());
  }
  array->SetName(arrayName);
  grid->GetPointData()->AddArray(array);
}

//------------------------------------------------------------------------------
int IsGradientCorrect(vtkDoubleArray* gradients, int offset)
{
  int numberOfComponents = gradients->GetNumberOfComponents();
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* values = gradients->GetTuple(i);
    for (int origComp = 0; origComp < numberOfComponents / 3; origComp++)
    {
      for (int gradDir = 0; gradDir < 3; gradDir++)
      {
        if ((origComp - gradDir + offset) % 3 == 0)
        {
          if (fabs(values[origComp * 3 + gradDir] - 1.) > Tolerance)
          {
            vtkGenericWarningMacro(
              "Gradient value should be one but is " << values[origComp * 3 + gradDir]);
            return 0;
          }
        }
        else if (fabs(values[origComp * 3 + gradDir]) > Tolerance)
        {
          vtkGenericWarningMacro(
            "Gradient value should be zero but is " << values[origComp * 3 + gradDir]);
          return 0;
        }
      }
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// vorticity from it
int IsVorticityCorrect(vtkDoubleArray* gradients, vtkDoubleArray* vorticity)
{
  if (gradients->GetNumberOfComponents() != 9 || vorticity->GetNumberOfComponents() != 3)
  {
    vtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double* v = vorticity->GetTuple(i);
    if (!ArePointsWithinTolerance(v[0], g[7] - g[5]))
    {
      vtkGenericWarningMacro("Bad vorticity[0] value "
        << v[0] << " " << g[7] - g[5] << " difference is " << (v[0] - g[7] + g[5]));
      return 0;
    }
    else if (!ArePointsWithinTolerance(v[1], g[2] - g[6]))
    {
      vtkGenericWarningMacro("Bad vorticity[1] value "
        << v[1] << " " << g[2] - g[6] << " difference is " << (v[1] - g[2] + g[6]));
      return 0;
    }
    else if (!ArePointsWithinTolerance(v[2], g[3] - g[1]))
    {
      vtkGenericWarningMacro("Bad vorticity[2] value "
        << v[2] << " " << g[3] - g[1] << " difference is " << (v[2] - g[3] + g[1]));
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// Q criterion from it
int IsQCriterionCorrect(vtkDoubleArray* gradients, vtkDoubleArray* qCriterion)
{
  if (gradients->GetNumberOfComponents() != 9 || qCriterion->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double qc = qCriterion->GetValue(i);

    double t1 = .25 *
      ((g[7] - g[5]) * (g[7] - g[5]) + (g[3] - g[1]) * (g[3] - g[1]) +
        (g[2] - g[6]) * (g[2] - g[6]));
    double t2 = .5 *
      (g[0] * g[0] + g[4] * g[4] + g[8] * g[8] +
        .5 *
          ((g[3] + g[1]) * (g[3] + g[1]) + (g[6] + g[2]) * (g[6] + g[2]) +
            (g[7] + g[5]) * (g[7] + g[5])));

    if (!ArePointsWithinTolerance(qc, t1 - t2))
    {
      vtkGenericWarningMacro(
        "Bad Q-criterion value " << qc << " " << t1 - t2 << " difference is " << (qc - t1 + t2));
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// divergence from it
int IsDivergenceCorrect(vtkDoubleArray* gradients, vtkDoubleArray* divergence)
{
  if (gradients->GetNumberOfComponents() != 9 || divergence->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double div = divergence->GetValue(i);
    double gValue = g[0] + g[4] + g[8];

    if (!ArePointsWithinTolerance(div, gValue))
    {
      vtkGenericWarningMacro(
        "Bad divergence value " << div << " " << gValue << " difference is " << (div - gValue));
      return 0;
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
int PerformTest(vtkDataSet* grid)
{
  // Cleaning out the existing field data so that I can replace it with
  // an analytic function that I know the gradient of
  grid->GetPointData()->Initialize();
  grid->GetCellData()->Initialize();
  const char fieldName[] = "LinearField";
  int offset = 1;
  const int numberOfComponents = 3;
  CreateCellData(grid, numberOfComponents, offset, fieldName);
  CreatePointData(grid, numberOfComponents, offset, fieldName);

  VTK_CREATE(vtkGradientFilter, cellGradients);
  cellGradients->SetInputData(grid);
  cellGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
  const char resultName[] = "Result";
  cellGradients->SetResultArrayName(resultName);

  VTK_CREATE(vtkGradientFilter, pointGradients);
  pointGradients->SetInputData(grid);
  pointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  // if we have an unstructured grid we also want to test out the options
  // for which cells contribute to the gradient computation so we loop
  // over them here.
  int gradientOptions = grid->IsA("vtkUnstructuredGrid") ? 2 : 0;
  for (int option = 0; option <= gradientOptions; option++)
  {
    cellGradients->SetContributingCellOption(option);
    pointGradients->SetContributingCellOption(option);
    cellGradients->Update();
    pointGradients->Update();

    vtkDoubleArray* gradCellArray = vtkArrayDownCast<vtkDoubleArray>(
      vtkDataSet::SafeDownCast(cellGradients->GetOutput())->GetCellData()->GetArray(resultName));

    if (!grid->IsA("vtkUnstructuredGrid"))
    {
      // ignore cell gradients if this is an unstructured grid
      // because the accuracy is so lousy
      if (!IsGradientCorrect(gradCellArray, offset))
      {
        return EXIT_FAILURE;
      }
    }

    vtkDoubleArray* gradPointArray = vtkArrayDownCast<vtkDoubleArray>(
      vtkDataSet::SafeDownCast(pointGradients->GetOutput())->GetPointData()->GetArray(resultName));

    if (!IsGradientCorrect(gradPointArray, offset))
    {
      return EXIT_FAILURE;
    }

    // now check on the vorticity calculations
    VTK_CREATE(vtkGradientFilter, cellVorticity);
    cellVorticity->SetInputData(grid);
    cellVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    cellVorticity->SetResultArrayName(resultName);
    cellVorticity->SetComputeVorticity(1);
    cellVorticity->SetContributingCellOption(option);
    cellVorticity->Update();

    VTK_CREATE(vtkGradientFilter, pointVorticity);
    pointVorticity->SetInputData(grid);
    pointVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
    pointVorticity->SetResultArrayName(resultName);
    pointVorticity->SetComputeVorticity(1);
    pointVorticity->SetComputeQCriterion(1);
    pointVorticity->SetComputeDivergence(1);
    pointVorticity->SetContributingCellOption(option);
    pointVorticity->Update();

    // cell stuff
    vtkDoubleArray* vorticityCellArray = vtkArrayDownCast<vtkDoubleArray>(
      vtkDataSet::SafeDownCast(cellVorticity->GetOutput())->GetCellData()->GetArray("Vorticity"));

    if (!IsVorticityCorrect(gradCellArray, vorticityCellArray))
    {
      return EXIT_FAILURE;
    }

    // point stuff
    vtkDoubleArray* vorticityPointArray = vtkArrayDownCast<vtkDoubleArray>(
      vtkDataSet::SafeDownCast(pointVorticity->GetOutput())->GetPointData()->GetArray("Vorticity"));

    if (!IsVorticityCorrect(gradPointArray, vorticityPointArray))
    {
      return EXIT_FAILURE;
    }
    vtkDoubleArray* divergencePointArray =
      vtkArrayDownCast<vtkDoubleArray>(vtkDataSet::SafeDownCast(pointVorticity->GetOutput())
                                         ->GetPointData()
                                         ->GetArray("Divergence"));

    if (!IsDivergenceCorrect(gradPointArray, divergencePointArray))
    {
      return EXIT_FAILURE;
    }
    vtkDoubleArray* qCriterionPointArray =
      vtkArrayDownCast<vtkDoubleArray>(vtkDataSet::SafeDownCast(pointVorticity->GetOutput())
                                         ->GetPointData()
                                         ->GetArray("Q-criterion"));
    if (!IsQCriterionCorrect(gradPointArray, qCriterionPointArray))
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestGradient(int* cellTypes, vtkGeneralTransform* transform)
{
  vtkNew<vtkCellTypeSource> cellTypeSource;
  cellTypeSource->SetBlocksDimensions(3, 3, 3); // make sure we have an interior cell
  cellTypeSource->SetCellOrder(3);

  vtkNew<vtkTransformFilter> transformFilter;
  transformFilter->SetInputConnection(cellTypeSource->GetOutputPort());
  transformFilter->SetTransform(transform);

  vtkNew<vtkElevationFilter> elevationFilter;
  elevationFilter->SetLowPoint(0, 0, 0);
  elevationFilter->SetHighPoint(1, 0, 0);
  elevationFilter->SetScalarRange(0, 1);
  elevationFilter->SetInputConnection(transformFilter->GetOutputPort());

  vtkNew<vtkGradientFilter> gradientFilter;
  gradientFilter->SetInputConnection(elevationFilter->GetOutputPort());
  gradientFilter->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, "Elevation");

  transformFilter->Update();
  vtkDataSet* output = transformFilter->GetOutput();
  double bounds[6];
  output->GetBounds(bounds);
  elevationFilter->SetLowPoint(bounds[0], 0, 0);
  elevationFilter->SetHighPoint(bounds[1], 0, 0);
  elevationFilter->SetScalarRange(bounds[0], bounds[1]);

  for (auto i = 0; cellTypes[i] != -1; i++)
  {
    cellTypeSource->SetCellType(cellTypes[i]);
    gradientFilter->Update();
    // Arrays generated by vtkm filters are of type `vtkAOSDataArrayTemplate`
    auto* result = vtkFloatArray::Superclass::SafeDownCast(
      gradientFilter->GetOutput()->GetPointData()->GetArray("Gradients"));
    double range[2];
    result->GetRange(range, 0);
    if (range[0] < .99 || range[1] > 1.01)
    {
      vtkGenericWarningMacro("Incorrect gradient for cell type " << cellTypes[i]);
      return EXIT_FAILURE;
    }
    for (auto j = 1; j < 3; j++)
    {
      result->GetRange(range, j);
      if (range[0] < -.01 || range[1] > .01)
      {
        vtkGenericWarningMacro("Incorrect gradient for cell type " << cellTypes[i]);
        return EXIT_FAILURE;
      }
    }
  }

  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestPoints(vtkImageData* grid, vtkUnstructuredGrid* ref)
{
  auto gridArray = vtkArrayDownCast<vtkDoubleArray>(grid->GetPointData()->GetAbstractArray("Pres"));
  auto refArray = vtkArrayDownCast<vtkDoubleArray>(ref->GetPointData()->GetAbstractArray("Pres"));
  double refPoint[3];
  double bounds[6];
  int extent[6];
  grid->GetBounds(bounds);
  grid->GetExtent(extent);
  int width[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, width);
  int ijk[3];
  for (vtkIdType pointId = 0; pointId < ref->GetNumberOfPoints(); ++pointId)
  {
    ref->GetPoint(pointId, refPoint);
    ijk[0] = (refPoint[0] - bounds[0]) / (bounds[1] - bounds[0]) * width[0];
    ijk[1] = (refPoint[1] - bounds[2]) / (bounds[3] - bounds[2]) * width[1];
    ijk[2] = (refPoint[2] - bounds[4]) / (bounds[5] - bounds[4]) * width[2];
    vtkIdType gridPointId = vtkStructuredData::ComputePointId(width, ijk);

    if (std::abs(gridArray->GetValue(gridPointId) - refArray->GetValue(pointId)) > 1e-6)
    {
      vtkGenericWarningMacro("Computing gradient on a grid with hidden points failed");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}

//------------------------------------------------------------------------------
int TestCells(vtkImageData* grid, vtkPointSet* ref)
{
  auto gridArray = vtkArrayDownCast<vtkDoubleArray>(grid->GetCellData()->GetAbstractArray("Pres"));
  auto refArray = vtkArrayDownCast<vtkDoubleArray>(ref->GetPointData()->GetAbstractArray("Pres"));
  double refPoint[3];
  double bounds[6];
  int extent[6];
  grid->GetBounds(bounds);
  grid->GetExtent(extent);
  int width[3];
  vtkStructuredData::GetDimensionsFromExtent(extent, width);
  int ijk[3];
  for (vtkIdType pointId = 0; pointId < ref->GetNumberOfPoints(); ++pointId)
  {
    ref->GetPoint(pointId, refPoint);
    ijk[0] = (refPoint[0] - bounds[0]) / (bounds[1] - bounds[0]) * (width[0] - 1);
    ijk[1] = (refPoint[1] - bounds[2]) / (bounds[3] - bounds[2]) * (width[1] - 1);
    ijk[2] = (refPoint[2] - bounds[4]) / (bounds[5] - bounds[4]) * (width[2] - 1);
    vtkIdType gridPointId = vtkStructuredData::ComputeCellId(width, ijk);

    if (std::abs(gridArray->GetValue(gridPointId) - refArray->GetValue(pointId)) > 1e-6)
    {
      vtkGenericWarningMacro("Computing gradient on a grid with hidden cells failed");
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
} // end local namespace

//------------------------------------------------------------------------------
int TestGradientAndVorticity(int argc, char* argv[])
{
  int i;
  // Need to get the data root.
  const char* data_root = nullptr;
  for (i = 0; i < argc - 1; i++)
  {
    if (strcmp("-D", argv[i]) == 0)
    {
      data_root = argv[i + 1];
      break;
    }
  }
  if (!data_root)
  {
    vtkGenericWarningMacro("Need to specify the directory to VTK_DATA_ROOT with -D <dir>.");
    return EXIT_FAILURE;
  }

  std::string filename;
  filename = data_root;
  filename += "/Data/SampleStructGrid.vtk";
  VTK_CREATE(vtkStructuredGridReader, structuredGridReader);
  structuredGridReader->SetFileName(filename.c_str());
  structuredGridReader->Update();
  vtkDataSet* grid = vtkDataSet::SafeDownCast(structuredGridReader->GetOutput());

  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // convert the structured grid to an unstructured grid
  VTK_CREATE(vtkUnstructuredGrid, ug);
  ug->SetPoints(vtkStructuredGrid::SafeDownCast(grid)->GetPoints());
  ug->Allocate(grid->GetNumberOfCells());
  for (vtkIdType id = 0; id < grid->GetNumberOfCells(); id++)
  {
    vtkCell* cell = grid->GetCell(id);
    ug->InsertNextCell(cell->GetCellType(), cell->GetPointIds());
  }

  if (PerformTest(ug))
  {
    return EXIT_FAILURE;
  }

  // Now test the gradient of a variety of cell types using the cell type
  // source. We scale and rotate the grid to make sure that we don't have the
  // cells conveniently set up to their parametric coordinate system and then
  // compare to an analytic function (f=x) such that the gradient is
  // (1, 0, 0).
  vtkNew<vtkGeneralTransform> transform;
  transform->Scale(2, 3, 4);
  int oneDCells[] = {
    VTK_LINE,
    // VTK_QUADRATIC_EDGE, //Derivatives() not implemented
    VTK_CUBIC_LINE,
    // VTK_LAGRANGE_CURVE, //Derivatives() not implemented
    -1 // mark as end
  };
  if (TestGradient(oneDCells, transform))
  {
    return EXIT_FAILURE;
  }

  transform->RotateZ(30);
  int twoDCells[] = {
    VTK_TRIANGLE, VTK_QUAD, VTK_QUADRATIC_TRIANGLE, VTK_QUADRATIC_QUAD, VTK_LAGRANGE_TRIANGLE,
    VTK_LAGRANGE_QUADRILATERAL,
    -1 // mark as end
  };
  if (TestGradient(twoDCells, transform))
  {
    return EXIT_FAILURE;
  }

  transform->RotateX(20);
  transform->RotateY(40);
  int threeDCells[] = {
    VTK_TETRA, VTK_HEXAHEDRON, VTK_WEDGE, VTK_PYRAMID, VTK_QUADRATIC_TETRA,
    VTK_QUADRATIC_HEXAHEDRON, VTK_QUADRATIC_WEDGE,
    // VTK_QUADRATIC_PYRAMID,
    VTK_LAGRANGE_TETRAHEDRON, VTK_LAGRANGE_HEXAHEDRON, VTK_LAGRANGE_WEDGE,
    -1 // mark as end
  };
  if (TestGradient(threeDCells, transform))
  {
    return EXIT_FAILURE;
  }

  // Testing handling of hidden cells and points
  std::string disk_out_ref = data_root;
  disk_out_ref += "/Data/disk_out_ref.ex2";
  vtkNew<vtkIOSSReader> iossReader;
  iossReader->SetFileName(disk_out_ref.c_str());
  iossReader->Update();
  vtkUnstructuredGrid* disk = vtkUnstructuredGrid::SafeDownCast(vtkPartitionedDataSet::SafeDownCast(
    vtkPartitionedDataSetCollection::SafeDownCast(iossReader->GetOutputDataObject(0))
      ->GetPartitionedDataSet(0))
                                                                  ->GetPartition(0));

  vtkNew<vtkResampleToImage> resampler;
  resampler->SetInputDataObject(disk);
  resampler->SetSamplingDimensions(50, 50, 50);
  resampler->SetUseInputBounds(true);

  vtkNew<vtkGradientFilter> pointGradient;
  pointGradient->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Pres");
  pointGradient->SetInputConnection(resampler->GetOutputPort());

  vtkNew<vtkThreshold> ugPointConverter;
  ugPointConverter->SetInputConnection(resampler->GetOutputPort());
  ugPointConverter->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Pres");
  ugPointConverter->SetLowerThreshold(-std::numeric_limits<double>::infinity());
  ugPointConverter->SetUpperThreshold(std::numeric_limits<double>::infinity());

  vtkNew<vtkGradientFilter> pointRefGradient;
  pointRefGradient->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, "Pres");
  pointRefGradient->SetInputConnection(ugPointConverter->GetOutputPort());

  pointRefGradient->Update();
  pointGradient->Update();

  if (TestPoints(vtkImageData::SafeDownCast(pointGradient->GetOutput()),
        vtkUnstructuredGrid::SafeDownCast(pointRefGradient->GetOutput())))
  {
    return EXIT_FAILURE;
  }

  vtkNew<vtkPointDataToCellData> point2cell;
  point2cell->SetInputConnection(resampler->GetOutputPort());

  vtkNew<vtkGradientFilter> cellGradient;
  cellGradient->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Pres");
  cellGradient->SetInputConnection(point2cell->GetOutputPort());

  vtkNew<vtkThreshold> ugCellConverter;
  ugCellConverter->SetInputConnection(cellGradient->GetOutputPort());
  ugCellConverter->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Pres");
  ugCellConverter->SetLowerThreshold(-std::numeric_limits<double>::infinity());
  ugCellConverter->SetUpperThreshold(std::numeric_limits<double>::infinity());

  vtkNew<vtkGradientFilter> cellRefGradient;
  cellRefGradient->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Pres");
  cellRefGradient->SetInputConnection(ugCellConverter->GetOutputPort());

  vtkNew<vtkCellCenters> cellCenterRefGradient;
  cellCenterRefGradient->CopyArraysOn();
  cellCenterRefGradient->SetInputConnection(cellRefGradient->GetOutputPort());

  cellCenterRefGradient->Update();
  cellGradient->Update();

  if (TestCells(vtkImageData::SafeDownCast(cellGradient->GetOutput()),
        vtkPointSet::SafeDownCast(cellCenterRefGradient->GetOutput())))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
