/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAnisotropicDiffusion2D.cxx
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
#include <math.h>
#include "vtkImageAnisotropicDiffusion2D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageAnisotropicDiffusion2D* vtkImageAnisotropicDiffusion2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageAnisotropicDiffusion2D");
  if(ret)
    {
    return (vtkImageAnisotropicDiffusion2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageAnisotropicDiffusion2D;
}





//----------------------------------------------------------------------------
// Construct an instance of vtkImageAnisotropicDiffusion2D fitler.
vtkImageAnisotropicDiffusion2D::vtkImageAnisotropicDiffusion2D()
{
  this->HandleBoundaries = 1;
  this->NumberOfIterations = 0;
  this->SetNumberOfIterations(4);
  this->DiffusionThreshold = 5.0;
  this->DiffusionFactor = 1;
  this->Faces = 0;
  this->FacesOn();
  this->Edges = 0;
  this->EdgesOn();
  this->Corners = 0;
  this->CornersOn();
  this->GradientMagnitudeThreshold = 1;
  this->GradientMagnitudeThresholdOff();
}


//----------------------------------------------------------------------------
void 
vtkImageAnisotropicDiffusion2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageSpatialFilter::PrintSelf(os, indent);
  os << indent << "NumberOfIterations: " << this->NumberOfIterations << "\n";
  os << indent << "DiffusionThreshold: " << this->DiffusionThreshold << "\n";
  os << indent << "DiffusionFactor: " << this->DiffusionFactor << "\n";

  os << indent << "Faces: " << this->Faces << "\n";

  if (this->Edges)
    {
    os << indent << "Edges: On\n";
    }
  else
    {
    os << indent << "Edges: Off\n";
    }

  if (this->Corners)
    {
    os << indent << "Corners: On\n";
    }
  else
    {
    os << indent << "Corners: Off\n";
    }

  if (this->GradientMagnitudeThreshold)
    {
    os << indent << "GradientMagnitudeThreshold: On\n";
    }
  else
    {
    os << indent << "GradientMagnitudeThreshold: Off\n";
    }
}


//----------------------------------------------------------------------------
// This method sets the number of inputs which also affects the
// input neighborhood needed to compute one output pixel.
void vtkImageAnisotropicDiffusion2D::SetNumberOfIterations(int num)
{
  int temp;
  
  vtkDebugMacro(<< "SetNumberOfIterations: " << num);
  if (this->NumberOfIterations == num)
    {
    return;
    }

  this->Modified();
  temp = num*2 + 1;
  this->KernelSize[0] = temp;
  this->KernelSize[1] = temp;
  this->KernelMiddle[0] = num;
  this->KernelMiddle[1] = num;

  this->NumberOfIterations = num;
}


  
  
  
  
//----------------------------------------------------------------------------
// This method contains a switch statement that calls the correct
// templated function for the input data type.  The input and output datas
// must have the same data type.
void vtkImageAnisotropicDiffusion2D::ThreadedExecute(vtkImageData *inData, 
						     vtkImageData *outData,
						     int outExt[6], int id)
{
  int inExt[6];
  float *ar;
  int idx;
  vtkImageData *temp;
  
  this->ComputeInputUpdateExtent(inExt,outExt);
  
  vtkDebugMacro(<< "Execute: inData = " << inData 
  << ", outData = " << outData);
  
  // this filter expects that input is the same type as output.
  if (inData->GetScalarType() != outData->GetScalarType())
    {
    vtkErrorMacro(<< "Execute: input ScalarType, " << inData->GetScalarType()
                  << ", must match out ScalarType " << outData->GetScalarType());
    return;
    }
  
  ar = inData->GetSpacing();

  // make the temporary regions to iterate over.
  vtkImageData *in = vtkImageData::New();
  in->SetExtent(inExt);
  in->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
  in->SetScalarType(VTK_FLOAT);
  in->CopyAndCastFrom(inData,inExt);
  
  vtkImageData *out = vtkImageData::New();
  out->SetExtent(inExt);
  out->SetNumberOfScalarComponents(inData->GetNumberOfScalarComponents());
  out->SetScalarType(VTK_FLOAT);
  
  // Loop performing the diffusion
  // Note: region extent could get smaller as the diffusion progresses
  // (but never get smaller than output region).
  for (idx = this->NumberOfIterations - 1; 
       !this->AbortExecute && idx >= 0; --idx)
    {
    if (!id)
      {
      this->UpdateProgress((float)(this->NumberOfIterations - idx)
			   /this->NumberOfIterations);
      }
    this->Iterate(in, out, ar[0], ar[1], outExt, idx);
    temp = in;
    in = out;
    out = temp;
    }
  
  // copy results into output.
  outData->CopyAndCastFrom(in,outExt);
  in->Delete();
  out->Delete();
}

  

