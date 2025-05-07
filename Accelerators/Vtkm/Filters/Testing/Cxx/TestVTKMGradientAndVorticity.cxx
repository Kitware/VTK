// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayCalculator.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"

#include "vtkmFilterOverrides.h"
#include "vtkmGradient.h"

#include <vector>
#include <viskores/testing/Testing.h>

#define VTK_CREATE(type, var) vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define VTK_MAXIMUM_NUMBER_OF_POINTS 216

namespace
{
double Tolerance = 0.00001;

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
template <typename GradientArrayType, typename CorrectArrayType>
int IsGradientCorrect(GradientArrayType* gradientsArray, CorrectArrayType* correctArray)
{
  int numberOfComponents = gradientsArray->GetNumberOfComponents();
  if (numberOfComponents != correctArray->GetNumberOfComponents())
  {
    std::cout << "Gradients array has unexpected number of components.\n";
    return 0;
  }
  vtkIdType numberOfTuples = gradientsArray->GetNumberOfTuples();
  if (numberOfTuples != correctArray->GetNumberOfTuples())
  {
    std::cout << "Gradients array has unexpected number of tuples.\n";
    return 0;
  }

  vtkDataArrayAccessor<GradientArrayType> gradients(gradientsArray);
  vtkDataArrayAccessor<CorrectArrayType> correct(correctArray);

  for (vtkIdType i = 0; i < numberOfTuples; i++)
  {
    bool invalid = false;
    for (int j = 0; j < numberOfComponents; j++)
    {
      double value = gradients.Get(i, j);
      double expected = correct.Get(i, j);

      if (std::abs(value - expected) > Tolerance)
      {
        invalid = true;
      }
    }

    if (invalid)
    {
      std::cout << "Gradient[ " << i << " ] should look like: " << std::endl;
      for (int j = 0; j < numberOfComponents; ++j)
      {
        std::cout << correct.Get(i, j);
        if ((j % 3) == 2)
        {
          std::cout << "\n";
        }
      }

      std::cout << "Gradient[ " << i << " ] actually looks like: " << std::endl;
      for (int j = 0; j < numberOfComponents; ++j)
      {
        std::cout << gradients.Get(i, j);
        if ((j % 3) == 2)
        {
          std::cout << "\n";
        }
      }
      std::cout << std::endl;

      return 0;
    }
  }
  return 1;
}

//------------------------------------------------------------------------------
// we assume that the gradients are correct and so we can compute the "real"
// vorticity from it
int IsVorticityCorrect(vtkDataArray* gradients, vtkDataArray* vorticity)
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
    if (!test_equal(v[0], g[7] - g[5]))
    {
      vtkGenericWarningMacro("Bad vorticity[0] value "
        << v[0] << " " << g[7] - g[5] << " difference is " << (v[0] - g[7] + g[5]));
      return 0;
    }
    else if (!test_equal(v[1], g[2] - g[6]))
    {
      vtkGenericWarningMacro("Bad vorticity[1] value "
        << v[1] << " " << g[2] - g[6] << " difference is " << (v[1] - g[2] + g[6]));
      return 0;
    }
    else if (!test_equal(v[2], g[3] - g[1]))
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
template <typename QCriterionType>
int IsQCriterionCorrect(vtkDataArray* gradients, QCriterionType* qCriterionArray)
{
  if (gradients->GetNumberOfComponents() != 9 || qCriterionArray->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  vtkDataArrayAccessor<QCriterionType> qCriterion(qCriterionArray);
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double qc = qCriterion.Get(i, 0);

    double t1 = .25 *
      ((g[7] - g[5]) * (g[7] - g[5]) + (g[3] - g[1]) * (g[3] - g[1]) +
        (g[2] - g[6]) * (g[2] - g[6]));
    double t2 = .5 *
      (g[0] * g[0] + g[4] * g[4] + g[8] * g[8] +
        .5 *
          ((g[3] + g[1]) * (g[3] + g[1]) + (g[6] + g[2]) * (g[6] + g[2]) +
            (g[7] + g[5]) * (g[7] + g[5])));

    if (!test_equal(qc, t1 - t2))
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
template <typename DivergenceType>
int IsDivergenceCorrect(vtkDataArray* gradients, DivergenceType* divergenceArray)
{
  if (gradients->GetNumberOfComponents() != 9 || divergenceArray->GetNumberOfComponents() != 1)
  {
    vtkGenericWarningMacro("Bad number of components.");
    return 0;
  }
  vtkDataArrayAccessor<DivergenceType> divergence(divergenceArray);
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    double* g = gradients->GetTuple(i);
    double div = divergence.Get(i, 0);
    double gValue = g[0] + g[4] + g[8];

    if (!test_equal(div, gValue))
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

  const char resultName[] = "Result";

  // cell stuff ---------------------------------------------------------------

  // don't test cell gradients on structured and rectilinear grids as it is currently
  // unsupported
  if (!grid->IsA("vtkStructuredGrid") && !grid->IsA("vtkRectilinearGrid"))
  {
    VTK_CREATE(vtkmGradient, cellGradients);
    cellGradients->ForceVTKmOn();
    cellGradients->SetInputData(grid);
    cellGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    cellGradients->SetResultArrayName(resultName);

    vtkmFilterOverrides::EnabledOff(); // Turn off override to instantiate VTK filter
    VTK_CREATE(vtkGradientFilter, correctCellGradients);
    vtkmFilterOverrides::EnabledOn();
    correctCellGradients->SetInputData(grid);
    correctCellGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    correctCellGradients->SetResultArrayName(resultName);

    cellGradients->Update();
    correctCellGradients->Update();

    vtkDataArray* gradCellArray = cellGradients->GetOutput()->GetCellData()->GetArray(resultName);

    vtkDoubleArray* correctCellArray = vtkArrayDownCast<vtkDoubleArray>(
      correctCellGradients->GetOutput()->GetCellData()->GetArray(resultName));

    if (!IsGradientCorrect(gradCellArray, correctCellArray))
    {
      return EXIT_FAILURE;
    }

    // now check on the vorticity calculations
    VTK_CREATE(vtkmGradient, cellVorticity);
    cellVorticity->ForceVTKmOn();
    cellVorticity->SetInputData(grid);
    cellVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
    cellVorticity->SetResultArrayName(resultName);
    cellVorticity->SetComputeVorticity(1);
    cellVorticity->Update();

    vtkDataArray* vorticityCellArray =
      cellVorticity->GetOutput()->GetCellData()->GetArray("Vorticity");
    if (!IsVorticityCorrect(gradCellArray, vorticityCellArray))
    {
      return EXIT_FAILURE;
    }
  }

  // point stuff --------------------------------------------------------------
  VTK_CREATE(vtkmGradient, pointGradients);
  pointGradients->ForceVTKmOn();
  pointGradients->SetInputData(grid);
  pointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  vtkmFilterOverrides::EnabledOff(); // Turn off override to instantiate VTK filter
  VTK_CREATE(vtkGradientFilter, correctPointGradients);
  vtkmFilterOverrides::EnabledOn();
  correctPointGradients->SetInputData(grid);
  correctPointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  correctPointGradients->SetResultArrayName(resultName);

  pointGradients->Update();
  correctPointGradients->Update();

  vtkDataArray* gradPointArray = pointGradients->GetOutput()->GetPointData()->GetArray(resultName);

  vtkDoubleArray* correctPointArray = vtkArrayDownCast<vtkDoubleArray>(
    correctPointGradients->GetOutput()->GetPointData()->GetArray(resultName));

  if (!IsGradientCorrect(gradPointArray, correctPointArray))
  {
    return EXIT_FAILURE;
  }

  // now check on the vorticity calculations
  VTK_CREATE(vtkmGradient, pointVorticity);
  pointVorticity->ForceVTKmOn();
  pointVorticity->SetInputData(grid);
  pointVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointVorticity->SetResultArrayName(resultName);
  pointVorticity->SetComputeVorticity(1);
  pointVorticity->SetComputeQCriterion(1);
  pointVorticity->SetComputeDivergence(1);
  pointVorticity->Update();

  vtkDataArray* vorticityPointArray =
    pointVorticity->GetOutput()->GetPointData()->GetArray("Vorticity");
  if (!IsVorticityCorrect(gradPointArray, vorticityPointArray))
  {
    return EXIT_FAILURE;
  }

  vtkDataArray* divergencePointArray =
    pointVorticity->GetOutput()->GetPointData()->GetArray("Divergence");
  if (!IsDivergenceCorrect(gradPointArray, divergencePointArray))
  {
    return EXIT_FAILURE;
  }

  vtkDataArray* qCriterionPointArray =
    pointVorticity->GetOutput()->GetPointData()->GetArray("Q-criterion");
  if (!IsQCriterionCorrect(gradPointArray, qCriterionPointArray))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
} // end local namespace

//------------------------------------------------------------------------------
int TestVTKMGradientAndVorticity(int argc, char* argv[])
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

  std::string filename(std::string(data_root) + "/Data/SampleStructGrid.vtk");
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

  return PerformTest(ug);
}
