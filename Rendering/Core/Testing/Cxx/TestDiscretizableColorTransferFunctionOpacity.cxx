/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiscretizableColorTransferFunction.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDiscretizableColorTransferFunction.h"
#include "vtkDoubleArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkSmartPointer.h"
#include "vtkUnsignedCharArray.h"

#include <cstring>

//----------------------------------------------------------------------------
int TestDiscretizableColorTransferFunctionOpacity(int, char*[])
{
  // Discretizable color transfer function
  const double controlPoints[] = { 0.0, 1.0, 0.0, 0.0, 255.0, 0.0, 0.0, 1.0 };

  vtkSmartPointer<vtkDiscretizableColorTransferFunction> dctf =
    vtkSmartPointer<vtkDiscretizableColorTransferFunction>::New();
  for (int i = 0; i < 2; ++i)
  {
    const double* xrgb = controlPoints + (i * 4);
    dctf->AddRGBPoint(xrgb[0], xrgb[1], xrgb[2], xrgb[3]);
  }

  // Scalar opacity transfer function
  const double opacityControlPoints[] = { 0.0, 0.0, 255.0, 0.5 };

  vtkSmartPointer<vtkPiecewiseFunction> pf = vtkSmartPointer<vtkPiecewiseFunction>::New();
  for (int i = 0; i < 2; ++i)
  {
    const double* xalpha = opacityControlPoints + (i * 2);
    pf->AddPoint(xalpha[0], xalpha[1]);
  }

  // Enable opacity mapping
  dctf->SetScalarOpacityFunction(pf);
  dctf->EnableOpacityMappingOn();
  dctf->Build();

  // Input scalars
  double inputScalars[] = { 0.0, 127.0, 255.0 };
  vtkSmartPointer<vtkDoubleArray> da = vtkSmartPointer<vtkDoubleArray>::New();
  for (int i = 0; i < 3; i++)
  {
    da->InsertNextTuple1(inputScalars[i]);
  }
  // Output colors
  unsigned char mapScalarsThroughTableOutput[12];
  vtkSmartPointer<vtkUnsignedCharArray> mapScalarsOutput;

  //--------------------------------------------------------------------------
  //  Colors mapping only. Output format = VTK_RGB
  //--------------------------------------------------------------------------

  // Map void* array to opacity using first entry point
  dctf->MapScalarsThroughTable(
    inputScalars, mapScalarsThroughTableOutput, VTK_DOUBLE, 3, 1, VTK_RGB);
  // Map data array to opacity using second entry point
  mapScalarsOutput.TakeReference(dctf->MapScalars(da, VTK_COLOR_MODE_DEFAULT, -1));

  unsigned char* mapScalarsOutputPtr =
    reinterpret_cast<unsigned char*>(mapScalarsOutput->GetVoidPointer(0));
  for (int i = 0; i < 3; ++i)
  {
    for (int k = 0; k < 3; ++k)
    {
      if (mapScalarsThroughTableOutput[i * 3 + k] != mapScalarsOutputPtr[i * 4 + k])
      {
        return EXIT_FAILURE;
      }
    }
  }
  //--------------------------------------------------------------------------
  //  Colors and opacity mapping. Output format = VTK_RGBA
  //--------------------------------------------------------------------------

  // Map void* array to opacity using first entry point
  dctf->MapScalarsThroughTable(
    inputScalars, mapScalarsThroughTableOutput, VTK_DOUBLE, 3, 1, VTK_RGBA);
  // Map data array to opacity using second entry point
  mapScalarsOutput.TakeReference(dctf->MapScalars(da, VTK_COLOR_MODE_MAP_SCALARS, -1));

  if (std::memcmp(mapScalarsThroughTableOutput, mapScalarsOutput->GetVoidPointer(0),
        3 * 4 * sizeof(unsigned char)))
  {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