//----------------------------------------------------------------------------
// This method performs one pass of the diffusion filter.
// The inData and outData are assumed to have data type float,
// and have the same extent.
void vtkImageAnisotropicDiffusion2D::Iterate(vtkImageData *inData, 
					     vtkImageData *outData,
					     float ar0, float ar1,
					     int *coreExtent, int count)
{
  int idx0, idx1, idx2;
  int inInc0, inInc1, inInc2;
  int outInc0, outInc1, outInc2;
  int inMin0, inMax0, inMin1, inMax1, inMin2, inMax2;
  int min0, max0, min1, max1, min2, max2;
  float *inPtr0, *inPtr1, *inPtr2;
  float *outPtr0, *outPtr1, *outPtr2;
  float th0, th1, th01;
  float df0, df1, df01;
  float temp, sum;
  int idxC, maxC;

  maxC = inData->GetNumberOfScalarComponents();
  inData->GetExtent(inMin0, inMax0, inMin1, inMax1, inMin2, inMax2);
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outData->GetIncrements(outInc0, outInc1, outInc2);

  // Avoid warnings.
  th0 = th1 = th01 = df0 = df1 = df01 = 0.0;
  
  // Compute direction specific diffusion thresholds and factors.
  sum = 0.0;
  if (this->Edges)
    {
    th0 = ar0 * this->DiffusionThreshold;
    df0 = 1.0 / ar0;
    th1 = ar1 * this->DiffusionThreshold;
    df1 = 1.0 / ar1;
    // two edges per direction.
    sum += 2.0 * (df0 + df1);
    }
  if (this->Corners)
    {
    temp = sqrt(ar0*ar0 + ar1*ar1);
    th01 = temp * this->DiffusionThreshold;
    df01 = 1 / temp;
    // four corners per plane
    sum += 4 * (df01);
    }

  if (sum > 0.0)
    {
    temp = this->DiffusionFactor / sum;
    df0 *= temp;
    df1 *= temp;
    df01 *= temp;
    }
  else
    {
    vtkWarningMacro(<< "Iterate: NO NEIGHBORS");
    return;
    }

  // Compute the shrinking extent to loop over.
  min0 = coreExtent[0] - count;
  max0 = coreExtent[1] + count;
  min1 = coreExtent[2] - count;
  max1 = coreExtent[3] + count;
  // intersection
  min0 = (min0 > inMin0) ? min0 : inMin0;
  max0 = (max0 < inMax0) ? max0 : inMax0;
  min1 = (min1 > inMin1) ? min1 : inMin1;
  max1 = (max1 < inMax1) ? max1 : inMax1;
  
  vtkDebugMacro(<< "Iteration count: " << count << " ("
  << min0 << ", " << max0 << ", " << min1 << ", " << max1 << ")");
  
  // I apologize for explicitely diffusing each neighbor, but it is the easiest
  // way to deal with the boundary conditions.  Besides it is fast.
  // (Are you sure every one is correct?!!!)
  min2 = inMin2;
  max2 = inMax2;
  
  
  inPtr2 = (float *)(inData->GetScalarPointer(min0, min1, min2));
  outPtr2 = (float *)(outData->GetScalarPointer(min0, min1, min2));

  for (idxC = 0; idxC < maxC; idxC++)
    {
    inPtr2 = (float *)(inData->GetScalarPointer(min0, min1, min2));
    outPtr2 = (float *)(outData->GetScalarPointer(min0, min1, min2));
    inPtr2 += idxC;
    outPtr2 += idxC;
    
    for (idx2 = min2; idx2 <= max2; ++idx2, inPtr2+=inInc2, outPtr2+=outInc2)
      {
      inPtr1 = inPtr2;
      outPtr1 = outPtr2;    
      for (idx1 = min1; idx1 <= max1; ++idx1, inPtr1+=inInc1, outPtr1+=outInc1)
	{
	inPtr0 = inPtr1;
	outPtr0 = outPtr1;    
	for (idx0 = min0; idx0 <= max0; ++idx0, inPtr0+=inInc0, outPtr0+=outInc0)
	  {
	  // Copy center
	  *outPtr0 = *inPtr0;
	
	  // Special case for gradient magnitude threhsold 
	  if (this->GradientMagnitudeThreshold)
	    {
	    float d0, d1;
	    // compute the gradient magnitude (central differences).
	    d0  = (idx0 != inMax0) ? inPtr0[inInc0] : *inPtr0;
	    d0 -= (idx0 != inMin0) ? inPtr0[-inInc0] : *inPtr0;
	    d0 /= ar0;
	    d1  = (idx1 != inMax1) ? inPtr0[inInc1] : *inPtr0;
	    d1 -= (idx1 != inMin1) ? inPtr0[-inInc1] : *inPtr0;
	    d1 /= ar1;
	    // If magnitude is big, don't diffuse.
	    d0 = sqrt(d0*d0 + d1*d1);
	    if (d0 > this->DiffusionThreshold)
	      {
	      // hack to not diffuse
	      th0 = th1 = th01 = 0.0;
	      }
	    else
	      {
	      // hack to diffuse
	      th0 = th1 = th01 = VTK_LARGE_FLOAT;
	      }
	    }
	
	  // Start diffusing
	  if (this->Edges)
	    {
	    // left
	    if (idx0 != inMin0)
	      {
	      temp = inPtr0[-inInc0] - *inPtr0;
	      if (fabs(temp) < th0)
		{
		*outPtr0 += temp * df0;
		}
	      }
	    // right
	    if (idx0 != inMax0)
	      {
	      temp = inPtr0[inInc0] - *inPtr0;
	      if (fabs(temp) < th0)
		{
		*outPtr0 += temp * df0;
		}
	      }
	    // up
	    if (idx1 != inMin1)
	      {
	      temp = inPtr0[-inInc1] - *inPtr0;
	      if (fabs(temp) < th1)
		{
		*outPtr0 += temp * df1;
		}
	      }
	    // down
	    if (idx1 != inMax1)
	      {
	      temp = inPtr0[inInc1] - *inPtr0;
	      if (fabs(temp) < th1)
		{
		*outPtr0 += temp * df1;
		}
	      }
	    }
	
	  if (this->Corners)
	    {
	    // left up
	    if (idx0 != inMin0 && idx1 != inMin1)
	      {
	      temp = inPtr0[-inInc0-inInc1] - *inPtr0;
	      if (fabs(temp) < th01)
		{
		*outPtr0 += temp * df01;
		}
	      }
	    // right up
	    if (idx0 != inMax0 && idx1 != inMin1)
	      {
	      temp = inPtr0[inInc0-inInc1] - *inPtr0;
	      if (fabs(temp) < th01)
		{
		*outPtr0 += temp * df01;
		}
	      }
	    // left down
	    if (idx0 != inMin0 && idx1 != inMax1)
	      {
	      temp = inPtr0[-inInc0+inInc1] - *inPtr0;
	      if (fabs(temp) < th01)
		{
		*outPtr0 += temp * df01;
		}
	      }
	    // right down
	    if (idx0 != inMax0 && idx1 != inMax1)
	      {
	      temp = inPtr0[inInc0+inInc1] - *inPtr0;
	      if (fabs(temp) < th01)
		{
		*outPtr0 += temp * df01;
		}
	      }
	    }
	  }
	}
      }
    }
}

  
