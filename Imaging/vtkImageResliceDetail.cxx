/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMath.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "vtkImageResliceDetail.h"
#include "vtkImageData.h"
#include "vtkMatrix4x4.h"


void vtkGetCastPixelsFunc(vtkImageResliceBase *self,
          void (**castpixels)(void *&out, const double *in, int numscalars ))
{
  int dataType = self->GetOutput()->GetScalarType();
  int numscalars = self->GetOutput()->GetNumberOfScalarComponents();

  switch (numscalars)
    {
    case 1:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *castpixels = &vtkResliceCastPixels<VTK_TT>::Cast1
          );
        default:
          *castpixels = 0;
        }
    default:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *castpixels = &vtkResliceCastPixels<VTK_TT>::Cast
          );
        default:
          *castpixels = 0;
        }
    }
}

//----------------------------------------------------------------------------
// This function simply clears the entire output to the background color,
// for cases where the transformation places the output extent completely
// outside of the input extent.
void vtkImageResliceClearExecute(vtkImageResliceBase *self,
                                 vtkImageData *, void *,
                                 vtkImageData *outData, void *outPtr,
                                 int outExt[6], int threadId)
{
  int numscalars;
  int idY, idZ;
  vtkIdType outIncX, outIncY, outIncZ;
  int scalarSize;
  unsigned long count = 0;
  unsigned long target;
  void *background;
  void (*setpixels)(void *&out, const void *in, int numscalars, int n);

  // for the progress meter
  target = static_cast<unsigned long>
    ((outExt[5]-outExt[4]+1)*(outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Get Increments to march through data
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  scalarSize = outData->GetScalarSize();
  numscalars = outData->GetNumberOfScalarComponents();

  // allocate a voxel to copy into the background (out-of-bounds) regions
  vtkAllocBackgroundPixel(self, &background, numscalars);
  // get the appropriate function for pixel copying
  vtkGetSetPixelsFunc(self, &setpixels);

  // Loop through output voxels
  for (idZ = outExt[4]; idZ <= outExt[5]; idZ++)
    {
    for (idY = outExt[2]; idY <= outExt[3]; idY++)
      {
      if (threadId == 0)
        { // update the progress if this is the main thread
        if (!(count%target))
          {
          self->UpdateProgress(count/(50.0*target));
          }
        count++;
        }
      // clear the pixels to background color and go to next row
      setpixels(outPtr, background, numscalars, outExt[1]-outExt[0]+1);
      outPtr = static_cast<void *>(
        static_cast<char *>(outPtr) + outIncY*scalarSize);
      }
    outPtr = static_cast<void *>(
      static_cast<char *>(outPtr) + outIncZ*scalarSize);
    }

  vtkFreeBackgroundPixel(self, &background);
}

//----------------------------------------------------------------------------
void vtkAllocBackgroundPixel(vtkImageResliceBase *self, void **rval,
                             int numComponents)
{
  vtkImageData *output = self->GetOutput();
  int scalarType = output->GetScalarType();
  int bytesPerPixel = numComponents*output->GetScalarSize();

  // allocate as an array of doubles to guarantee alignment
  // (this is probably more paranoid than necessary)
  int n = (bytesPerPixel + VTK_SIZEOF_DOUBLE - 1)/VTK_SIZEOF_DOUBLE;
  double *doublePtr = new double[n];
  *rval = doublePtr;

  switch (scalarType)
    {
    vtkTemplateAliasMacro(vtkCopyBackgroundColor(self, (VTK_TT *)(*rval),
                                                 numComponents));
    }
}

//----------------------------------------------------------------------------
// get a pixel copy function that is appropriate for the data type
void vtkGetSetPixelsFunc(vtkImageResliceBase *self,
                         void (**setpixels)(void *&out, const void *in,
                                            int numscalars, int n))
{
  vtkImageData *output = self->GetOutput();
  int dataType = output->GetScalarType();
  int dataSize = output->GetScalarSize();
  int numscalars = output->GetNumberOfScalarComponents();
  void *dataPtr = output->GetScalarPointer();

  // If memory is 4-byte aligned, copy in 4-byte chunks
  if (vtkImageReslicePointerAlignment(dataPtr, 4) &&
      ((dataSize*numscalars) & 0x03) == 0 &&
      dataSize < 4 && dataSize*numscalars <= 16)
    {
    switch ((dataSize*numscalars) >> 2)
      {
      case 1:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set1;
        break;
      case 2:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set2;
        break;
      case 3:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set3;
        break;
      case 4:
        *setpixels = &vtkImageResliceSetPixels<vtkTypeInt32>::Set4;
        break;
      }
    return;
    }

  switch (numscalars)
    {
    case 1:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set1
          );
        default:
          *setpixels = 0;
        }
    case 2:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set2
          );
        default:
          *setpixels = 0;
        }
    case 3:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set3
          );
        default:
          *setpixels = 0;
        }
    case 4:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set4
          );
        default:
          *setpixels = 0;
        }
    default:
      switch (dataType)
        {
        vtkTemplateAliasMacro(
          *setpixels = &vtkImageResliceSetPixels<VTK_TT>::Set
          );
        default:
          *setpixels = 0;
        }
    }
}

//----------------------------------------------------------------------------
// check a matrix to see whether it is the identity matrix
int vtkIsIdentityMatrix(vtkMatrix4x4 *matrix)
{
  static double identity[16] = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
  int i,j;

  for (i = 0; i < 4; i++)
    {
    for (j = 0; j < 4; j++)
      {
      if (matrix->GetElement(i,j) != identity[4*i+j])
        {
        return 0;
        }
      }
    }
  return 1;
}
