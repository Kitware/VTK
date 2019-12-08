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
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkUnstructuredGrid.h"
#include "vtkmCleanGrid.h"
#include "vtkmGradient.h"

#include <vtkm/testing/Testing.h>

#include <vector>

namespace
{
double Tolerance = 0.00001;

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

      std::cout << "Gradient[ " << i << " ] should look like: " << std::endl;
      std::cout << expected[0] << ", " << expected[1] << ", " << expected[2] << std::endl;
      if (numberOfComponents > 3)
      {
        std::cout << expected[3] << ", " << expected[4] << ", " << expected[5] << std::endl;
        std::cout << expected[6] << ", " << expected[7] << ", " << expected[8] << std::endl;
      }

      std::cout << "Gradient[ " << i << " ] actually looks like: " << std::endl;
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
  const char fieldName[] = "LinearField";

  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputData(grid);
  calculator->SetResultArrayName(fieldName);
  calculator->SetFunction("coordsY*iHat+coordsX*jHat+coordsZ*kHat");
  calculator->SetAttributeTypeToPointData();
  calculator->AddCoordinateScalarVariable("coordsX", 0);
  calculator->AddCoordinateScalarVariable("coordsY", 1);
  calculator->AddCoordinateScalarVariable("coordsZ", 2);

  const char resultName[] = "Result";

  vtkNew<vtkmGradient> pointGradients;
  pointGradients->SetInputConnection(calculator->GetOutputPort());
  pointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  vtkNew<vtkGradientFilter> correctPointGradients;
  correctPointGradients->SetInputConnection(calculator->GetOutputPort());
  correctPointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  correctPointGradients->SetResultArrayName(resultName);

  pointGradients->Update();
  correctPointGradients->Update();

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

  vtkNew<vtkmGradient> pointVorticity;
  pointVorticity->SetInputConnection(calculator->GetOutputPort());
  pointVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointVorticity->SetResultArrayName(resultName);
  pointVorticity->SetComputeVorticity(1);
  pointVorticity->SetComputeQCriterion(1);
  pointVorticity->SetComputeDivergence(1);
  pointVorticity->Update();

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
int TestVTKMGradient(int /* argc */, char* /* argv */[])
{
  vtkDataSet* grid = nullptr;

  vtkNew<vtkRTAnalyticSource> wavelet;
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetCenter(0, 0, 0);
  wavelet->Update();

  grid = vtkDataSet::SafeDownCast(wavelet->GetOutput());

  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // convert the structured grid to an unstructured grid
  vtkNew<vtkmCleanGrid> ug;
  ug->SetInputConnection(wavelet->GetOutputPort());
  ug->Update();

  grid = vtkDataSet::SafeDownCast(ug->GetOutput());
  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // now try with 2D wavelets
  wavelet->SetWholeExtent(-10, 10, -10, 10, 0, 0);
  wavelet->SetCenter(0, 0, 0);
  wavelet->Update();

  grid = vtkDataSet::SafeDownCast(wavelet->GetOutput());
  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  // convert the 2D structured grid to an unstructured grid
  ug->Update();

  grid = vtkDataSet::SafeDownCast(ug->GetOutput());
  if (PerformTest(grid))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
