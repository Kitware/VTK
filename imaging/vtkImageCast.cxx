/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCast.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Abdalmajeid M. Alyassin who developed this class.

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
#include "vtkImageCache.h"
#include "vtkImageCast.h"



//----------------------------------------------------------------------------
vtkImageCast::vtkImageCast()
{
  this->OutputScalarType = VTK_FLOAT;
  this->ClampOverflow = 0;
}


//----------------------------------------------------------------------------
// Just change the Image extent.
void vtkImageCast::ExecuteImageInformation()
{
  this->Output->SetScalarType(this->OutputScalarType);
}

//----------------------------------------------------------------------------
// Description:
// The update method first checks to see is a cast is necessary.
void vtkImageCast::InternalUpdate(vtkImageData *data)
{
  
  if (! this->Input || ! this->Output)
    {
    vtkErrorMacro("Update: Input or output is not set.");
    return;
    }
  
  // Do the scalar types already match
  if (this->Input->GetScalarType() == this->Output->GetScalarType())
    {
    int bypassSave = this->Bypass;
    // just copy by reference. (use Bypass)
    vtkDebugMacro("Update: Cast is not necessary.");
    this->Bypass = 1;
    this->vtkImageFilter::InternalUpdate(data);
    this->Bypass = bypassSave;
    return;
    }
  
  // call the superclass update which will cause an execute.
  this->vtkImageFilter::InternalUpdate(data);
}

//----------------------------------------------------------------------------
// Description:
// This templated function executes the filter for any type of data.
template <class IT, class OT>
static void vtkImageCastExecute(vtkImageCast *self,
				vtkImageData *inData, IT *inPtr,
				vtkImageData *outData, OT *outPtr,
				int outExt[6], int id)
{
  float typeMin, typeMax, val;
  int clamp;
  int idxR, idxY, idxZ;
  int maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int rowLength;
  unsigned long count = 0;
  unsigned long target;

  // for preventing overflow
  typeMin = outData->GetScalarTypeMin();
  typeMax = outData->GetScalarTypeMax();
  clamp = self->GetClampOverflow();

  // find the region to loop over
  rowLength = (outExt[1] - outExt[0]+1)*inData->GetNumberOfScalarComponents();
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(outExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target)) self->UpdateProgress(count/(50.0*target));
	count++;
	}
      // put the test for clamp to avoid the innermost loop
      if (clamp)
	{
	for (idxR = 0; idxR < rowLength; idxR++)
	  {
	  // Pixel operation
	  val = (float)(*inPtr);
	  if (val > typeMax)
	    {
	    val = typeMax;
	    }
	  if (val < typeMin)
	    {
	    val = typeMin;
	    }
	  *outPtr = (OT)(val);
	  outPtr++;
	  inPtr++;
	  }
	}
      else
	{
	for (idxR = 0; idxR < rowLength; idxR++)
	  {
	  // Pixel operation
	  *outPtr = (OT)(*inPtr);
	  outPtr++;
	  inPtr++;
	  }
	}
      outPtr += outIncY;
      inPtr += inIncY;
      }
    outPtr += outIncZ;
    inPtr += inIncZ;
    }
}



//----------------------------------------------------------------------------
template <class T>
static void vtkImageCastExecute(vtkImageCast *self,
				vtkImageData *inData, T *inPtr,
				vtkImageData *outData, int outExt[6], int id)
{
  void *outPtr = outData->GetScalarPointerForExtent(outExt);

  switch (outData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (float *)(outPtr),outExt, id);
      break;
    case VTK_INT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (int *)(outPtr),outExt, id); 
      break;
    case VTK_SHORT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (short *)(outPtr),outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned short *)(outPtr),outExt, id); 
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCastExecute(self, 
			  inData, (T *)(inPtr), 
			  outData, (unsigned char *)(outPtr),outExt, id); 
      break;
    default:
      vtkGenericWarningMacro("Execute: Unknown output ScalarType");
      return;
    }
}




//----------------------------------------------------------------------------
// Description:
// This method is passed a input and output region, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageCast::ThreadedExecute(vtkImageData *inData, 
				   vtkImageData *outData,
				   int outExt[6], int id)
{
  void *inPtr = inData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
		<< ", outData = " << outData);
  
  switch (inData->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImageCastExecute(this, inData, (float *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_INT:
      vtkImageCastExecute(this, inData, (int *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_SHORT:
      vtkImageCastExecute(this, inData, (short *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImageCastExecute(this, inData, (unsigned short *)(inPtr), 
			  outData, outExt, id);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImageCastExecute(this, inData, (unsigned char *)(inPtr), 
			  outData, outExt, id);
      break;
    default:
      vtkErrorMacro(<< "Execute: Unknown input ScalarType");
      return;
    }
}

void vtkImageCast::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageFilter::PrintSelf(os,indent);

  os << indent << "OutputScalarType: " << this->OutputScalarType << "\n";
  os << indent << "ClampOverflow: ";
  if (this->ClampOverflow)
    {
    os << "On\n";
    }
  else 
    {
    os << "Off\n";
    }
}

