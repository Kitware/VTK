/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCursor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCursor3D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkImageCursor3D);

//----------------------------------------------------------------------------
void vtkImageCursor3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;

  os << indent << "Cursor Radius: " << this->CursorRadius << "\n";
  os << indent << "Cursor Value: " << this->CursorValue << "\n";
  os << indent << "Cursor Position: (" << this->CursorPosition[0];
  for (idx = 1; idx < 3; ++idx)
    {
    os << ", " << this->CursorPosition[idx];
    }
  os << ")\n";
}

//----------------------------------------------------------------------------
vtkImageCursor3D::vtkImageCursor3D()
{
  this->CursorPosition[0] = 0;
  this->CursorPosition[1] = 0;
  this->CursorPosition[2] = 0;

  this->CursorRadius = 5;
  this->CursorValue = 255;
}



template <class T>
void vtkImageCursor3DExecute(vtkImageCursor3D *self,
                             vtkImageData *outData, T *ptr)
{
  int min0, max0, min1, max1, min2, max2;
  int c0, c1, c2;
  int idx;
  double value;
  int rad = self->GetCursorRadius();

  c0 = static_cast<int>(self->GetCursorPosition()[0]);
  c1 = static_cast<int>(self->GetCursorPosition()[1]);
  c2 = static_cast<int>(self->GetCursorPosition()[2]);
  value = self->GetCursorValue();

  outData->GetExtent(min0, max0, min1, max1, min2, max2);

  if (c1 >= min1 && c1 <= max1 && c2 >= min2 && c2 <= max2)
    {
    for (idx = c0 - rad; idx <= c0 + rad; ++idx)
      {
      if (idx >= min0 && idx <= max0)
        {
        ptr = static_cast<T *>(outData->GetScalarPointer(idx, c1, c2));
        *ptr = static_cast<T>(value);
        }
      }
    }


  if (c0 >= min0 && c0 <= max0 && c2 >= min2 && c2 <= max2)
    {
    for (idx = c1 - rad; idx <= c1 + rad; ++idx)
      {
      if (idx >= min1 && idx <= max1)
        {
        ptr = static_cast<T *>(outData->GetScalarPointer(c0, idx, c2));
        *ptr = static_cast<T>(value);
        }
      }
    }


  if (c0 >= min0 && c0 <= max0 && c1 >= min1 && c1 <= max1)
    {
    for (idx = c2 - rad; idx <= c2 + rad; ++idx)
      {
      if (idx >= min2 && idx <= max2)
        {
        ptr = static_cast<T *>(outData->GetScalarPointer(c0, c1, idx));
        *ptr = static_cast<T>(value);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Split up into finished and border datas.  Fill the border datas.
int vtkImageCursor3D::RequestData(
  vtkInformation* request,
  vtkInformationVector** inputVector,
  vtkInformationVector* outputVector)
{
  void *ptr = NULL;

  // let superclass allocate data
  this->Superclass::RequestData(request, inputVector, outputVector);

  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData =
    vtkImageData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  switch (outData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCursor3DExecute(this,outData, static_cast<VTK_TT *>(ptr)));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }

  return 1;
}




