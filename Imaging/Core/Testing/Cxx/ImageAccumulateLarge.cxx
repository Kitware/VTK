/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageAccumulateLarge.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"
#include "vtkImageData.h"
#include "vtkImageAccumulate.h"
#include "vtkPointData.h"
#include "vtkDataArray.h"
#include "vtkDoubleArray.h"

int ImageAccumulateLarge(int argc, char *argv[])
{
  vtkIdType dim;
  if (argc < 2)
    {
    std::cout << "Usage: " << argv[0] << " dimension" << std::endl;
    return EXIT_FAILURE;
    }
  // For routine testing (nightly, local) we keep these dimensions small
  // To test bin overflow, change the 32, to 2048
  dim = atoi(argv[1]);

  // Allocate an image
  vtkSmartPointer<vtkImageData> image =
    vtkSmartPointer<vtkImageData>::New();
  image->SetDimensions(dim,dim,dim);
  image->AllocateScalars(VTK_UNSIGNED_CHAR,1);

  // Initialize the image with zeroes and ones
  vtkIdType oneBinExpected = 10;
  vtkIdType zeroBinExpected = dim * dim * dim - oneBinExpected;

  memset(static_cast<void *>(
           image->GetScalarPointer(oneBinExpected, 0, 0)), 0, zeroBinExpected);
  memset(static_cast<void *>(
           image->GetScalarPointer(0, 0, 0)), 1, oneBinExpected);

  vtkSmartPointer<vtkImageAccumulate> filter =
    vtkSmartPointer<vtkImageAccumulate>::New();
  filter->SetInputData(image);
  filter->SetComponentExtent(0, 1, 0, 0, 0, 0);
  filter->SetComponentOrigin(0, 0, 0);
  filter->SetComponentSpacing(1, 1, 1);
  filter->Update();
#define OT vtkIdType
  vtkIdType zeroBinResult =
    static_cast<vtkIdType>(*(static_cast<OT *> (filter->GetOutput()->GetScalarPointer(0,0,0))));
  vtkIdType oneBinResult =
    static_cast<vtkIdType>(*(static_cast<OT *> (filter->GetOutput()->GetScalarPointer(1,0,0))));
  int status = EXIT_SUCCESS;
  if (zeroBinResult != zeroBinExpected)
    {
    std::cout << "Expected the 0 bin count to be " << zeroBinExpected << " but got " << zeroBinResult << std::endl;
    status = EXIT_FAILURE;
    }
  if (oneBinResult != oneBinExpected)
    {
    std::cout << "Expected the 1 bin count to be " << oneBinExpected << " but got " << oneBinResult << std::endl;
    status = EXIT_FAILURE;
    }

  return status;
}
