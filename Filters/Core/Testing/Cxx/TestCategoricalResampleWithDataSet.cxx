/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalResampleWithDataSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkNew.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkResampleWithDataSet.h"
#include "vtkSphereSource.h"

#include <cmath>

int TestCategoricalResampleWithDataSet(int, char *[])
{
  vtkNew<vtkImageData> imageData;
  imageData->SetExtent(-5,5,-5,5,-5,5);
  imageData->AllocateScalars(VTK_DOUBLE,1);

  int* ext = imageData->GetExtent();

  double radius = 3.;
  double inValue = 10.;
  double outValue = -10.;

  for (int z = ext[0]; z < ext[1]; z++)
  {
    for (int y = ext[2]; y < ext[3]; y++)
    {
      for (int x = ext[4]; x < ext[5]; x++)
      {
        double* p = static_cast<double*>(imageData->GetScalarPointer(x,y,z));
        if (x*x + y*y + z*z < radius*radius)
        {
          p[0] = inValue;
        }
        else
        {
          p[0] = outValue;
        }
      }
    }
  }

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(radius);

  vtkNew<vtkResampleWithDataSet> probeFilter;
  probeFilter->SetInputConnection(sphere->GetOutputPort());
  probeFilter->SetSourceData(imageData.GetPointer());
  probeFilter->SetCategoricalData(true);
  probeFilter->Update();

  vtkDataSet* outputData = vtkDataSet::SafeDownCast(probeFilter->GetOutput());

  vtkDoubleArray* values = vtkDoubleArray::SafeDownCast(outputData->
                                                        GetPointData()->
                                                        GetScalars());

  static const double epsilon = 1.e-8;

  for (vtkIdType i=0;i<values->GetNumberOfValues();i++)
  {
    if (std::fabs(values->GetValue(i) - inValue) > epsilon &&
        std::fabs(values->GetValue(i) - outValue) > epsilon)
      {
        return EXIT_FAILURE;
      }
  }
  return EXIT_SUCCESS;
}
