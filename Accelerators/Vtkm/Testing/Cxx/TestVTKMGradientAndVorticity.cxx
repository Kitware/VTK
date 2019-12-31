/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGradientAndVorticity.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkArrayCalculator.h"
#include "vtkCell.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkStdString.h"
#include "vtkStructuredGrid.h"
#include "vtkStructuredGridReader.h"
#include "vtkUnstructuredGrid.h"
#include "vtkUnstructuredGridReader.h"
#include "vtkmGradient.h"

#include <vector>
#include <vtkm/testing/Testing.h>

#define VTK_CREATE(type, var) vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

// The 3D cell with the maximum number of points is VTK_LAGRANGE_HEXAHEDRON.
// We support up to 6th order hexahedra.
#define VTK_MAXIMUM_NUMBER_OF_POINTS 216

namespace
{
double Tolerance = 0.00001;

//-----------------------------------------------------------------------------
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
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  array->SetName(arrayName);
  grid->GetCellData()->AddArray(array);
}

//-----------------------------------------------------------------------------
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
    array->SetTypedTuple(i, &tupleValues[0]);
  }
  array->SetName(arrayName);
  grid->GetPointData()->AddArray(array);
}

//-----------------------------------------------------------------------------
int IsGradientCorrect(vtkDoubleArray* gradients, vtkDoubleArray* correct)
{
  int numberOfComponents = gradients->GetNumberOfComponents();
  for (vtkIdType i = 0; i < gradients->GetNumberOfTuples(); i++)
  {
    bool invalid = false;
    for (int j = 0; j < numberOfComponents; j++)
    {
      double value = gradients->GetTypedComponent(i, j);
      double expected = correct->GetTypedComponent(i, j);

      if ((value - expected) > Tolerance)
      {
        invalid = true;
      }
    }

    if (invalid)
    {
      std::vector<double> values;
      values.resize(numberOfComponents);
      std::vector<double> expected;
      expected.resize(numberOfComponents);

      gradients->GetTypedTuple(i, values.data());
      correct->GetTypedTuple(i, expected.data());

      std::cout << "Gradient[ i ] should look like: " << std::endl;
      std::cout << expected[0] << ", " << expected[1] << ", " << expected[2] << std::endl;
      if (numberOfComponents > 3)
      {
        std::cout << expected[3] << ", " << expected[4] << ", " << expected[5] << std::endl;
        std::cout << expected[6] << ", " << expected[7] << ", " << expected[8] << std::endl;
      }

      std::cout << "Gradient[ i ] actually looks like: " << std::endl;
      std::cout << values[0] << ", " << values[1] << ", " << values[2] << std::endl;
      if (numberOfComponents > 3)
      {
        std::cout << values[3] << ", " << values[4] << ", " << values[5] << std::endl;
        std::cout << values[6] << ", " << values[7] << ", " << values[8] << std::endl;
      }
      std::cout << std::endl;
    }

    if (i > 10 && invalid)
    {
      return 0;
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
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

//-----------------------------------------------------------------------------
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

    if (!test_equal(qc, t1 - t2))
    {
      vtkGenericWarningMacro(
        "Bad Q-criterion value " << qc << " " << t1 - t2 << " difference is " << (qc - t1 + t2));
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
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

    if (!test_equal(div, gValue))
    {
      vtkGenericWarningMacro(
        "Bad divergence value " << div << " " << gValue << " difference is " << (div - gValue));
      return 0;
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
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

  VTK_CREATE(vtkmGradient, cellGradients);
  cellGradients->SetInputData(grid);
  cellGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
  cellGradients->SetResultArrayName(resultName);

  VTK_CREATE(vtkGradientFilter, correctCellGradients);
  correctCellGradients->SetInputData(grid);
  correctCellGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
  correctCellGradients->SetResultArrayName(resultName);

  VTK_CREATE(vtkmGradient, pointGradients);
  pointGradients->SetInputData(grid);
  pointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  VTK_CREATE(vtkGradientFilter, correctPointGradients);
  correctPointGradients->SetInputData(grid);
  correctPointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  correctPointGradients->SetResultArrayName(resultName);

  cellGradients->Update();
  pointGradients->Update();

  correctCellGradients->Update();
  correctPointGradients->Update();

  vtkDoubleArray* gradCellArray = vtkArrayDownCast<vtkDoubleArray>(
    vtkDataSet::SafeDownCast(cellGradients->GetOutput())->GetCellData()->GetArray(resultName));

  vtkDoubleArray* correctCellArray =
    vtkArrayDownCast<vtkDoubleArray>(vtkDataSet::SafeDownCast(correctCellGradients->GetOutput())
                                       ->GetCellData()
                                       ->GetArray(resultName));

  if (!grid->IsA("vtkStructuredGrid"))
  {
    // ignore cell gradients on structured grids as the version for
    // VTK-m differs from VTK. Once VTK-m is able to do stencil based
    // gradients for point and cells, we can enable this check.
    if (!IsGradientCorrect(gradCellArray, correctCellArray))
    {
      return EXIT_FAILURE;
    }
  }

  vtkDoubleArray* gradPointArray = vtkArrayDownCast<vtkDoubleArray>(
    vtkDataSet::SafeDownCast(pointGradients->GetOutput())->GetPointData()->GetArray(resultName));

  vtkDoubleArray* correctPointArray =
    vtkArrayDownCast<vtkDoubleArray>(vtkDataSet::SafeDownCast(correctPointGradients->GetOutput())
                                       ->GetPointData()
                                       ->GetArray(resultName));

  if (!IsGradientCorrect(gradPointArray, correctPointArray))
  {
    return EXIT_FAILURE;
  }

  // now check on the vorticity calculations
  VTK_CREATE(vtkmGradient, cellVorticity);
  cellVorticity->SetInputData(grid);
  cellVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_CELLS, fieldName);
  cellVorticity->SetResultArrayName(resultName);
  cellVorticity->SetComputeVorticity(1);
  cellVorticity->Update();

  VTK_CREATE(vtkmGradient, pointVorticity);
  pointVorticity->SetInputData(grid);
  pointVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointVorticity->SetResultArrayName(resultName);
  pointVorticity->SetComputeVorticity(1);
  pointVorticity->SetComputeQCriterion(1);
  pointVorticity->SetComputeDivergence(1);
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

  vtkDoubleArray* divergencePointArray = vtkArrayDownCast<vtkDoubleArray>(
    vtkDataSet::SafeDownCast(pointVorticity->GetOutput())->GetPointData()->GetArray("Divergence"));
  if (!IsDivergenceCorrect(gradPointArray, divergencePointArray))
  {
    return EXIT_FAILURE;
  }

  vtkDoubleArray* qCriterionPointArray = vtkArrayDownCast<vtkDoubleArray>(
    vtkDataSet::SafeDownCast(pointVorticity->GetOutput())->GetPointData()->GetArray("Q-criterion"));
  if (!IsQCriterionCorrect(gradPointArray, qCriterionPointArray))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
} // end local namespace

//-----------------------------------------------------------------------------
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

  vtkStdString filename(std::string(data_root) + "/Data/SampleStructGrid.vtk");
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
