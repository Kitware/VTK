// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLogger.h"
#include "vtkPointSet.h"
#include "vtkPointsMatchingTransformFilter.h"
#include "vtkSphereSource.h"

namespace
{
bool CheckFirstPointPosition(vtkPointsMatchingTransformFilter* filter, double groundtruth[3])
{
  vtkPointSet* data = filter->GetOutput();
  double* point = data->GetPoints()->GetPoint(0);
  if (point[0] == groundtruth[0] && point[1] == groundtruth[1] && point[2] == groundtruth[2])
  {
    return true;
  }
  return false;
}
}

int TestPointsMatchingTransformFilter(int, char*[])
{
  vtkNew<vtkSphereSource> sphere;
  vtkNew<vtkPointsMatchingTransformFilter> transformFilter;

  transformFilter->SetInputConnection(sphere->GetOutputPort());
  transformFilter->Update();

  // Test identity
  double* point0 = sphere->GetOutput()->GetPoints()->GetPoint(0);
  double groundtruth[3] = { point0[0], point0[1], point0[2] };
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "Default transform should be identity.");
    return EXIT_FAILURE;
  }

  // Test non invertible matrix
  transformFilter->SetSourcePoint2(0.0, 0.0, 0.0);
  transformFilter->SetSourcePoint3(0.0, 0.0, 0.0);
  transformFilter->SetSourcePoint4(0.0, 0.0, 0.0);
  transformFilter->Update();
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "Non invertible source matrix should result in identity.");
    return EXIT_FAILURE;
  }

  // Test scale
  transformFilter->SetSourcePoint2(0.5, 0.0, 0.0);
  transformFilter->SetSourcePoint3(0.0, 0.5, 0.0);
  transformFilter->SetSourcePoint4(0.0, 0.0, 0.5);
  transformFilter->Update();
  groundtruth[0] = 2.0 * point0[0];
  groundtruth[1] = 2.0 * point0[1];
  groundtruth[2] = 2.0 * point0[2];
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "A scale factor of 2 should be applied.");
    return EXIT_FAILURE;
  }

  // Test translation
  transformFilter->SetTargetPoint1(0.5, 0.5, 0.5);
  transformFilter->SetTargetPoint2(1.0, 0.5, 0.5);
  transformFilter->SetTargetPoint3(0.5, 1.0, 0.5);
  transformFilter->SetTargetPoint4(0.5, 0.5, 1.0);
  transformFilter->Update();
  groundtruth[0] = point0[0] + 0.5;
  groundtruth[1] = point0[1] + 0.5;
  groundtruth[2] = point0[2] + 0.5;
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "Source should be translated by (0.5, 0.5, 0.5).");
    return EXIT_FAILURE;
  }

  // Test rotation
  transformFilter->SetTargetPoint1(0.0, 0.0, 0.0);
  transformFilter->SetTargetPoint2(0.5, 0.0, 0.0);
  transformFilter->SetTargetPoint3(0.0, 0.0, 0.5);
  transformFilter->SetTargetPoint4(0.0, -0.5, 0.0);
  transformFilter->Update();
  groundtruth[0] = point0[0];
  groundtruth[1] = -point0[2];
  groundtruth[2] = point0[1];
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "Source should be rotated by 90° on the zx axis.");
    return EXIT_FAILURE;
  }

  // Test RigidTransform option with rigid transform
  transformFilter->RigidTransformOn();
  transformFilter->Update();
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "For rigid transform, the rigid option should not modify the output.");
    return EXIT_FAILURE;
  }

  // Test RigidTransform option with non rigid transform
  transformFilter->SetTargetPoint1(0.0, 0.1, 0.0);
  groundtruth[0] = -0.048361159861087799072265625;
  groundtruth[1] = -0.4319727718830108642578125;
  groundtruth[2] = -0.048361159861087799072265625;
  transformFilter->Update();
  if (!::CheckFirstPointPosition(transformFilter, groundtruth))
  {
    vtkLog(ERROR, "Polar projection seems invalid.");
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
