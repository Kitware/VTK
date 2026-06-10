// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridGradient.h"
#include "vtkMathUtilities.h"
#include "vtkNew.h"
#include "vtkRandomHyperTreeGridSource.h"
#include "vtkTestUtilities.h"

#include <cmath>
#include <cstdlib>
#include <iostream>

#define CHECK_GRADIENT(array, tupleId, x, y, z)                                                    \
  do                                                                                               \
  {                                                                                                \
    double grad[3];                                                                                \
    array->GetTuple((tupleId), grad);                                                              \
    if (!vtkMathUtilities::FuzzyCompare(grad[0], x, 1e-4) ||                                       \
      !vtkMathUtilities::FuzzyCompare(grad[1], y, 1e-4) ||                                         \
      !vtkMathUtilities::FuzzyCompare(grad[2], z, 1e-4))                                           \
    {                                                                                              \
      std::cerr << "Tuple " << tupleId << " expected (" << x << ", " << y << ", " << z             \
                << ") but got (" << grad[0] << ", " << grad[1] << ", " << grad[2] << ")."          \
                << std::endl;                                                                      \
      return EXIT_FAILURE;                                                                         \
    }                                                                                              \
  } while (0)

//------------------------------------------------------------------------------
int TestHyperTreeGridGradient(int, char*[])
{
  // Testing Unlimited / No masking
  vtkNew<vtkRandomHyperTreeGridSource> source;
  source->SetDimensions(4, 4, 4);
  source->SetMaxDepth(3);
  source->SetSeed(1);
  source->Update();

  vtkNew<vtkHyperTreeGridGradient> gradient;
  gradient->SetInputConnection(source->GetOutputPort());
  gradient->SetMode(vtkHyperTreeGridGradient::ComputeMode::UNLIMITED);
  gradient->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_CELLS, "Depth");
  gradient->Update();

  vtkHyperTreeGrid* output = vtkHyperTreeGrid::SafeDownCast(gradient->GetOutput());
  vtkDataArray* gradArray = output->GetCellData()->GetArray("Gradient");

  CHECK_GRADIENT(gradArray, 10, -16.0, 12.2, 14.2);
  CHECK_GRADIENT(gradArray, 50, -15.2, 1.0, 1.0);

  // Testing Unstructured / No masking
  gradient->SetMode(vtkHyperTreeGridGradient::ComputeMode::UNSTRUCTURED);
  gradient->Update();

  CHECK_GRADIENT(gradArray, 10, -3.65515, 3.20863, 3.96653);
  CHECK_GRADIENT(gradArray, 50, -0.52823, 0.160766, 0.160766);

  // Testing Unstructured / Masking
  source->SetMaskedFraction(0.25);
  gradient->Update();

  CHECK_GRADIENT(gradArray, 10, -3.05802, 0.682319, 2.93304);
  CHECK_GRADIENT(gradArray, 15, 3.18086, -3.61722, -1.43541);

  // Testing Unlimited / Masking
  gradient->SetMode(vtkHyperTreeGridGradient::ComputeMode::UNLIMITED);
  gradient->Update();

  CHECK_GRADIENT(gradArray, 10, -14.2, 1.4, 10.4);
  CHECK_GRADIENT(gradArray, 15, 11.7, -11.9, -4.4);

  return EXIT_SUCCESS;
}
