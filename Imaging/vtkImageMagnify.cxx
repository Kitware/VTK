/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageMagnify.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkImageMagnify.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageMagnify* vtkImageMagnify::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageMagnify");
  if(ret)
    {
    return (vtkImageMagnify*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageMagnify;
}






//----------------------------------------------------------------------------
// Constructor: Sets default filter to be identity.
vtkImageMagnify::vtkImageMagnify()
{
  int idx;
  
  this->Interpolate = 0;
  for (idx = 0; idx < 3; ++idx)
    {
    this->MagnificationFactors[idx] = 1;
    }
}


//----------------------------------------------------------------------------
// Computes any global image information associated with regions.
void vtkImageMagnify::ExecuteInformation(vtkImageData *inData, 
					 vtkImageData *outData)
{
  float *spacing;
  int idx;
  int *inExt;
  float outSpacing[3];
  int outExt[6];
  
  inExt = inData->GetWholeExtent();
  spacing = inData->GetSpacing();
  for (idx = 0; idx < 3; idx++)
    {
    // Scale the output extent
    outExt[idx*2] = inExt[idx*2] * this->MagnificationFactors[idx];
    outExt[idx*2+1] = outExt[idx*2] + 
      (inExt[idx*2+1] - inExt[idx*2] + 1)*this->MagnificationFactors[idx] - 1;
    
    // Change the data spacing
    outSpacing[idx] = spacing[idx] / (float)(this->MagnificationFactors[idx]);
    }
  
  outData->SetWholeExtent(outExt);
  outData->SetSpacing(outSpacing);

}

//----------------------------------------------------------------------------
// This method computes the Region of input necessary to generate outRegion.
// It assumes offset and size are multiples of Magnify Factors.
void vtkImageMagnify::ComputeInputUpdateExtent(int inExt[6],
					       int outExt[6])
{
  int idx;
  
  for (idx = 0; idx < 3; idx++)
    {
    // For Min. Round Down
    inExt[idx*2] = (int)(floor((float)(outExt[idx*2]) / 
			       (float)(this->MagnificationFactors[idx])));
    inExt[idx*2+1] = (int)(floor((float)(outExt[idx*2+1]) / 
				 (float)(this->MagnificationFactors[idx])));
    }
}




//----------------------------------------------------------------------------
// The templated execute function handles all the data types.
// 2d even though operation is 1d.
// Note: Slight misalignment (pixel replication is not nearest neighbor).
template <class T>
static void vtkImageMagnifyExecute(vtkImageMagnify *self,
				  vtkImageData *inData, T *inPtr, int inExt[6],
				  vtkImageData *outData, T *outPtr,
				  int outExt[6], int id)
{
  int idxC, idxX, idxY, idxZ;
  int inIdxX, inIdxY, inIdxZ;
  int inMaxX, inMaxY, inMaxZ;
  int maxC, maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  int interpolate;
  int magXIdx, magX;
  int magYIdx, magY;
  int magZIdx, magZ;
  T *inPtrZ, *inPtrY, *inPtrX, *outPtrC;
  float iMag, iMagP = 0.0, iMagPY = 0.0, iMagPZ = 0.0, iMagPYZ = 0.0;
  T dataP = 0, dataPX = 0, dataPY = 0, dataPZ = 0;
  T dataPXY = 0, dataPXZ = 0, dataPYZ = 0, dataPXYZ = 0;
  int interpSetup;
  
  interpolate = self->GetInterpolate();
  magX = self->GetMagnificationFactors()[0];
  magY = self->GetMagnificationFactors()[1];
  magZ = self->GetMagnificationFactors()[2];
  iMag = 1.0/(magX*magY*magZ);
  
  // find the region to loop over
  maxC = outData->GetNumberOfScalarComponents();
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  target = (unsigned long)(maxC*(maxZ+1)*(maxY+1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetIncrements(inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);

  // Now I am putting in my own boundary check because of ABRs and FMRs
  // And I do not understand (nor do I care to figure out) what
  // Ken is doing with his checks. (Charles)
  inMaxX = inExt[1];
  inMaxY = inExt[3];
  inMaxZ = inExt[5];
  inData->GetExtent(idxC, inMaxX, idxC, inMaxY, idxC, inMaxZ);
  
  // Loop through ouput pixels
  for (idxC = 0; idxC < maxC; idxC++)
    {
    inPtrZ = inPtr + idxC;
    inIdxZ = inExt[4];
    outPtrC = outPtr + idxC;
    magZIdx = magZ - outExt[4]%magZ - 1;
    for (idxZ = 0; idxZ <= maxZ; idxZ++, magZIdx--)
      {
      inPtrY = inPtrZ;
      inIdxY = inExt[2];
      magYIdx = magY - outExt[2]%magY - 1;
      for (idxY = 0; !self->AbortExecute && idxY <= maxY; idxY++, magYIdx--)
	{
	if (!id) 
	  {
	  if (!(count%target))
	    {
	    self->UpdateProgress(count/(50.0*target));
	    }
	  count++;
	  }
	
	if (interpolate)
	  {
	  // precompute some values for interpolation
	  iMagP = (magYIdx + 1)*(magZIdx + 1)*iMag;
	  iMagPY = (magY - magYIdx - 1)*(magZIdx + 1)*iMag;
	  iMagPZ = (magYIdx + 1)*(magZ - magZIdx - 1)*iMag;
	  iMagPYZ = (magY - magYIdx - 1)*(magZ - magZIdx - 1)*iMag;
	  }
	
	magXIdx = magX - outExt[0]%magX - 1;
	inPtrX = inPtrY;
	inIdxX = inExt[0];
	interpSetup = 0;
	for (idxX = 0; idxX <= maxX; idxX++, magXIdx--)
	  {
	  // Pixel operation
	  if (!interpolate)
	    {
	    *outPtrC = *inPtrX;
	    }
	  else
	    {
	    // setup data values for interp, overload dataP as an 
	    // indicator of if this has been done yet
	    if (!interpSetup) 
	      {
	      int tiX, tiY, tiZ;
	      
	      dataP = *inPtrX;

	      // Now I am putting in my own boundary check because of 
	      // ABRs and FMRs
	      // And I do not understand (nor do I care to figure out) what
	      // Ken was doing with his checks. (Charles)
	      if (inIdxX < inMaxX) 
		{
		tiX = inIncX;
		}
	      else
		{
		tiX = 0;
		}
	      if (inIdxY < inMaxY) 
		{
		tiY = inIncY;
		}
	      else
		{
		tiY = 0;
		}
	      if (inIdxZ < inMaxZ)
		{
		tiZ = inIncZ;
		}
	      else
		{
		tiZ = 0;
		}
	      dataPX = *(inPtrX + tiX); 
	      dataPY = *(inPtrX + tiY); 
	      dataPZ = *(inPtrX + tiZ);
	      dataPXY = *(inPtrX + tiX + tiY); 
	      dataPXZ = *(inPtrX + tiX + tiZ); 
	      dataPYZ = *(inPtrX + tiY + tiZ); 
	      dataPXYZ = *(inPtrX + tiX + tiY + tiZ); 
	      interpSetup = 1;
	      }
	    *outPtrC = (T)
	      ((float)dataP*(magXIdx + 1)*iMagP + 
	       (float)dataPX*(magX - magXIdx - 1)*iMagP +
	       (float)dataPY*(magXIdx + 1)*iMagPY + 
	       (float)dataPXY*(magX - magXIdx - 1)*iMagPY +
	       (float)dataPZ*(magXIdx + 1)*iMagPZ + 
	       (float)dataPXZ*(magX - magXIdx - 1)*iMagPZ +
	       (float)dataPYZ*(magXIdx + 1)*iMagPYZ + 
	       (float)dataPXYZ*(magX - magXIdx - 1)*iMagPYZ);
	    }
	  outPtrC += maxC;
	  if (!magXIdx) 
	    {
	    inPtrX += inIncX;
	    ++inIdxX;
	    magXIdx = magX;
	    interpSetup = 0;
	    }
	  }
	outPtrC += outIncY;
	if (!magYIdx) 
	  {
	  inPtrY += inIncY;
	  ++inIdxY;
	  magYIdx = magY;
	  }
	}
      outPtrC += outIncZ;
      if (!magZIdx) 
	{
	inPtrZ += inIncZ;
	++inIdxZ;
	magZIdx = magZ;
	}
      }
    }
}

void vtkImageMagnify::ThreadedExecute(vtkImageData *inData, 
				      vtkImageData *outData,
				      int outExt[6], int id)
{
  int inExt[6];
  this->ComputeInputUpdateExtent(inExt,outExt);
  void *inPtr = inData->GetScalarPointerForExtent(inExt);
  void *outPtr = outData->GetScalarPointerForExtent(outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
    << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro8(vtkImageMagnifyExecute, this, inData, (VTK_TT *)(inPtr),
                      inExt, outData, (VTK_TT *)(outPtr), outExt, id);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}

void vtkImageMagnify::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "MagnificationFactors: ( "
     << this->MagnificationFactors[0] << ", "
     << this->MagnificationFactors[1] << ", "
     << this->MagnificationFactors[2] << " )\n";

  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");

}

