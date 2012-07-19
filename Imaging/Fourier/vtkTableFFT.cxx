// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTableFFT.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkTableFFT.h"

#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageFFT.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkTable.h"

#include "vtkSmartPointer.h"
#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

#include <string.h>

#include <vtksys/SystemTools.hxx>
using namespace vtksys;

//=============================================================================
vtkStandardNewMacro(vtkTableFFT);

//-----------------------------------------------------------------------------
vtkTableFFT::vtkTableFFT()
{
}

vtkTableFFT::~vtkTableFFT()
{
}

void vtkTableFFT::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int vtkTableFFT::RequestData(vtkInformation *vtkNotUsed(request),
                             vtkInformationVector **inputVector,
                             vtkInformationVector *outputVector)
{
  vtkTable *input = vtkTable::GetData(inputVector[0]);
  vtkTable *output = vtkTable::GetData(outputVector);

  if (!input || !output)
    {
    vtkWarningMacro(<< "No input or output.");
    return 0;
    }

  vtkIdType numColumns = input->GetNumberOfColumns();
  for (vtkIdType col = 0; col < numColumns; col++)
    {
    this->UpdateProgress((double)col/numColumns);

    vtkDataArray *array = vtkDataArray::SafeDownCast(input->GetColumn(col));
    if (!array) continue;
    if (array->GetNumberOfComponents() != 1) continue;
    if (array->GetName())
      {
      if (SystemTools::Strucmp(array->GetName(),"time") == 0) continue;
      if (strcmp(array->GetName(), "vtkValidPointMask") == 0)
        {
        output->AddColumn(array);
        continue;
        }
      }
    if (array->IsA("vtkIdTypeArray")) continue;

    vtkSmartPointer<vtkDataArray> frequencies = this->DoFFT(array);
    frequencies->SetName(array->GetName());
    output->AddColumn(frequencies);
    }

  return 1;
}

//-----------------------------------------------------------------------------
vtkSmartPointer<vtkDataArray> vtkTableFFT::DoFFT(vtkDataArray *input)
{
  // Build an image data containing the input data.
  VTK_CREATE(vtkImageData, imgInput);
  imgInput->SetDimensions(input->GetNumberOfTuples(), 1, 1);
  imgInput->SetScalarType(input->GetDataType(), input->GetInformation());
  imgInput->GetPointData()->SetScalars(input);

  // Compute the FFT
  VTK_CREATE(vtkImageFFT, fft);
  fft->SetInputData(imgInput);
  fft->Update();

  // Return the result
  return vtkSmartPointer<vtkDataArray>(fft->GetOutput()->GetPointData()->GetScalars());
}
