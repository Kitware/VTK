/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePointDataIterator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImagePointDataIterator.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkDataArray.h"
#include "vtkPointData.h"
#include "vtkAlgorithm.h"

#include <algorithm>

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
vtkImagePointDataIterator::vtkImagePointDataIterator()
{
  this->Id = 0;
  this->SpanEnd = 0;
  this->RowEnd = 0;
  this->SliceEnd = 0;
  this->End = 0;

  this->RowEndIncrement = 0;
  this->RowIncrement = 0;
  this->SliceEndIncrement = 0;
  this->SliceIncrement = 0;

  this->Extent[0] = 0;
  this->Extent[1] = 0;
  this->Extent[2] = 0;
  this->Extent[3] = 0;
  this->Extent[4] = 0;
  this->Extent[5] = 0;

  this->Index[0] = 0;
  this->Index[1] = 0;
  this->Index[2] = 0;
  this->StartY = 0;

  this->HasStencil = false;
  this->InStencil = false;
  this->SpanSliceEndIncrement = 0;
  this->SpanSliceIncrement = 0;
  this->SpanIndex = 0;
  this->SpanCountPointer = 0;
  this->SpanListPointer = 0;

  this->Algorithm = 0;
  this->Count = 0;
  this->Target = 0;
  this->ThreadId = 0;
}

//----------------------------------------------------------------------------
void vtkImagePointDataIterator::Initialize(
  vtkImageData *image, const int extent[6], vtkImageStencilData *stencil,
  vtkAlgorithm *algorithm, int threadId)
{
  const int *dataExtent = image->GetExtent();
  if (extent == 0)
  {
    extent = dataExtent;
  }

  // Save the extent (will be adjusted if there is a stencil).
  bool emptyExtent = false;
  for (int i = 0; i < 6; i += 2)
  {
    this->Extent[i] = std::max(extent[i], dataExtent[i]);
    this->Extent[i+1] = std::min(extent[i+1], dataExtent[i+1]);
    if (this->Extent[i] > this->Extent[i+1])
    {
      emptyExtent = true;
    }
  }

  // Compute the increments for marching through the data.
  this->RowIncrement = dataExtent[1] - dataExtent[0] + 1;
  this->SliceIncrement =
    this->RowIncrement*(dataExtent[3] - dataExtent[2] + 1);

  int rowSpan, sliceSpan, volumeSpan;

  if (!emptyExtent)
  {
    // Compute the span of the image region to be covered.
    rowSpan = this->Extent[1] - this->Extent[0] + 1;
    sliceSpan = this->Extent[3] - this->Extent[2] + 1;
    volumeSpan = this->Extent[5] - this->Extent[4] + 1;
    this->Id = (this->Extent[0] - dataExtent[0]) +
      (this->Extent[2] - dataExtent[2])*this->RowIncrement +
      (this->Extent[4] - dataExtent[4])*this->SliceIncrement;

    // Compute the end increments (continuous increments).
    this->RowEndIncrement = this->RowIncrement - rowSpan;
    this->SliceEndIncrement = this->RowEndIncrement +
      this->SliceIncrement - this->RowIncrement*sliceSpan;
  }
  else
  {
    // Extent is empty, IsAtEnd() will immediately return "true"
    rowSpan = 0;
    sliceSpan = 0;
    volumeSpan = 0;
    this->Id = 0;
    this->RowEndIncrement = 0;
    this->SliceEndIncrement = 0;
    for (int i = 0; i < 6; i += 2)
    {
      this->Extent[i] = dataExtent[i];
      this->Extent[i+1] = dataExtent[i]-1;
    }
  }

  // Get the end pointers for row, slice, and volume.
  this->SpanEnd = this->Id + rowSpan;
  this->RowEnd = this->Id + rowSpan;
  this->SliceEnd = this->Id +
    (this->RowIncrement*sliceSpan - this->RowEndIncrement);
  this->End = this->Id +
    (this->SliceIncrement*volumeSpan - this->SliceEndIncrement);

  // For keeping track of the current x,y,z index.
  this->Index[0] = this->Extent[0];
  this->Index[1] = this->Extent[2];
  this->Index[2] = this->Extent[4];

  // For resetting the Y index after each slice.
  this->StartY = this->Index[1];

  // Code for when a stencil is provided.
  if (stencil)
  {
    this->HasStencil = true;
    this->InStencil = false;

    this->SpanIndex = 0;
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
      int botOffset = this->Extent[2] - stencilExtent[2];
      if (botOffset >= 0)
      {
        this->SpanSliceEndIncrement += botOffset;
      }
      int topOffset = stencilExtent[3] - this->Extent[3];
      if (topOffset >= 0)
      {
        this->SpanSliceEndIncrement += topOffset;
      }
    }

    // Find the offset to the start position within the YZ array.
    vtkIdType startOffset = 0;

    int yOffset = this->Extent[2] - stencilExtent[2];
    if (yOffset < 0)
    {
      this->Extent[2] = stencilExtent[2];
      // starting before start of stencil: subtract the increment that
      // will be added in NextSpan() upon entry into stencil extent
      startOffset -= 1;
    }
    else
    {
      // starting partway into the stencil, so add an offset
      startOffset += yOffset;
    }

    if (stencilExtent[3] <= this->Extent[3])
    {
      this->Extent[3] = stencilExtent[3];
    }

    int zOffset = this->Extent[4] - stencilExtent[4];
    if (zOffset < 0)
    {
      this->Extent[4] = stencilExtent[4];
      // starting before start of stencil: subtract the increment that
      // will be added in NextSpan() upon entry into stencil extent
      if (yOffset >= 0)
      {
        startOffset -= 1 + this->SpanSliceEndIncrement;
      }
    }
    else
    {
      // starting partway into the stencil, so add an offset
      startOffset += zOffset*this->SpanSliceIncrement;
    }

    if (stencilExtent[5] <= this->Extent[5])
    {
      this->Extent[5] = stencilExtent[5];
    }

    if (this->Extent[2] <= this->Extent[3] &&
        this->Extent[4] <= this->Extent[5])
    {
      this->SpanCountPointer =
        vtkImageStencilIteratorFriendship::GetExtentListLengths(stencil) +
        startOffset;

      this->SpanListPointer =
        vtkImageStencilIteratorFriendship::GetExtentLists(stencil) +
        startOffset;

      // Get the current position within the span list for the current row
      if (yOffset >= 0 && zOffset >= 0)
      {
        // If starting within stencil extent, check stencil immediately
        this->InStencil = true;
        this->SetSpanState(this->Extent[0]);
      }
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
    this->SpanIndex = 0;
    this->SpanCountPointer = 0;
    this->SpanListPointer = 0;
  }

  if (algorithm)
  {
    this->Algorithm = algorithm;
    vtkIdType maxCount = sliceSpan;
    maxCount *= volumeSpan;
    this->Target = maxCount/50 + 1;
    this->Count = this->Target*50 - (maxCount/this->Target)*this->Target + 1;
    this->ThreadId = threadId;
  }
  else
  {
    this->Algorithm = 0;
    this->Target = 0;
    this->Count = 0;
    this->ThreadId = 0;
  }
}

