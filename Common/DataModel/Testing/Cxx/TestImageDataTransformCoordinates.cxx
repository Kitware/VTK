// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkImageData.h"

int TestImageDataTransformCoordinates(int, char*[])
{
  vtkNew<vtkImageData> srcImage;
  srcImage->SetDimensions(2, 2, 1);
  srcImage->SetOrigin(4., 4., 0.);
  srcImage->SetSpacing(1., 1., 0.);

  // The conversion matrices are not cleared between computations, and compute with each setter.
  // Shallow copying forces the computation with all parameters already set. With zero spacing,
  // the IndexToPhysicalMatrix is a singular matrix and used to cause a silent failure in the
  // computation of the PhysicalToIndexMatrix.
  vtkNew<vtkImageData> image;
  image->ShallowCopy(srcImage);

  bool success = true;
  {
    double ijk[3] = { 0., 0., 0. };
    double xyz[3];
    image->TransformContinuousIndexToPhysicalPoint(ijk, xyz);

    // Index -> Physical is fine
    success &= (xyz[0] == 4. && xyz[1] == 4. && xyz[2] == 0.);
  }

  {
    double xyz[3] = { 5., 5., 0. };
    double ijk[3];

    // if the Physical -> Index matrix is not properly computed, this method returns wrong results
    image->TransformPhysicalPointToContinuousIndex(xyz, ijk);
    success &= (ijk[0] == 1. && ijk[1] == 1. && ijk[2] == 0.);
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
