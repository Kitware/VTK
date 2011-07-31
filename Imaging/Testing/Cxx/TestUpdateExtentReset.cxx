/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageWeightedSum.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include <vtkColorTransferFunction.h>
#include <vtkImageData.h>
#include <vtkImageReslice.h>
#include <vtkImageMapToColors.h>
#include <vtkImageAppendComponents.h>
#include <vtkSmartPointer.h>

int TestUpdateExtentReset(int vtkNotUsed(argc), char * vtkNotUsed(argv) [] )
{
  vtkSmartPointer<vtkImageData> img = vtkSmartPointer<vtkImageData>::New();
  img->SetDimensions(100, 100, 100);
  img->SetScalarTypeToFloat();
  img->SetNumberOfScalarComponents(1);
  img->AllocateScalars();

  float *scalars = static_cast<float *>(img->GetScalarPointer());
  vtkIdType n = 100*100*100;
  for (vtkIdType i = 0; i < n; i++)
    {
    scalars[i] = 0.0;
    }

  vtkSmartPointer<vtkImageReslice> reslicer = vtkSmartPointer<vtkImageReslice>::New();
  reslicer->SetInput(img);
  reslicer->SetOutputExtent(0, 100, 0, 100, 0, 0);

  vtkSmartPointer<vtkImageMapToColors> colors =
    vtkSmartPointer<vtkImageMapToColors>::New();
  colors->SetInput(reslicer->GetOutput());

  vtkSmartPointer<vtkColorTransferFunction> ctf =
    vtkSmartPointer<vtkColorTransferFunction>::New();
  ctf->AddRGBPoint(0, 1., 0., 0.);
  colors->SetLookupTable(ctf);

  vtkSmartPointer<vtkImageAppendComponents> append =
    vtkSmartPointer<vtkImageAppendComponents>::New();
  append->SetInputConnection(0 ,colors->GetOutput()->GetProducerPort());

  colors->Update();
  append->Update();
  colors->Update();
  reslicer->SetOutputExtent(0, 100, 0, 80, 0, 0);
  colors->Update();

  return EXIT_SUCCESS;
}
