/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageDistance1D.cxx
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
#include "vtkImageRegion.h"
#include "vtkImageCache.h"
#include "vtkImageDistance1D.h"

//----------------------------------------------------------------------------
vtkImageDistance1D::vtkImageDistance1D()
{
  this->SetOutputScalarType(VTK_SHORT);
  
  this->NumberOfExecutionAxes = 1;
}


//----------------------------------------------------------------------------
void vtkImageDistance1D::SetFilteredAxis(int axis)
{
  this->SetFilteredAxes(1, &axis);
}
  
  
//----------------------------------------------------------------------------
// Description:
// Intercepts the caches Update to make the region larger than requested.
// Create the whole output array.
void vtkImageDistance1D::InterceptCacheUpdate()
{
  int min, max;
  
  if ( ! this->Input)
    {
    vtkErrorMacro(<< "Input not set.");
    return;
    }

  this->Output->GetAxisWholeExtent(this->FilteredAxes[0], min, max);
  this->Output->SetAxisUpdateExtent(this->FilteredAxes[0], min, max);
}

//----------------------------------------------------------------------------
// Description:
// This method tells the superclass that the whole input array is needed
// to compute any output region.
void vtkImageDistance1D::ComputeRequiredInputUpdateExtent()
{
  int min, max;

  this->Input->GetAxisWholeExtent(this->FilteredAxes[0], min, max);
  this->Input->SetAxisUpdateExtent(this->FilteredAxes[0], min, max);
}


//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the
// distance algorithm.
void vtkImageDistance1D::Execute(vtkImageRegion *inRegion, 
					 vtkImageRegion *outRegion)
{
  short *inPtr, *outPtr;
  int inInc, outInc;
  int min, max;
  int idx;
  short distP, distN;
  short big = 2000;

  vtkDebugMacro(<< "Execute: inRegion = " << inRegion 
		<< ", outRegion = " << outRegion);
  
  // this filter expects that inputand output are short
  if (inRegion->GetScalarType() != VTK_SHORT ||
      outRegion->GetScalarType() != VTK_SHORT)
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inRegion->GetScalarType()
                  << ", and out ScalarType " << outRegion->GetScalarType()
                  << " must be short.");
    return;
    }


  outRegion->GetExtent(min, max);
  outRegion->GetIncrements(outInc);
  inRegion->GetIncrements(inInc);
  
  // Forward pass
  distP = big;
  distN = -big;
  inPtr = (short *)(inRegion->GetScalarPointer());
  outPtr = (short *)(outRegion->GetScalarPointer());
  for (idx = min; idx <= max; ++idx)
    {
    if (*inPtr >= 0)
      {
      distN = 0;
      if (distP > *inPtr)
	{
	distP = *inPtr;
	}
      *outPtr = distP;
      }
    if (*inPtr <= 0)
      {
      distP = 0;
      if (distN < *inPtr)
	{
	distN = *inPtr;
	}
      *outPtr = distN;
      }
    
    if (distP < big)
      {
      ++distP;
      }
    if (distN > -big)
      {
      --distN;
      }
    
    inPtr += inInc;
    outPtr += outInc;
    }

  // backward pass
  distP = big;
  distN = -big;
  outPtr -= outInc;  // Undo the last increment to put us at the last pixel
  for (idx = max; idx >= min; --idx)
    {
    if (*outPtr >= 0)
      {
      if (distP > *outPtr)
	{
	distP = *outPtr;
	}
      *outPtr = distP;
      }
    if (*outPtr <= 0)
      {
      if (distN < *outPtr)
	{
	distN = *outPtr;
	}
      *outPtr = distN;
      }
    
    if (distP < big)
      {
      ++distP;
      }
    if (distN > -big)
      {
      --distN;
      }
    
    outPtr -= outInc;
    }
}



