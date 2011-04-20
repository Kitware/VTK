/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageStencilIterator.txx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Include blockers needed since vtkImageStencilIterator.h includes
// this file when VTK_NO_EXPLICIT_TEMPLATE_INSTANTIATION is defined.

#ifndef __vtkImageStencilIterator_txx
#define __vtkImageStencilIterator_txx

#include "vtkImageStencilIterator.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkAlgorithm.h"

//----------------------------------------------------------------------------
class vtkImageStencilIteratorFriendship
{
  public:

    static int *GetExtentListLengths(vtkImageStencilData *stencil)
      {
      return stencil->ExtentListLengths;
      }

    static int **GetExtentLists(vtkImageStencilData *stencil)
      {
      return stencil->ExtentLists;
      }
};

//----------------------------------------------------------------------------
template <class DType>
vtkImageStencilIterator<DType>::vtkImageStencilIterator()
{
  this->Pointer = 0;
  this->SpanEndPointer = 0;
  this->RowEndPointer = 0;
  this->SliceEndPointer = 0;
  this->EndPointer = 0;

  this->PixelIncrement = 0;
  this->RowEndIncrement = 0;
  this->RowIncrement = 0;
  this->SliceEndIncrement = 0;
  this->SliceIncrement = 0;

  this->HasStencil = false;
  this->InStencil = false;
  this->SpanSliceEndIncrement = 0;
  this->SpanSliceIncrement = 0;
  this->SpanMinX = 0;
  this->SpanMaxX = 0;
  this->SpanMinY = 0;
  this->SpanMaxY = 0;
  this->SpanMinZ = 0;
  this->SpanMaxZ = 0;
  this->SpanIndexX = 0;
  this->SpanIndexY = 0;
  this->SpanIndexZ = 0;
  this->SpanCountPointer = 0;
  this->SpanListPointer = 0;

  this->Algorithm = 0;
  this->Count = 0;
  this->Target = 0;
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageStencilIterator<DType>::Initialize(
  vtkImageData *image, vtkImageStencilData *stencil, int extent[6])
{
  image->GetIncrements(
    this->PixelIncrement, this->RowIncrement, this->SliceIncrement);

  vtkIdType tmp;
  image->GetContinuousIncrements(
    extent, tmp, this->RowEndIncrement, this->SliceEndIncrement);

  this->Pointer = static_cast<DType *>(
    image->GetScalarPointerForExtent(extent));

  this->SpanEndPointer = this->Pointer;
  this->RowEndPointer = this->Pointer;
  this->SliceEndPointer = this->Pointer;
  this->EndPointer = this->Pointer;

  if (extent[1] >= extent[0] &&
      extent[3] >= extent[2] &&
      extent[5] >= extent[4])
    {
    this->SpanEndPointer = this->Pointer +
      this->PixelIncrement*(extent[1] - extent[0] + 1);
    this->RowEndPointer = this->SpanEndPointer;
    this->SliceEndPointer = this->RowEndPointer +
      this->RowIncrement*(extent[3] - extent[2]);
    this->EndPointer = this->SliceEndPointer +
      this->SliceIncrement*(extent[5] - extent[4]);
    }

  // Code for when a stencil is provided
  if (stencil)
    {
    this->HasStencil = true;
    this->InStencil = true;

    this->SpanIndexX = 0;
    this->SpanIndexY = 0;
    this->SpanIndexZ = 0;

    int stencilExtent[6];
    stencil->GetExtent(stencilExtent);

    // The stencil has a YZ array of span lists, we need increments
    // to get to the next Z position in the YZ array.
    this->SpanSliceIncrement = 0;
    this->SpanSliceEndIncrement = 0;

    if (stencilExtent[3] >= stencilExtent[2] &&
        stencilExtent[5] >= stencilExtent[4])
      {
      this->SpanSliceIncrement = stencilExtent[3] - stencilExtent[2] + 1;
      int botOffset = extent[2] - stencilExtent[2];
      if (botOffset >= 0)
        {
        this->SpanSliceEndIncrement += botOffset;
        }
      int topOffset = stencilExtent[3] - extent[3];
      if (topOffset >= 0)
        {
        this->SpanSliceEndIncrement += topOffset;
        }
      }

    // The X extent for the spans we want to iterate through
    this->SpanMinX = extent[0];
    this->SpanMaxX = extent[1];

    // Find the offset to the start position within the YZ array.
    vtkIdType startOffset = 0;

    int yOffset = extent[2] - stencilExtent[2];
    if (yOffset >= 0)
      {
      this->SpanMinY = 0;
      startOffset += yOffset;
      }
    else
      {
      this->SpanMinY = -yOffset;
      }

    if (stencilExtent[3] > extent[3])
      {
      this->SpanMaxY = extent[3] - extent[2];
      }
    else
      {
      this->SpanMaxY = stencilExtent[3] - extent[2];
      }

    int zOffset = extent[4] - stencilExtent[4];
    if (zOffset >= 0)
      {
      this->SpanMinZ = 0;
      startOffset += zOffset*this->SpanSliceIncrement;
      }
    else
      {
      this->SpanMinZ = -zOffset;
      }

    if (stencilExtent[5] > extent[5])
      {
      this->SpanMaxZ = extent[5] - extent[4];
      }
    else
      {
      this->SpanMaxZ = stencilExtent[5] - extent[4];
      }

    if (this->SpanMinY <= this->SpanMaxY &&
        this->SpanMinZ <= this->SpanMaxZ)
      {
      this->SpanCountPointer =
        vtkImageStencilIteratorFriendship::GetExtentListLengths(stencil) +
        startOffset;

      this->SpanListPointer =
        vtkImageStencilIteratorFriendship::GetExtentLists(stencil) +
        startOffset;

      // Holds the current position within the span list for the current row
      this->SetSpanState(this->SpanMinX);
      }
    else
      {
      this->SpanCountPointer = 0;
      this->SpanListPointer = 0;
      this->InStencil = false;
      }
    }
  else
    {
    this->HasStencil = false;
    this->InStencil = true;
    this->SpanSliceEndIncrement = 0;
    this->SpanSliceIncrement = 0;
    this->SpanMinX = 0;
    this->SpanMaxX = 0;
    this->SpanMinY = 0;
    this->SpanMaxY = 0;
    this->SpanMinZ = 0;
    this->SpanMaxZ = 0;
    this->SpanIndexX = 0;
    this->SpanIndexY = 0;
    this->SpanIndexZ = 0;
    this->SpanCountPointer = 0;
    this->SpanListPointer = 0;
    }
}

//----------------------------------------------------------------------------
template <class DType>
vtkImageStencilIterator<DType>::vtkImageStencilIterator(
  vtkImageData *image, vtkImageStencilData *stencil, int extent[6],
  vtkAlgorithm *algorithm, int threadId)
{
  this->Initialize(image, stencil, extent);

  if (algorithm && threadId == 0)
    {
    this->Algorithm = algorithm;
    vtkIdType maxCount = extent[3] - extent[2] + 1;
    maxCount *= extent[5] - extent[4] + 1;
    this->Target = maxCount/50 + 1;
    this->Count = this->Target*50 - (maxCount/this->Target)*this->Target + 1;
    }
  else
    {
    this->Algorithm = 0;
    this->Target = 0;
    this->Count = 0;
    }
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageStencilIterator<DType>::SetSpanState(int idX)
{
  // Find the span that includes idX
  bool inStencil = false;
  int *spans = *this->SpanListPointer;
  int n = *this->SpanCountPointer;
  int i;
  for (i = 0; i < n; i++)
    {
    if (spans[i] > idX)
      {
      break;
      }
    inStencil = !inStencil;
    }

  // Set the primary span state variables
  this->SpanIndexX = i;
  this->InStencil = inStencil;

  // Clamp the span end to SpanMaxX+1
  int endIdX = this->SpanMaxX + 1;
  if (i < n && spans[i] <= this->SpanMaxX)
    {
    endIdX = spans[i];
    }

  // Compute the pointers for idX and endIdX
  DType *rowStartPointer =
    this->RowEndPointer - (this->RowIncrement - this->RowEndIncrement);

  this->Pointer =
    rowStartPointer + (idX - this->SpanMinX)*this->PixelIncrement;

  this->SpanEndPointer =
    rowStartPointer + (endIdX - this->SpanMinX)*this->PixelIncrement;
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageStencilIterator<DType>::NextSpan()
{
  if (this->SpanEndPointer == this->RowEndPointer)
    {
    int spanIncr = 1;

    if (this->SpanEndPointer != this->SliceEndPointer)
      {
      // Move to the next row
      this->Pointer = this->RowEndPointer + this->RowEndIncrement;
      this->RowEndPointer += this->RowIncrement;
      this->SpanEndPointer = this->RowEndPointer;
      this->SpanIndexY++;
      }
    else if (this->SpanEndPointer != this->EndPointer)
      {
      // Move to the next slice
      this->Pointer = this->SliceEndPointer + this->SliceEndIncrement;
      this->SliceEndPointer += this->SliceIncrement;
      this->RowEndPointer = this->Pointer +
        (this->RowIncrement - this->RowEndIncrement);
      this->SpanEndPointer = this->RowEndPointer;
      this->SpanIndexY = 0;
      this->SpanIndexZ++;
      spanIncr += this->SpanSliceEndIncrement;
      }
    else
      {
      // reached EndPointer
      this->Pointer = this->EndPointer;
      return;
      }

    if (this->HasStencil)
      {
      if ((this->SpanIndexY >= this->SpanMinY) &&
          (this->SpanIndexY <= this->SpanMaxY) &&
          (this->SpanIndexZ >= this->SpanMinZ) &&
          (this->SpanIndexZ <= this->SpanMaxZ))
        {
        this->SpanCountPointer += spanIncr;
        this->SpanListPointer += spanIncr;
        this->SetSpanState(this->SpanMinX);
        }
      else
        {
        this->InStencil = false;
        }
      }

    if (this->Algorithm)
      {
      this->ReportProgress();
      }
    }
  else
    {
    // Move to the next span in the current row
    this->Pointer = this->SpanEndPointer;

    // Clamp the span end to SpanMaxX+1
    int endIdX = this->SpanMaxX + 1;
    this->SpanIndexX++;
    if (this->SpanIndexX < *this->SpanCountPointer)
      {
      int tmpIdX = (*this->SpanListPointer)[this->SpanIndexX];
      if (tmpIdX < endIdX)
        {
        endIdX = tmpIdX;
        }
      }

    // Compute the pointer for endIdX
    this->SpanEndPointer = this->RowEndPointer -
      (this->RowIncrement - this->RowEndIncrement) +
      (endIdX - this->SpanMinX)*this->PixelIncrement;

    // Flip the state
    this->InStencil = !this->InStencil;
    }
}

//----------------------------------------------------------------------------
template <class DType>
void vtkImageStencilIterator<DType>::ReportProgress()
{
  if (this->Count % this->Target == 0)
    {
    if (this->Algorithm->GetAbortExecute())
      {
      this->Pointer = this->EndPointer;
      this->SpanEndPointer = this->EndPointer;
      this->RowEndPointer = this->EndPointer;
      this->SliceEndPointer = this->EndPointer;
      }
    else
      {
      this->Algorithm->UpdateProgress(0.02*(this->Count/this->Target));
      }
    }
  this->Count++;
}

#endif
