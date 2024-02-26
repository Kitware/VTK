// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkHyperTreeGrid.h"
#include "vtkHyperTreeGridAxisClip.h"
#include "vtkHyperTreeGridOrientedGeometryCursor.h"
#include "vtkHyperTreeGridPreConfiguredSource.h"
#include "vtkIdTypeArray.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"

#include <cstdlib>
#include <numeric>

namespace
{
constexpr double EPSILON = 0.1;
constexpr double CLIP_BOUNDS[6] = { -0.5, 0.5, 0., 1., -1., 0.1 };

bool CheckClippedBounds(vtkHyperTreeGrid* htg)
{
  vtkNew<vtkHyperTreeGridAxisClip> clip;

  clip->SetInputDataObject(htg);
  clip->SetClipTypeToBox();
  clip->SetBounds(::CLIP_BOUNDS);
  clip->SetInsideOut(false);
  clip->Update();
  auto clipped = clip->GetOutputDataObject(0);

  if (!clipped)
  {
    std::cerr << "Clipped is nullptr" << std::endl;
    return false;
  }

  vtkHyperTreeGrid* out = vtkHyperTreeGrid::SafeDownCast(clipped);
  if (!out)
  {
    std::cerr << "Clip failed to provide a vtkHyperTreeGrid" << std::endl;
    return false;
  }

  double* bounds = out->GetBounds();
  for (int idx = 0; idx < 6; idx++)
  {
    if (std::abs(bounds[idx] - ::CLIP_BOUNDS[idx]) > ::EPSILON)
    {
      std::cerr << "Clipped output does not have valid bounds." << std::endl;
      return false;
    }
  }
  return true;
}
}

int TestHyperTreeGridBounds(int, char*[])
{
  vtkNew<vtkHyperTreeGridPreConfiguredSource> htgSrc;
  htgSrc->SetHTGMode(vtkHyperTreeGridPreConfiguredSource::BALANCED_2DEPTH_3BRANCH_3X3X2);
  htgSrc->Update();

  vtkHyperTreeGrid* input = vtkHyperTreeGrid::SafeDownCast(htgSrc->GetOutputDataObject(0));
  if (!input)
  {
    std::cerr << "Something went wrong with HTG generation, input is nullptr" << std::endl;
    return EXIT_FAILURE;
  }

  bool res = ::CheckClippedBounds(input);

  return res ? EXIT_SUCCESS : EXIT_FAILURE;
}
