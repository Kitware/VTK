/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageIslandRemoval2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkImageIslandRemoval2D.h"
#include "vtkImageCache.h"







//----------------------------------------------------------------------------
// Description:
// Constructor: Sets default filter to be identity.
vtkImageIslandRemoval2D::vtkImageIslandRemoval2D()
{
  this->SetAxes(VTK_IMAGE_X_AXIS, VTK_IMAGE_Y_AXIS);
  this->SetAreaThreshold(4);
  this->SquareNeighborhoodOff();
  this->SetReplaceValue(255);
  this->SetIslandValue(0);

  this->ExecuteDimensionality = 2;
  this->Dimensionality = 2;
}

//----------------------------------------------------------------------------
void vtkImageIslandRemoval2D::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);
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
// Description:
// Intercepts the caches UpdateRegion to make the region larger than requested.
// The whole image is generated when any region is requested.
void vtkImageIslandRemoval2D::InterceptCacheUpdate(vtkImageRegion *region)
{
  int extent[4];

  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }
  
  this->Input->UpdateImageInformation(region);
  region->GetImageExtent(2, extent);
  region->SetExtent(2, extent);
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
					  vtkImageRegion *inRegion, T *inPtr,
					  vtkImageRegion *outRegion, T *outPtr)
{
  int outIdx0, outIdx1;
  int outMin0, outMax0, outMin1, outMax1;
  int outInc0, outInc1;
  T *outPtr0, *outPtr1;
  int inInc0, inInc1;
  T *inPtr0, *inPtr1;
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
  
  
  
  squareNeighborhood = self->GetSquareNeighborhood();
  area = self->GetAreaThreshold();
  islandValue = (T)(self->GetIslandValue());
  replaceValue = (T)(self->GetReplaceValue());
  numPixels = 0;
  // In case all 8 neighbors get added before we test the number.
  pixels = new vtkImage2DIslandPixel [area + 8]; 
  
  outRegion->GetExtent(outMin0, outMax0, outMin1, outMax1);
  outRegion->GetIncrements(outInc0, outInc1);
  inRegion->GetIncrements(inInc0, inInc1);

  // Loop through pixels setting all output to 0 (unvisited).
  outPtr1 = outPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
      {
      *outPtr0 = (T)(0);  // Unvisited
      outPtr0 += outInc0;
      }
    outPtr1 += outInc1;
    }
    
  // Loop though all pixels looking for islands
  outPtr1 = outPtr;
  inPtr1 = inPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
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
	  newPixel->inPtr = (void *)(inPtr0);
	  newPixel->outPtr = (void *)(outPtr0);
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
	    if (nextPixel->idx0 > outMin0)
	      {
	      inNeighborPtr = (T *)(nextPixel->inPtr) - inInc0;
	      if ( *inNeighborPtr == islandValue)
		{
		outNeighborPtr = (T *)(nextPixel->outPtr) - outInc0;
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
		  newPixel->inPtr = (void *)(inNeighborPtr);
		  newPixel->outPtr = (void *)(outNeighborPtr);
		  newPixel->idx0 = nextPixel->idx0 - 1;
		  newPixel->idx1 = nextPixel->idx1;
		  *outNeighborPtr = 1;  // visited don't know
		  }
		}
	      }
	    // right
	    if (nextPixel->idx0 < outMax0)
	      {
	      inNeighborPtr = (T *)(nextPixel->inPtr) + inInc0;
	      if ( *inNeighborPtr == islandValue)
		{
		outNeighborPtr = (T *)(nextPixel->outPtr) + outInc0;
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
		  newPixel->inPtr = (void *)(inNeighborPtr);
		  newPixel->outPtr = (void *)(outNeighborPtr);
		  newPixel->idx0 = nextPixel->idx0 + 1;
		  newPixel->idx1 = nextPixel->idx1;
		  *outNeighborPtr = 1;  // visited don't know
		  }
		}
	      }
	    // up
	    if (nextPixel->idx1 > outMin1)
	      {
	      inNeighborPtr = (T *)(nextPixel->inPtr) - inInc1;
	      if ( *inNeighborPtr == islandValue)
		{
		outNeighborPtr = (T *)(nextPixel->outPtr) - outInc1;
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
		  newPixel->inPtr = (void *)(inNeighborPtr);
		  newPixel->outPtr = (void *)(outNeighborPtr);
		  newPixel->idx0 = nextPixel->idx0;
		  newPixel->idx1 = nextPixel->idx1 - 1;
		  *outNeighborPtr = 1;  // visited don't know
		  }
		}
	      }
	    // down
	    if (nextPixel->idx1 < outMax1)
	      {
	      inNeighborPtr = (T *)(nextPixel->inPtr) + inInc1;
	      if ( *inNeighborPtr == islandValue)
		{
		outNeighborPtr = (T *)(nextPixel->outPtr) + outInc1;
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
		  newPixel->inPtr = (void *)(inNeighborPtr);
		  newPixel->outPtr = (void *)(outNeighborPtr);
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
	      if (nextPixel->idx0 > outMin0 && nextPixel->idx1 > outMin1)
		{
		inNeighborPtr = (T *)(nextPixel->inPtr) - inInc0 - inInc1;
		if ( *inNeighborPtr == islandValue)
		  {
		  outNeighborPtr = (T *)(nextPixel->outPtr) - outInc0 -outInc1;
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
		    newPixel->inPtr = (void *)(inNeighborPtr);
		    newPixel->outPtr = (void *)(outNeighborPtr);
		    newPixel->idx0 = nextPixel->idx0 - 1;
		    newPixel->idx1 = nextPixel->idx1 - 1;
		    *outNeighborPtr = 1;  // visited don't know
		    }
		  }
		}
	      // upper right
	      if (nextPixel->idx0 < outMax0 && nextPixel->idx1 > outMin1)
		{
		inNeighborPtr = (T *)(nextPixel->inPtr) + inInc0 - inInc1;
		if ( *inNeighborPtr == islandValue)
		  {
		  outNeighborPtr = (T *)(nextPixel->outPtr) + outInc0 -outInc1;
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
		    newPixel->inPtr = (void *)(inNeighborPtr);
		    newPixel->outPtr = (void *)(outNeighborPtr);
		    newPixel->idx0 = nextPixel->idx0 + 1;
		    newPixel->idx1 = nextPixel->idx1 - 1;
		    *outNeighborPtr = 1;  // visited don't know
		    }
		  }
		}
	      // lower left
	      if (nextPixel->idx0 > outMin0 && nextPixel->idx1 < outMax1)
		{
		inNeighborPtr = (T *)(nextPixel->inPtr) - inInc0 + inInc1;
		if ( *inNeighborPtr == islandValue)
		  {
		  outNeighborPtr = (T *)(nextPixel->outPtr) - outInc0 +outInc1;
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
		    newPixel->inPtr = (void *)(inNeighborPtr);
		    newPixel->outPtr = (void *)(outNeighborPtr);
		    newPixel->idx0 = nextPixel->idx0 - 1;
		    newPixel->idx1 = nextPixel->idx1 + 1;
		    *outNeighborPtr = 1;  // visited don't know
		    }
		  }
		}
	      // lower right
	      if (nextPixel->idx0 < outMax0 && nextPixel->idx1 < outMax1)
		{
		inNeighborPtr = (T *)(nextPixel->inPtr) + inInc0 + inInc1;
		if ( *inNeighborPtr == islandValue)
		  {
		  outNeighborPtr = (T *)(nextPixel->outPtr) + outInc0 +outInc1;
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
		    newPixel->inPtr = (void *)(inNeighborPtr);
		    newPixel->outPtr = (void *)(outNeighborPtr);
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
	  
	  // Change don't knows to keep value
	  nextPixel = pixels;
	  for (nextPixelIdx = 0; nextPixelIdx < numPixels; ++nextPixelIdx)
	    {
	    *((T *)(nextPixel->outPtr)) = keepValue;
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

  delete [] pixels;
  
  
  
  
  // Loop though all pixels actually copying and replacing.
  outPtr1 = outPtr;
  inPtr1 = inPtr;
  for (outIdx1 = outMin1; outIdx1 <= outMax1; ++outIdx1)
    {
    outPtr0 = outPtr1;
    inPtr0 = inPtr1;
    for (outIdx0 = outMin0; outIdx0 <= outMax0; ++outIdx0)
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
}

    
//----------------------------------------------------------------------------
// Description:
// This method uses the input region to fill the output region.
// It can handle any type data, but the two regions must have the same 
// data type.  Assumes that in and out have the same lower extent.
void vtkImageIslandRemoval2D::Execute(vtkImageRegion *inRegion, 
					      vtkImageRegion *outRegion)
{
  void *inPtr, *outPtr;
  
  
  // this filter expects that input is the same type as output.
  if (inRegion->GetScalarType() != outRegion->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " 
                  << vtkImageScalarTypeNameMacro(inRegion->GetScalarType())
                  << ", must match out ScalarType "
                  << vtkImageScalarTypeNameMacro(outRegion->GetScalarType()));
    return;
    }

  inPtr = inRegion->GetScalarPointer();
  outPtr = outRegion->GetScalarPointer();

  switch (inRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageIslandRemoval2DExecute(this, 
			   inRegion, (float *)(inPtr), 
			   outRegion, (float *)(outPtr));
      break;
    case VTK_INT:
      vtkImageIslandRemoval2DExecute(this, 
			   inRegion, (int *)(inPtr), 
			   outRegion, (int *)(outPtr));
      break;
    case VTK_SHORT:
      vtkImageIslandRemoval2DExecute(this, 
			   inRegion, (short *)(inPtr), 
			   outRegion, (short *)(outPtr));
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageIslandRemoval2DExecute(this, 
			   inRegion, (unsigned short *)(inPtr), 
			   outRegion, (unsigned short *)(outPtr));
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageIslandRemoval2DExecute(this, 
			   inRegion, (unsigned char *)(inPtr), 
			   outRegion, (unsigned char *)(outPtr));
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }  
}
















