/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageAccumulate.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageData.h"
#include "vtkPointData.h"
#include "vtkSmartPointer.h"
#include "vtkImageOcclusionSpectrum.h"

#include <algorithm>
#include <iterator>

#define for_(i, x, y) for (int i = x, I = y; i < I; ++i)
#define vsp(type, name) \
        vtkSmartPointer<vtk##type> name = vtkSmartPointer<vtk##type>::New()

namespace
{
  template <typename T>
  void print (int const* dim, T const* pointer)
  {
    for_(z,0,dim[2])
      {
      for_(y,0,dim[1])
        {
        for_(x,0,dim[0])
          {
          // cout << vtkstd::fixed << vtkstd::showpos << vtkstd::setprecision(5)
          //     << *pointer++;
          cout << (*pointer++ ? '+' : '-');
          }
        cout << endl;
        }
      cout << endl;
      }
  }
}

int TestImageOcclusionSpectrum (int , char**)
{
  int const dim [3] = {64,64,64};
  vsp(ImageData, image);
    {
    image->SetDimensions(dim);
    image->SetNumberOfScalarComponents(1);

    for_(z,0,dim[2])
    for_(y,0,dim[1])
    for_(x,0,dim[0])
      {
      image->SetScalarComponentFromDouble(x,y,z,0,0);
      }
    image->SetScalarComponentFromDouble(0,0,0,0,1);
    // print(dim, static_cast<double*>
    //   (image->GetPointData()->GetScalars()->GetVoidPointer(0)));
    }

  vsp(ImageOcclusionSpectrum, os);
    {
    os->SetInput(image);
    os->Update();
    // os->PrintSelf(cout, vtkIndent());

    print(dim, static_cast<double*>
      (os->GetOutput()->GetPointData()->GetScalars()->GetVoidPointer(0)));
    }

  return 0;
}
