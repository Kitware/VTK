/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFeatureAnd.cxx
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
#include "vtkImageFeatureAnd.h"


//----------------------------------------------------------------------------
vtkImageFeatureAnd::vtkImageFeatureAnd()
{
  this->OutputConnectedValue = 255;
  this->OutputUnconnectedValue = 0;
  this->Connector = vtkImageConnector::New();
}

//----------------------------------------------------------------------------
vtkImageFeatureAnd::~vtkImageFeatureAnd()
{
  this->Connector->Delete();
}

//----------------------------------------------------------------------------
void vtkImageFeatureAnd::SetFilteredAxes(int num, int *axes)
{
  if (num > 3)
    {
    vtkWarningMacro("SetFilteredAxes: Only handle up to three axes");
    num = 3;
   }
  this->vtkImageTwoInputFilter::SetFilteredAxes(num, axes);
}


//----------------------------------------------------------------------------
// Update the whole image in cache because we will be generating the whole
// image anyway.
void vtkImageFeatureAnd::InterceptCacheUpdate(vtkImageCache *out)
{
  int idx, axis;
  int min, max;

  for (idx = 0; idx < this->NumberOfFilteredAxes; ++idx)
    {
    axis = this->FilteredAxes[idx];
    out->GetAxisWholeExtent(axis, min, max);
    out->SetAxisUpdateExtent(axis, min, max);
    }    
}


//----------------------------------------------------------------------------
void vtkImageFeatureAnd::Execute(vtkImageRegion *in1Region,
				 vtkImageRegion *in2Region,
				 vtkImageRegion *outRegion)
{
  int index[3];
  int idx0, idx1, idx2;
  int in1Inc0, in1Inc1, in1Inc2;
  int in2Inc0, in2Inc1, in2Inc2;
  int outInc0, outInc1, outInc2;
  int min0, max0, min1, max1, min2, max2;
  unsigned char *in1Ptr0, *in1Ptr1, *in1Ptr2;
  unsigned char *in2Ptr0, *in2Ptr1, *in2Ptr2;
  unsigned char *outPtr0, *outPtr1, *outPtr2;
  unsigned char intermediateValue;

  if (in1Region->GetScalarType() != VTK_UNSIGNED_CHAR ||
       in2Region->GetScalarType() != VTK_UNSIGNED_CHAR ||
       outRegion->GetScalarType() != VTK_UNSIGNED_CHAR)
    {
    vtkErrorMacro("Execute: Both inputs and output must have scalar type UnsignedChar");
    return;
    }

  // Pick an intermediate value (In some cases, we could eliminate the last threshold.)
  intermediateValue = 1;

  //-----
  // threshold to find seeds, assume extent of inputs are the same,
  in1Region->GetIncrements(in1Inc0, in1Inc1, in1Inc2);
  in2Region->GetIncrements(in2Inc0, in2Inc1, in2Inc2);
  in1Region->GetExtent(min0, max0, min1, max1, min2, max2);
  outRegion->GetIncrements(outInc0, outInc1, outInc2);
  in1Ptr2 = (unsigned char *)(in1Region->GetScalarPointer());
  in2Ptr2 = (unsigned char *)(in2Region->GetScalarPointer());
  outPtr2 = (unsigned char *)(outRegion->GetScalarPointer());
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    index[2] = idx2;
    in1Ptr1 = in1Ptr2;
    in2Ptr1 = in2Ptr2;
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      index[1] = idx1;
      in1Ptr0 = in1Ptr1;
      in2Ptr0 = in2Ptr1;
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
	index[0] = idx0;
	*outPtr0 = 0;
        if (*in1Ptr0)
          {
          *outPtr0 = intermediateValue;
	  if (*in2Ptr0)
	    {
	    // might as well set this to connected value here.
	    *outPtr0 = this->OutputConnectedValue;
	    // make a seed
	    this->Connector->AddSeed(this->Connector->NewSeed(index, outPtr0));
	    }
          }
        in1Ptr0 += in1Inc0;
        in2Ptr0 += in2Inc0;
        outPtr0 += outInc0;
        }
      in1Ptr1 += in1Inc1;
      in2Ptr1 += in2Inc1;
      outPtr1 += outInc1;
      }
    in1Ptr2 += in1Inc2;
    in2Ptr2 += in2Inc2;
    outPtr2 += outInc2;
    }
  
  //-----
  // connect
  this->Connector->SetUnconnectedValue(intermediateValue);
  this->Connector->SetConnectedValue(this->OutputConnectedValue);
  this->Connector->MarkRegion(outRegion, this->NumberOfFilteredAxes);

  //-----
  // Threshold to convert intermediate values into OutputUnconnectedValues
  outPtr2 = (unsigned char *)(outRegion->GetScalarPointer());
  for (idx2 = min2; idx2 <= max2; ++idx2)
    {
    outPtr1 = outPtr2;
    for (idx1 = min1; idx1 <= max1; ++idx1)
      {
      outPtr0 = outPtr1;
      for (idx0 = min0; idx0 <= max0; ++idx0)
        {
        if (*outPtr0 == intermediateValue)
          {
          *outPtr0 = this->OutputUnconnectedValue;
          }
        outPtr0 += outInc0;
        }
      outPtr1 += outInc1;
      }
     outPtr2 += outInc2;
    }
}




