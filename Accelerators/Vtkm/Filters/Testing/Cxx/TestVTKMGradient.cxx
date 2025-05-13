// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkArrayCalculator.h"
#include "vtkCell.h"
#include "vtkDataArrayAccessor.h"
#include "vtkDataSet.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkUnstructuredGrid.h"

#include "vtkmCleanGrid.h"
#include "vtkmFilterOverrides.h"
#include "vtkmGradient.h"

#include <viskores/testing/Testing.h>

namespace
{
double Tolerance = 0.00001;

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
  pointGradients->ForceVTKmOn();
  pointGradients->SetInputConnection(calculator->GetOutputPort());
  pointGradients->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointGradients->SetResultArrayName(resultName);

  vtkmFilterOverrides::EnabledOff(); // Turn off override to instantiate VTK filter
  vtkNew<vtkGradientFilter> correctPointGradients;
  vtkmFilterOverrides::EnabledOn();
  correctPointGradients->SetInputConnection(calculator->GetOutputPort());
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

  vtkNew<vtkmGradient> pointVorticity;
  pointVorticity->ForceVTKmOn();
  pointVorticity->SetInputConnection(calculator->GetOutputPort());
  pointVorticity->SetInputScalars(vtkDataObject::FIELD_ASSOCIATION_POINTS, fieldName);
  pointVorticity->SetResultArrayName(resultName);
  pointVorticity->SetComputeVorticity(1);
  pointVorticity->SetComputeQCriterion(1);
  pointVorticity->SetComputeDivergence(1);
  pointVorticity->Update();

  // point stuff
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
