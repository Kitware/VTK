/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIslandRemoval2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageIslandRemoval2D.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"

vtkStandardNewMacro(vtkImageIslandRemoval2D);

//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageIslandRemoval2D::vtkImageIslandRemoval2D()
{
  this->AreaThreshold = 0;
  this->SetAreaThreshold(4);
  this->SquareNeighborhood = 1;
  this->SquareNeighborhoodOff();
  this->ReplaceValue = 0;
  this->SetReplaceValue(255);
  this->IslandValue = 255;
  this->SetIslandValue(0);
}

//----------------------------------------------------------------------------
void vtkImageIslandRemoval2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "AreaThreshold: " << this->AreaThreshold;
  if (this->SquareNeighborhood)
    {
    os << indent << "Neighborhood: Square";
    }
  else
    {
    os << indent << "Neighborhood: Cross";
    }
  os << indent << "IslandValue: " << this->IslandValue;
  os << indent << "ReplaceValue: " << this->ReplaceValue;
  
}




//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// Codes:  0 => unvisited. 1 => visted don't know. 
//         2 => visted keep.  3 => visited replace.
// Please excuse the length of this function.  The easiest way to choose
// neighborhoods is to check neighbors one by one directly.  Also, I did
// not want to break the templated function into pieces.
template <class T>
void vtkImageIslandRemoval2DExecute(vtkImageIslandRemoval2D *self,
                                    vtkImageData *inData, T *inPtr,
                                    vtkImageData *outData, T *outPtr,
                                    int outExt[6])
{
  int outIdx0, outIdx1, outIdx2;
  vtkIdType outInc0, outInc1, outInc2;
  T *outPtr0, *outPtr1, *outPtr2;
  vtkIdType inInc0, inInc1, inInc2;
  T *inPtr0, *inPtr1, *inPtr2;
  vtkImage2DIslandPixel *pixels;  // All the pixels visited so far.
  int numPixels;      // The number of pixels visited so far.
  vtkImage2DIslandPixel *newPixel;   // The last pixel in the list.
  int nextPixelIdx;   // The index of the next pixel to grow.
  vtkImage2DIslandPixel *nextPixel;  // The next pixel to grow.
  int keepValue;
  int area;
  int squareNeighborhood;
  T islandValue;
  T replaceValue;
  T *inNeighborPtr, *outNeighborPtr;
  int idxC, maxC;
  unsigned long count = 0;
  unsigned long target;

  squareNeighborhood = self->GetSquareNeighborhood();
  area = self->GetAreaThreshold();
  islandValue = static_cast<T>(self->GetIslandValue());
  replaceValue = static_cast<T>(self->GetReplaceValue());
  
  outData->GetIncrements(outInc0, outInc1, outInc2);
  inData->GetIncrements(inInc0, inInc1, inInc2);
  maxC = outData->GetNumberOfScalarComponents();
  
  // Loop through pixels setting all output to 0 (unvisited).
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtr2 = outPtr + idxC;
    for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
      {
      outPtr1 = outPtr2;
      for (outIdx1 = outExt[2]; outIdx1 <= outExt[3]; ++outIdx1)
        {
        outPtr0 = outPtr1;
        for (outIdx0 = outExt[0]; outIdx0 <= outExt[1]; ++outIdx0)
          {
          *outPtr0 = static_cast<T>(0);  // Unvisited
          outPtr0 += outInc0;
          }
        outPtr1 += outInc1;
        }
      outPtr2 += outInc2;
      }
    }

  // update the progress and possibly abort
  self->UpdateProgress(0.1);
  if (self->AbortExecute)
    {
    return;
    }
  
  // In case all 8 neighbors get added before we test the number.
  pixels = new vtkImage2DIslandPixel [area + 8]; 
  
  target = static_cast<unsigned long>(maxC*(outExt[5]-outExt[4]+1)*
                                      (outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Loop though all pixels looking for islands
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtr2 = outPtr + idxC;
    inPtr2 = inPtr + idxC;
    for (outIdx2 = outExt[4]; 
         !self->AbortExecute && outIdx2 <= outExt[5]; ++outIdx2)
      {
      if (!(count%target))
        {
        self->UpdateProgress(0.1 + 0.8*count/(50.0*target));
        }
      count++;
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (outIdx1 = outExt[2]; outIdx1 <= outExt[3]; ++outIdx1)
        {
        outPtr0 = outPtr1;
        inPtr0 = inPtr1;
        for (outIdx0 = outExt[0]; outIdx0 <= outExt[1]; ++outIdx0)
          {
          if (*outPtr0 == 0)
            {
            if (*inPtr0 != islandValue)
              {
              // not an island, keep and go on
              *outPtr0 = 2;
              }
            else
              {
            
              // Start an island search
              // Save first pixel.
              newPixel = pixels;
              newPixel->inPtr = static_cast<void *>(inPtr0);
              newPixel->outPtr = static_cast<void *>(outPtr0);
              newPixel->idx0 = outIdx0;
              newPixel->idx1 = outIdx1;
              numPixels = 1;
              nextPixelIdx = 0;
              nextPixel = pixels;
              *outPtr0 = 1;  // visited don't know
              keepValue = 1;
              // breadth first search
              while (keepValue == 1)  // don't know
                {
                // Check all the neighbors
                // left
                if (nextPixel->idx0 > outExt[0])
                  {
                  inNeighborPtr = static_cast<T *>(nextPixel->inPtr) - inInc0;
                  if ( *inNeighborPtr == islandValue)
                    {
                    outNeighborPtr = static_cast<T *>(nextPixel->outPtr) - outInc0;
                    if ( *outNeighborPtr == 2)
                      {
                      // This is part of a bigger island.
                      keepValue = 2;
                      }
                    if ( *outNeighborPtr == 0)
                      {
                      // New pixel to add
                      ++newPixel;
                      ++numPixels;
                      newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                      newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                      newPixel->idx0 = nextPixel->idx0 - 1;
                      newPixel->idx1 = nextPixel->idx1;
                      *outNeighborPtr = 1;  // visited don't know
                      }
                    }
                  }
                // right
                if (nextPixel->idx0 < outExt[1])
                  {
                  inNeighborPtr = static_cast<T *>(nextPixel->inPtr) + inInc0;
                  if ( *inNeighborPtr == islandValue)
                    {
                    outNeighborPtr = static_cast<T *>(nextPixel->outPtr)
                      + outInc0;
                    if ( *outNeighborPtr == 2)
                      {
                      // This is part of a bigger island.
                      keepValue = 2;
                      }
                    if ( *outNeighborPtr == 0)
                      {
                      // New pixel to add
                      ++newPixel;
                      ++numPixels;
                      newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                      newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                      newPixel->idx0 = nextPixel->idx0 + 1;
                      newPixel->idx1 = nextPixel->idx1;
                      *outNeighborPtr = 1;  // visited don't know
                      }
                    }
                  }
                // up
                if (nextPixel->idx1 > outExt[2])
                  {
                  inNeighborPtr = static_cast<T *>(nextPixel->inPtr) - inInc1;
                  if ( *inNeighborPtr == islandValue)
                    {
                    outNeighborPtr = static_cast<T *>(nextPixel->outPtr) - outInc1;
                    if ( *outNeighborPtr == 2)
                      {
                      // This is part of a bigger island.
                      keepValue = 2;
                      }
                    if ( *outNeighborPtr == 0)
                      {
                      // New pixel to add
                      ++newPixel;
                      ++numPixels;
                      newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                      newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                      newPixel->idx0 = nextPixel->idx0;
                      newPixel->idx1 = nextPixel->idx1 - 1;
                      *outNeighborPtr = 1;  // visited don't know
                      }
                    }
                  }
                // down
                if (nextPixel->idx1 < outExt[3])
                  {
                  inNeighborPtr = static_cast<T *>(nextPixel->inPtr) + inInc1;
                  if ( *inNeighborPtr == islandValue)
                    {
                    outNeighborPtr = static_cast<T *>(nextPixel->outPtr) + outInc1;
                    if ( *outNeighborPtr == 2)
                      {
                      // This is part of a bigger island.
                      keepValue = 2;
                      }
                    if ( *outNeighborPtr == 0)
                      {
                      // New pixel to add
                      ++newPixel;
                      ++numPixels;
                      newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                      newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                      newPixel->idx0 = nextPixel->idx0;
                      newPixel->idx1 = nextPixel->idx1 + 1;
                      *outNeighborPtr = 1;  // visited don't know
                      }
                    }
                  }
                // Corners
                if (squareNeighborhood)
                  {
                  // upper left
                  if (nextPixel->idx0 > outExt[0] && nextPixel->idx1 > outExt[2])
                    {
                    inNeighborPtr = static_cast<T *>(nextPixel->inPtr) - inInc0 - inInc1;
                    if ( *inNeighborPtr == islandValue)
                      {
                      outNeighborPtr = static_cast<T *>(nextPixel->outPtr) - outInc0 -outInc1;
                      if ( *outNeighborPtr == 2)
                        {
                        // This is part of a bigger island.
                        keepValue = 2;
                        }
                      if ( *outNeighborPtr == 0)
                        {
                        // New pixel to add
                        ++newPixel;
                        ++numPixels;
                        newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                        newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                        newPixel->idx0 = nextPixel->idx0 - 1;
                        newPixel->idx1 = nextPixel->idx1 - 1;
                        *outNeighborPtr = 1;  // visited don't know
                        }
                      }
                    }
                  // upper right
                  if (nextPixel->idx0 < outExt[1] && nextPixel->idx1 > outExt[2])
                    {
                    inNeighborPtr = static_cast<T *>(nextPixel->inPtr) + inInc0 - inInc1;
                    if ( *inNeighborPtr == islandValue)
                      {
                      outNeighborPtr = static_cast<T *>(nextPixel->outPtr) + outInc0 -outInc1;
                      if ( *outNeighborPtr == 2)
                        {
                        // This is part of a bigger island.
                        keepValue = 2;
                        }
                      if ( *outNeighborPtr == 0)
                        {
                        // New pixel to add
                        ++newPixel;
                        ++numPixels;
                        newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                        newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                        newPixel->idx0 = nextPixel->idx0 + 1;
                        newPixel->idx1 = nextPixel->idx1 - 1;
                        *outNeighborPtr = 1;  // visited don't know
                        }
                      }
                    }
                  // lower left
                  if (nextPixel->idx0 > outExt[0] && nextPixel->idx1 < outExt[3])
                    {
                    inNeighborPtr = static_cast<T *>(nextPixel->inPtr) - inInc0 + inInc1;
                    if ( *inNeighborPtr == islandValue)
                      {
                      outNeighborPtr = static_cast<T *>(nextPixel->outPtr) - outInc0 +outInc1;
                      if ( *outNeighborPtr == 2)
                        {
                        // This is part of a bigger island.
                        keepValue = 2;
                        }
                      if ( *outNeighborPtr == 0)
                        {
                        // New pixel to add
                        ++newPixel;
                        ++numPixels;
                        newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                        newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                        newPixel->idx0 = nextPixel->idx0 - 1;
                        newPixel->idx1 = nextPixel->idx1 + 1;
                        *outNeighborPtr = 1;  // visited don't know
                        }
                      }
                    }
                  // lower right
                  if (nextPixel->idx0 < outExt[1] && nextPixel->idx1 < outExt[3])
                    {
                    inNeighborPtr = static_cast<T *>(nextPixel->inPtr) + inInc0 + inInc1;
                    if ( *inNeighborPtr == islandValue)
                      {
                      outNeighborPtr = static_cast<T *>(nextPixel->outPtr) + outInc0 +outInc1;
                      if ( *outNeighborPtr == 2)
                        {
                        // This is part of a bigger island.
                        keepValue = 2;
                        }
                      if ( *outNeighborPtr == 0)
                        {
                        // New pixel to add
                        ++newPixel;
                        ++numPixels;
                        newPixel->inPtr = static_cast<void *>(inNeighborPtr);
                        newPixel->outPtr = static_cast<void *>(outNeighborPtr);
                        newPixel->idx0 = nextPixel->idx0 + 1;
                        newPixel->idx1 = nextPixel->idx1 + 1;
                        *outNeighborPtr = 1;  // visited don't know
                        }
                      }
                    }
                  }
              
                // Move to the next pixel to grow.
                ++nextPixel;
                ++nextPixelIdx;
              
                // Have we visted enogh pixels to determine this is a keeper?
                if (keepValue == 1 && numPixels >= area)
                  {
                  keepValue = 2;
                  }
              
                // Have we run out of pixels to grow?
                if (keepValue == 1 && nextPixelIdx >= numPixels)
                  {
                  // The island is too small. Set island values too replace.
                  keepValue = 3;
                  }
                }
            
              // Change "don't knows" to keep value
              nextPixel = pixels;
              for (nextPixelIdx = 0; nextPixelIdx < numPixels; ++nextPixelIdx)
                {
                *(static_cast<T *>(nextPixel->outPtr)) = keepValue;
                ++nextPixel;
                }
              }
            }
        
          outPtr0 += outInc0;
          inPtr0 += inInc0;
          }
        outPtr1 += outInc1;
        inPtr1 += inInc1;
        }
      outPtr2 += outInc2;
      inPtr2 += inInc2;
      }
    }
  
  delete [] pixels;
  
  // update the progress and possibly abort
  self->UpdateProgress(0.9);
  if (self->AbortExecute)
    {
    return;
    }

  // Loop though all pixels actually copying and replacing.
  for (idxC = 0; idxC < maxC; idxC++)
    {
    outPtr2 = outPtr + idxC;
    inPtr2 = inPtr + idxC;
    for (outIdx2 = outExt[4]; outIdx2 <= outExt[5]; ++outIdx2)
      {
      outPtr1 = outPtr2;
      inPtr1 = inPtr2;
      for (outIdx1 = outExt[2]; outIdx1 <= outExt[3]; ++outIdx1)
        {
        outPtr0 = outPtr1;
        inPtr0 = inPtr1;
        for (outIdx0 = outExt[0]; outIdx0 <= outExt[1]; ++outIdx0)
          {
          if (*outPtr0 == 3)
            {
            *outPtr0 = replaceValue;
            }
          else
            {
            *outPtr0 = *inPtr0;
            }
          inPtr0 += inInc0;
          outPtr0 += outInc0;
          }
        inPtr1 += inInc1;
        outPtr1 += outInc1;
        }
      inPtr2 += inInc2;
      outPtr2 += outInc2;
      }
    }
}

    
//----------------------------------------------------------------------------
// This method uses the input data to fill the output data.
// It can handle any type data, but the two datas must have the same 
// data type.  Assumes that in and out have the same lower extent.
int vtkImageIslandRemoval2D::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int outExt[6];
  
  // get the data object
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkImageData *inData = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageData *outData = vtkImageData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int wholeExtent[6];
  int extent[6];
  
  // We need to allocate our own scalars.
  outInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), wholeExtent);
  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  extent[0] = wholeExtent[0];
  extent[1] = wholeExtent[1];
  extent[2] = wholeExtent[2];
  extent[3] = wholeExtent[3];
  outData->SetExtent(extent);
  outData->AllocateScalars();
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " 
                  << vtkImageScalarTypeNameMacro(inData->GetScalarType())
                  << ", must match out ScalarType "
                  << vtkImageScalarTypeNameMacro(outData->GetScalarType()));
    return 1;
    }

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), outExt);
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageIslandRemoval2DExecute(this, inData, 
                                     static_cast<VTK_TT *>(inPtr), outData, 
                                     static_cast<VTK_TT *>(outPtr), outExt));
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return 1;
    }

  return 1;
}







