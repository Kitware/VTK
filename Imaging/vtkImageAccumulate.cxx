/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageAccumulate.cxx
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

#include "vtkImageAccumulate.h"
#include <math.h>
#include <stdlib.h>
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageAccumulate* vtkImageAccumulate::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageAccumulate");
  if(ret)
    {
    return (vtkImageAccumulate*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageAccumulate;
}





//----------------------------------------------------------------------------
// Constructor sets default values
vtkImageAccumulate::vtkImageAccumulate()
{
  int idx;
  
  for (idx = 0; idx < 3; ++idx)
    {
    this->ComponentSpacing[idx] = 1.0;
    this->ComponentOrigin[idx] = 0.0;
    this->ComponentExtent[idx*2] = 0;
    this->ComponentExtent[idx*2+1] = 0;
    }
  this->ComponentExtent[1] = 255;
  
  this->StencilFunction = NULL;
  this->ClippingExtents = NULL;
 
  this->ReverseStencil = 0;

  this->Min[0] = this->Min[1] = this->Min[2] = 0.0;
  this->Max[0] = this->Max[1] = this->Max[2] = 0.0;
  this->Mean[0] = this->Mean[1] = this->Mean[2] = 0.0;
  this->PixelCount = 0;
}


//----------------------------------------------------------------------------
vtkImageAccumulate::~vtkImageAccumulate()
{
  this->SetStencilFunction(NULL);
  this->SetClippingExtents(NULL);
}

//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int extent[6])
{
  int idx, modified = 0;
  
  for (idx = 0; idx < 6; ++idx)
    {
    if (this->ComponentExtent[idx] != extent[idx])
      {
      this->ComponentExtent[idx] = extent[idx];
      this->Modified();
      }
    }
  if (modified)
    {
    this->Modified();
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::SetComponentExtent(int minX, int maxX, 
                                            int minY, int maxY,
                                            int minZ, int maxZ)
{
  int extent[6];
  
  extent[0] = minX;  extent[1] = maxX;
  extent[2] = minY;  extent[3] = maxY;
  extent[4] = minZ;  extent[5] = maxZ;
  this->SetComponentExtent(extent);
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::GetComponentExtent(int extent[6])
{
  int idx;
  
  for (idx = 0; idx < 6; ++idx)
    {
    extent[idx] = this->ComponentExtent[idx];
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageAccumulateExecute(vtkImageAccumulate *self,
				      vtkImageData *inData, T *inPtr,
				      vtkImageData *outData, int *outPtr,
				      double Min[3],
				      double Max[3],
				      double Mean[3],
				      long int *PixelCount)
{
  int idX, idY, idZ, idxC;
  int r1, r2, cr1, cr2, iter, rval;
  int min0, max0, min1, max1, min2, max2;
  int inInc0, inInc1, inInc2;
  T *tempPtr;
  int *outPtrC;
  int numC, outIdx, *outExtent, *outIncs;
  float *origin, *spacing;
  unsigned long count = 0;
  unsigned long target;

  // variables used to compute statistics (filter handles max 3 components)
  double sum[3];
  sum[0] = sum[1] = sum[2] = 0.0;
  Min[0] = Min[1] = Min[2] = VTK_DOUBLE_MAX;
  Max[0] = Max[1] = Max[2] = VTK_DOUBLE_MIN;
  *PixelCount = 0;
  
  vtkImageClippingExtents *clippingExtents = self->GetClippingExtents();

  // Zero count in every bin
  outData->GetExtent(min0, max0, min1, max1, min2, max2);
  memset((void *)outPtr, 0, 
         (max0-min0+1)*(max1-min1+1)*(max2-min2+1)*sizeof(int));
    
  // Get information to march through data 
  numC = inData->GetNumberOfScalarComponents();
  inData->GetUpdateExtent(min0, max0, min1, max1, min2, max2);
  inData->GetIncrements(inInc0, inInc1, inInc2);
  outExtent = outData->GetExtent();
  outIncs = outData->GetIncrements();
  origin = outData->GetOrigin();
  spacing = outData->GetSpacing();

  target = (unsigned long)((max2 - min2 + 1)*(max1 - min1 +1)/50.0);
  target++;


  // Loop through input pixels
  for (idZ = min2; idZ <= max2; idZ++)
    {
    for (idY = min1; idY <= max1; idY++)
      {
      if (!(count%target)) 
	{
        self->UpdateProgress(count/(50.0*target));
	}
      count++;
	

      if (clippingExtents == NULL)
        {
        tempPtr = inPtr + (inInc2*(idZ - min2) +
                           inInc1*(idY - min1));
        for (idX = min0; idX <= max0; idX++)
          {
	  // find the bin for this pixel.
	  outPtrC = outPtr;
	  for (idxC = 0; idxC < numC; ++idxC)
	    {
	    // Gather statistics
	    sum[idxC]+= *tempPtr;
	    if (*tempPtr > Max[idxC])
	      Max[idxC] = *tempPtr;
	    else if (*tempPtr < Min[idxC])
	      Min[idxC] = *tempPtr;
	    (*PixelCount)++;
	    // compute the index
	    outIdx = (int) floor((((double)*tempPtr++ - origin[idxC]) / spacing[idxC]));
	    if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
	      {
	      // Out of bin range
	      outPtrC = NULL;
	      break;
	      }
	    outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
	    }
	  if (outPtrC)
	    {
	    ++(*outPtrC);
	    }
        
          }          
        }
      else
        {
        iter = 0;
        cr1 = min0;
        for (;;)
          {
          rval = clippingExtents->GetNextExtent(r1, r2, min0, max0,
                                                idY, idZ, iter);
          cr2 = r1 - 1;
          if (!self->GetReverseStencil())
            {
            tempPtr = inPtr + (inInc2*(idZ - min2) +
                               inInc1*(idY - min1) +
                               numC*(cr1 - min0));

            for (idX = cr1; idX <= cr2; idX++)
              {
	      // find the bin for this pixel.
	      outPtrC = outPtr;
	      for (idxC = 0; idxC < numC; ++idxC)
		{
		// Gather statistics
		sum[idxC]+= *tempPtr;
		if (*tempPtr > Max[idxC])
		  Max[idxC] = *tempPtr;
		else if (*tempPtr < Min[idxC])
		  Min[idxC] = *tempPtr;
		(*PixelCount)++;
		// compute the index
		outIdx = (int) floor((((double)*tempPtr++ - origin[idxC]) / spacing[idxC]));
		if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
		  {
		  // Out of bin range
		  outPtrC = NULL;
		  break;
		  }
		outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
		}
	      if (outPtrC)
		{
		++(*outPtrC);
		}
	      
	      }
            }
          cr1 = r2 + 1;

          // break if no foreground extents left
          if (rval == 0)
            {
            break;
            }

          if (self->GetReverseStencil())
            {
            // do unchanged portion
            tempPtr = inPtr + (inInc2*(idZ - min2) +
                               inInc1*(idY - min1) +
                               numC*(r1 - min0));

            for (idX = r1; idX <= r2; idX++)
              {
	      // find the bin for this pixel.
	      outPtrC = outPtr;
	      for (idxC = 0; idxC < numC; ++idxC)
		{
		// Gather statistics
		sum[idxC]+= *tempPtr;
		if (*tempPtr > Max[idxC])
		  Max[idxC] = *tempPtr;
		else if (*tempPtr < Min[idxC])
		  Min[idxC] = *tempPtr;
		(*PixelCount)++;
		// compute the index
		outIdx = (int) floor((((double)*tempPtr++ - origin[idxC]) / spacing[idxC]));
		if (outIdx < outExtent[idxC*2] || outIdx > outExtent[idxC*2+1])
		  {
		  // Out of bin range
		  outPtrC = NULL;
		  break;
		  }
		outPtrC += (outIdx - outExtent[idxC*2]) * outIncs[idxC];
		}
	      if (outPtrC)
		{
		++(*outPtrC);
		}
	      
              }
            }
          }
        }
      }
    }
  
  if (*PixelCount) // avoid the div0
    {
    Mean[0] = sum[0] / (double)*PixelCount;    
    Mean[1] = sum[1] / (double)*PixelCount;    
    Mean[2] = sum[2] / (double)*PixelCount;    
    }
  else
    {
    Mean[0]=Mean[1]=Mean[2] = 0.0;
    }
  
}

        

//----------------------------------------------------------------------------
// This method is passed a input and output Data, and executes the filter
// algorithm to fill the output from the input.
// It just executes a switch statement to call the correct function for
// the Datas data types.
void vtkImageAccumulate::ExecuteData(vtkDataObject *vtkNotUsed(out))
{
  void *inPtr;
  void *outPtr;
  vtkImageData *inData = this->GetInput();
  vtkImageData *outData = this->GetOutput();
  
  vtkDebugMacro(<<"Executing image accumulate");
  
  // We need to allocate our own scalars since we are overriding
  // the superclasses "Execute()" method.
  outData->SetExtent(outData->GetWholeExtent());
  outData->AllocateScalars();
  
  inPtr = inData->GetScalarPointerForExtent(inData->GetUpdateExtent());
  outPtr = outData->GetScalarPointer();
  
  // Components turned into x, y and z
  if (this->GetInput()->GetNumberOfScalarComponents() > 3)
    {
    vtkErrorMacro("This filter can handle upto 3 components");
    return;
    }
  
  // this filter expects that output is type int.
  if (outData->GetScalarType() != VTK_INT)
    {
    vtkErrorMacro(<< "Execute: out ScalarType " << outData->GetScalarType()
                  << " must be int\n");
    return;
    }
  
  switch (inData->GetScalarType())
    {
    vtkTemplateMacro9(vtkImageAccumulateExecute, this, 
                      inData, (VTK_TT *)(inPtr), 
                      outData, (int *)(outPtr),
		      this->Min, this->Max,
		      this->Mean, &this->PixelCount);
    default:
      vtkErrorMacro(<< "Execute: Unknown ScalarType");
      return;
    }
}


//----------------------------------------------------------------------------
void vtkImageAccumulate::ExecuteInformation(vtkImageData *input, 
                                            vtkImageData *output)
{
  output->SetWholeExtent(this->ComponentExtent);
  output->SetOrigin(this->ComponentOrigin);
  output->SetSpacing(this->ComponentSpacing);
  output->SetNumberOfScalarComponents(1);
  output->SetScalarType(VTK_INT);

  // See if we should be setting up some clipping extents
  if (this->StencilFunction)
    {
    if (!this->ClippingExtents)
      {
      this->ClippingExtents = vtkImageClippingExtents::New();
      }
    this->ClippingExtents->SetClippingObject(this->StencilFunction);
    }
  if (this->ClippingExtents)
    {
    this->ClippingExtents->BuildExtents(input);
    }
}

//----------------------------------------------------------------------------
// Get ALL of the input.
void vtkImageAccumulate::ComputeInputUpdateExtent(int inExt[6], 
                                                  int outExt[6])
{
  int *wholeExtent;

  outExt = outExt;
  wholeExtent = this->GetInput()->GetWholeExtent();
  memcpy(inExt, wholeExtent, 6*sizeof(int));
}


void vtkImageAccumulate::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "ComponentOrigin: ( "
     << this->ComponentOrigin[0] << ", "
     << this->ComponentOrigin[1] << ", "
     << this->ComponentOrigin[2] << " )\n";

  os << indent << "ComponentSpacing: ( "
     << this->ComponentSpacing[0] << ", "
     << this->ComponentSpacing[1] << ", "
     << this->ComponentSpacing[2] << " )\n";

  os << indent << "ComponentExtent: ( "
     << this->ComponentExtent[0] << "," << this->ComponentExtent[1] << " "
     << this->ComponentExtent[2] << "," << this->ComponentExtent[3] << " "
     << this->ComponentExtent[4] << "," << this->ComponentExtent[5] << " }\n";

  os << indent << "StencilFunction: " << this->StencilFunction << "\n";
  os << indent << "ReverseStencil: " << (this->ReverseStencil ?
		                         "On\n" : "Off\n");

  os << indent << "ClippingExtents: " << this->ClippingExtents << "\n";
}

