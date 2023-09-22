// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkConeSource.h"
#include "vtkCubeSource.h"
#include "vtkCylinderSource.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkSphereSource.h"

#include <array>
#include <cstdlib>

int TestCompositePolyDataMapperStaticBounds(int, char*[])
{
  vtkNew<vtkConeSource> cone;
  cone->SetRadius(1.0);
  cone->SetHeight(2.0);
  vtkNew<vtkCylinderSource> cylinder;
  cylinder->SetHeight(2.0);
  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  cone->Update();
  cylinder->Update();
  pdsc->SetPartition(0, 0, cone->GetOutput());
  pdsc->SetPartition(1, 0, cylinder->GetOutput());

  vtkNew<vtkCompositePolyDataMapper> cpdm;
  cpdm->SetInputDataObject(pdsc);
  cpdm->SetStatic(true);

  std::array<double, 6> bounds;
  cpdm->GetBounds(bounds.data());
  bool success = true;
  for (int i = 0; i < 6; i += 2)
  {
    success &= (bounds[i] != 1.0);
  }
  for (int i = 1; i < 6; i += 2)
  {
    success &= (bounds[i] != -1.0);
  }

  // now add a new dataset and ask the mapper for bounds. It should not be equal to previous bounds.
  std::array<double, 6> cubeBounds = { -10.0, 10.0, -15.0, 15.0, -20.0, 20.0 };
  vtkNew<vtkCubeSource> cube;
  cube->SetCenter(0.0, 0.0, 0.0);
  cube->SetBounds(cubeBounds.data());
  cube->Update();
  pdsc->SetPartition(2, 0, cube->GetOutput());

  std::array<double, 6> newBounds;
  cpdm->GetBounds(newBounds.data());
  for (int i = 0; i < 6; ++i)
  {
    // newBounds must not equal original bounds
    // newBounds must equal the bounds of the big cube we added.
    success &= ((bounds[i] != newBounds[i]) && (newBounds[i] == cubeBounds[i]));
  }
  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