//----------------------------------------------------------------------------
void vtkImagePointDataIterator::SetSpanState(int idX)
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
  this->SpanIndex = i;
  this->InStencil = inStencil;

  // Clamp the span end to MaxX+1
  int endIdX = this->Extent[1] + 1;
  if (i < n && spans[i] <= this->Extent[1])
  {
    endIdX = spans[i];
  }

  // Compute the pointers for idX and endIdX
  vtkIdType rowStart =
    this->RowEnd - (this->RowIncrement - this->RowEndIncrement);

  this->Id = rowStart + (idX - this->Extent[0]);
  this->SpanEnd = rowStart + (endIdX - this->Extent[0]);
}

//----------------------------------------------------------------------------
void vtkImagePointDataIterator::NextSpan()
{
  if (this->SpanEnd == this->RowEnd)
  {
    int spanIncr = 1;

    if (this->SpanEnd != this->SliceEnd)
    {
      // Move to the next row
      this->Id = this->RowEnd + this->RowEndIncrement;
      this->RowEnd += this->RowIncrement;
      this->SpanEnd = this->RowEnd;
      this->Index[1]++;
    }
    else if (this->SpanEnd != this->End)
    {
      // Move to the next slice
      this->Id = this->SliceEnd + this->SliceEndIncrement;
      this->SliceEnd += this->SliceIncrement;
      this->RowEnd = this->Id +
        (this->RowIncrement - this->RowEndIncrement);
      this->SpanEnd = this->RowEnd;
      this->Index[1] = this->StartY;
      this->Index[2]++;
      spanIncr += this->SpanSliceEndIncrement;
    }
    else
    {
      // reached End
      this->Id = this->End;
      return;
    }

    // Start of next row
    this->Index[0] = this->Extent[0];

    if (this->HasStencil)
    {
      if ((this->Index[1] >= this->Extent[2]) &&
          (this->Index[1] <= this->Extent[3]) &&
          (this->Index[2] >= this->Extent[4]) &&
          (this->Index[2] <= this->Extent[5]))
      {
        this->SpanCountPointer += spanIncr;
        this->SpanListPointer += spanIncr;
        this->SetSpanState(this->Extent[0]);
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
    this->Id = this->SpanEnd;
    int spanCount = *this->SpanCountPointer;
    int endIdX = this->Extent[1] + 1;
    this->Index[0] = endIdX;
    if (this->SpanIndex < spanCount)
    {
      int tmpIdX = (*this->SpanListPointer)[this->SpanIndex];
      if (tmpIdX < endIdX)
      {
        this->Index[0] = tmpIdX;
      }
    }

    // Get the index to the start of the span after the next
    this->SpanIndex++;
    if (this->SpanIndex < spanCount)
    {
      int tmpIdX = (*this->SpanListPointer)[this->SpanIndex];
      if (tmpIdX < endIdX)
      {
        endIdX = tmpIdX;
      }
    }

    // Compute the end of the span
    this->SpanEnd = this->RowEnd -
      (this->RowIncrement - this->RowEndIncrement) +
      (endIdX - this->Extent[0]);

    // Flip the state
    this->InStencil = !this->InStencil;
  }
}

//----------------------------------------------------------------------------
void *vtkImagePointDataIterator::GetVoidPointer(
  vtkDataArray *array, vtkIdType i, int *pixelIncrement)
{
  int n = array->GetNumberOfComponents();
  if (pixelIncrement)
  {
    *pixelIncrement = n;
  }
  return array->GetVoidPointer(i*n);
}

//----------------------------------------------------------------------------
void *vtkImagePointDataIterator::GetVoidPointer(
  vtkImageData *image, vtkIdType i, int *pixelIncrement)
{
  return vtkImagePointDataIterator::GetVoidPointer(
    image->GetPointData()->GetScalars(), i, pixelIncrement);
}

//----------------------------------------------------------------------------
void vtkImagePointDataIterator::ReportProgress()
{
  if (this->Count % this->Target == 0)
  {
    if (this->Algorithm->GetAbortExecute())
    {
      this->Id = this->End;
      this->SpanEnd = this->End;
      this->RowEnd = this->End;
      this->SliceEnd = this->End;
    }
    else if (this->ThreadId == 0)
    {
      this->Algorithm->UpdateProgress(0.02*(this->Count/this->Target));
    }
  }
  this->Count++;
}
