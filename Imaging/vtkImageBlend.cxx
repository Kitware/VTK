/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi and Sebastien Barre who developed this class.

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

#include "vtkImageBlend.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageBlend* vtkImageBlend::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageBlend");
  if(ret)
    {
    return (vtkImageBlend*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageBlend;
}

//----------------------------------------------------------------------------
vtkImageBlend::vtkImageBlend()
{
  this->Stencil = 0;
  this->Opacity = 0;
  this->OpacityArrayLength = 0;
  this->BlendMode = VTK_IMAGE_BLEND_MODE_NORMAL;
  this->CompoundThreshold = 0.0;
  this->DataWasPassed = 0;
}

//----------------------------------------------------------------------------
vtkImageBlend::~vtkImageBlend()
{
  this->SetStencil(0);
  if (this->Opacity)
    {
    delete [] this->Opacity;
    }
  this->OpacityArrayLength = 0;
}

//----------------------------------------------------------------------------
void vtkImageBlend::SetOpacity(int idx, double opacity)
{
  int i;
  int newLength;
  double *newArray;

  if (opacity < 0.0)
    {
    opacity = 0.0;
    }
  if (opacity > 1.0)
    {
    opacity = 1.0;
    }

  if (idx >= this->OpacityArrayLength)
    {
    newLength = idx + 1; 
    newArray = new double[newLength];
    for (i = 0; i < this->OpacityArrayLength; i++)
      {
      newArray[i] = this->Opacity[i];
      }
    for (; i < newLength; i++)
      {
      newArray[i] = 1.0;
      }
    if (this->Opacity)
      {
      delete [] this->Opacity;
      }
    this->Opacity = newArray;
    this->OpacityArrayLength = newLength;
    }

  if (this->Opacity[idx] != opacity)
    {
    this->Opacity[idx] = opacity;
    this->Modified();
    }
}
    
//----------------------------------------------------------------------------
double vtkImageBlend::GetOpacity(int idx)
{
  if (idx >= this->OpacityArrayLength)
    {
    return 1.0;
    }
  return this->Opacity[idx];
}    

//----------------------------------------------------------------------------
void vtkImageBlend::ExecuteInformation(vtkImageData **inDatas,
				       vtkImageData *outData)
{
  vtkImageStencilData *stencil = this->GetStencil();
  if (stencil)
    {
    stencil->SetSpacing(inDatas[0]->GetSpacing());
    stencil->SetOrigin(inDatas[0]->GetOrigin());
    }
}

//----------------------------------------------------------------------------
// This method computes the extent of the input region necessary to generate
// an output region.  Before this method is called "region" should have the
// extent of the output region.  After this method finishes, "region" should
// have the extent of the required input region.  The default method assumes
// the required input extent are the same as the output extent.
// Note: The splitting methods call this method with outRegion = inRegion.
void vtkImageBlend::ComputeInputUpdateExtent(int inExt[6],
					     int outExt[6],
					     int whichInput)
{
  memcpy(inExt,outExt,sizeof(int)*6);

  int *wholeExtent = this->GetInput(whichInput)->GetWholeExtent();
  int i;

  // clip with the whole extent
  for (i = 0; i < 3; i++)
    {
    if (inExt[2*i] < wholeExtent[2*i])
      {
      inExt[2*i] = wholeExtent[2*i];
      }
    if (inExt[2*i+1] > wholeExtent[2*i+1])
      {
      inExt[2*i+1] = wholeExtent[2*i+1];
      }
    }
}

//----------------------------------------------------------------------------
// This method checks to see if we can simply reference the input data
void vtkImageBlend::ExecuteData(vtkDataObject *output)
{
  // check to see if we have more than one input
  int singleInput = 1;
  for (int idx = 1; idx < this->NumberOfInputs; idx++)
    {
    if (this->GetInput(idx) != NULL)
      {
      singleInput = 0;
      }
    }
  if (singleInput)
    {
    vtkDebugMacro("ExecuteData: single input, passing data");

    vtkImageData *outData = (vtkImageData *)(output);
    vtkImageData *inData = this->GetInput();
 
    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    this->DataWasPassed = 1;
    }
  else // multiple inputs
    {
    if (this->DataWasPassed)
      {
      ((vtkImageData *)output)->GetPointData()->SetScalars((vtkScalars*)NULL);
      this->DataWasPassed = 0;
      }
    
    vtkImageStencilData *stencil = this->GetStencil();
    if (stencil)
      {
      stencil->SetUpdateExtent(((vtkImageData *)output)->GetUpdateExtent());
      stencil->Update();
      }
    this->vtkImageMultipleInputFilter::ExecuteData(output);
    }
}

//----------------------------------------------------------------------------
// helper function for the stencil
template <class T>
static inline int vtkBlendGetNextExtent(vtkImageStencilData *stencil,
					int &r1, int &r2, int rmin, int rmax,
					int yIdx, int zIdx, 
					T *&outPtr, T *&inPtr, 
					int outScalars, int inScalars,
					int &iter)
{
  // trivial case if stencil is not set
  if (!stencil)
    {
    if (iter++ == 0)
      {
      r1 = rmin;
      r2 = rmax;
      return 1;
      }
    return 0;
    }

  // save r2
  int oldr2 = r2;
  if (iter == 0)
    { // if no 'last time', start just before rmin
    oldr2 = rmin - 1;
    }

  int rval = stencil->GetNextExtent(r1, r2, rmin, rmax, yIdx, zIdx, iter);
  int incr = r1 - oldr2 - 1;
  if (rval == 0)
    {
    incr = rmax - oldr2;
    }

  outPtr += incr*outScalars;
  inPtr += incr*inScalars;

  return rval;
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBlendExecute(vtkImageBlend *self, int extent[6], 
				 vtkImageData *inData, T *inPtr,
				 vtkImageData *outData, T *outPtr,
				 float opacity, int id)
{
  int idxX, idxY, idxZ;
  int minX, maxX, iter;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int inC, outC;
  float minA,maxA;
  float r,f;
  unsigned long count = 0;
  unsigned long target;

  vtkImageStencilData *stencil = self->GetStencil();

  if (inData->GetScalarType() == VTK_DOUBLE ||
      inData->GetScalarType() == VTK_FLOAT)
    {
    minA = 0.0;
    maxA = 1.0;
    }
  else
    {
    minA = inData->GetScalarTypeMin();
    maxA = inData->GetScalarTypeMax();
    }

  r = opacity;
  f = 1.0-r;

  opacity = opacity/(maxA-minA);

  inC = inData->GetNumberOfScalarComponents();
  outC = outData->GetNumberOfScalarComponents();

  target = (unsigned long)((extent[3] - extent[2] + 1)*
			   (extent[5] - extent[4] + 1)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(extent, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(extent, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (idxY = extent[2]; !self->AbortExecute && idxY <= extent[3]; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      iter = 0;
      if (outC >= 3 && inC >= 4)
	{ // RGB(A) blended with RGBA
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = opacity*(inPtr[3]-minA);
	    f = 1.0-r;
	    outPtr[0] = T(outPtr[0]*f + inPtr[0]*r);
	    outPtr[1] = T(outPtr[1]*f + inPtr[1]*r);
	    outPtr[2] = T(outPtr[2]*f + inPtr[2]*r);
	    outPtr += outC; 
	    inPtr += inC;
	    }
	  }
	}
      else if (outC >= 3 && inC == 3)
	{ // RGB(A) blended with RGB
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    outPtr[0] = T(outPtr[0]*f + inPtr[0]*r);
	    outPtr[1] = T(outPtr[1]*f + inPtr[1]*r);
	    outPtr[2] = T(outPtr[2]*f + inPtr[2]*r);
	    outPtr += outC; 
	    inPtr += inC;
	    }
	  }
	}
      else if (outC >= 3 && inC == 2)
	{ // RGB(A) blended with luminance+alpha
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = opacity*(inPtr[1]-minA);
	    f = 1.0-r;
	    outPtr[0] = T(outPtr[0]*f + (*inPtr)*r);
	    outPtr[1] = T(outPtr[1]*f + (*inPtr)*r);
	    outPtr[2] = T(outPtr[2]*f + (*inPtr)*r);
	    outPtr += outC; 
	    inPtr += 2;
	    }
	  }
	}
      else if (outC >= 3 && inC == 1)
	{ // RGB(A) blended with luminance
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    outPtr[0] = T(outPtr[0]*f + (*inPtr)*r);
	    outPtr[1] = T(outPtr[1]*f + (*inPtr)*r);
	    outPtr[2] = T(outPtr[2]*f + (*inPtr)*r);
	    outPtr += outC; 
	    inPtr++;
	    }
	  }
	}
      else if (inC == 2)
	{ // luminance(+alpha) blended with luminance+alpha
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = opacity*(inPtr[1]-minA);
	    f = 1.0-r;
	    *outPtr = T((*outPtr)*f + (*inPtr)*r);
	    outPtr += outC; 
	    inPtr += 2;
	    }
	  }
	}
      else
	{ // luminance(+alpha) blended with luminance
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    *outPtr = T((*outPtr)*f + (*inPtr)*r);
	    outPtr += outC; 
	    inPtr++;
	    }
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
// This templated function executes the filter specifically for char data
template <class T>
static void vtkImageBlendExecuteChar(vtkImageBlend *self, int extent[6], 
				     vtkImageData *inData, T *inPtr,
				     vtkImageData *outData, T *outPtr,
				     float opacity, int id)
{
  int idxX, idxY, idxZ;
  int minX, maxX, iter;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int inC, outC;
  unsigned short r,f;
  unsigned long count = 0;
  unsigned long target;

  vtkImageStencilData *stencil = self->GetStencil();

  r = (unsigned short)(255*opacity);
  f = 255-r;

  inC = inData->GetNumberOfScalarComponents();
  outC = outData->GetNumberOfScalarComponents();

  target = (unsigned long)((extent[3] - extent[2] + 1)*
			   (extent[5] - extent[4] + 1)/50.0);
  target++;  

  // Get increments to march through data 
  inData->GetContinuousIncrements(extent, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(extent, outIncX, outIncY, outIncZ);

   // Loop through ouput pixels
  for (idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (idxY = extent[2]; !self->AbortExecute && idxY <= extent[3]; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      
      iter = 0;
      if (outC >= 3 && inC >= 4)
	{ // RGB(A) blended with RGBA
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = (unsigned short)(inPtr[3]*opacity);
	    f = 255-r;
	    outPtr[0] = (outPtr[0]*f + inPtr[0]*r) >> 8;
	    outPtr[1] = (outPtr[1]*f + inPtr[1]*r) >> 8;
	    outPtr[2] = (outPtr[2]*f + inPtr[2]*r) >> 8;
	    outPtr += outC; 
	    inPtr += inC;
	    }
	  }
	}
      else if (outC >= 3 && inC == 3)
	{ // RGB(A) blended with RGB
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    outPtr[0] = (outPtr[0]*f + inPtr[0]*r) >> 8;
	    outPtr[1] = (outPtr[1]*f + inPtr[1]*r) >> 8;
	    outPtr[2] = (outPtr[2]*f + inPtr[2]*r) >> 8;
	    outPtr += outC; 
	    inPtr += inC;
	    }
	  }
	}
      else if (outC >= 3 && inC == 2)
	{ // RGB(A) blended with luminance+alpha
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = (unsigned short)(inPtr[1]*opacity);
	    f = 255-r;
	    outPtr[0] = (outPtr[0]*f + (*inPtr)*r) >> 8;
	    outPtr[1] = (outPtr[1]*f + (*inPtr)*r) >> 8;
	    outPtr[2] = (outPtr[2]*f + (*inPtr)*r) >> 8;
	    outPtr += outC; 
	    inPtr += 2;
	    }
	  }
	}
      else if (outC >= 3 && inC == 1)
	{ // RGB(A) blended with luminance
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    outPtr[0] = (outPtr[0]*f + (*inPtr)*r) >> 8;
	    outPtr[1] = (outPtr[1]*f + (*inPtr)*r) >> 8;
	    outPtr[2] = (outPtr[2]*f + (*inPtr)*r) >> 8;
	    outPtr += outC; 
	    inPtr++;
	    }
	  }
	}
      else if (inC == 2)
	{ // luminance(+alpha) blended with luminance+alpha
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    r = (unsigned short)(inPtr[1]*opacity);
	    f = 255-r;
	    *outPtr = ((*outPtr)*f + (*inPtr)*r) >> 8;
	    outPtr += outC; 
	    inPtr += 2;
	    }
	  }
	}
      else
	{ // luminance(+alpha) blended with luminance
	while (vtkBlendGetNextExtent(stencil, minX, maxX, extent[0], extent[1],
				     idxY, idxZ,
				     outPtr, inPtr, outC, inC, iter))
	  {
	  for (idxX = minX; idxX <= maxX; idxX++)
	    {
	    *outPtr = ((*outPtr)*f + (*inPtr)*r) >> 8;
	    outPtr += outC; 
	    inPtr++;
	    }
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
// This function simply does a copy (for the first input)
//----------------------------------------------------------------------------
void vtkImageBlendCopyData(vtkImageData *inData, vtkImageData *outData,
			   int *ext)
{
  int idxY, idxZ, maxY, maxZ;
  int inIncX, inIncY, inIncZ, rowLength;
  unsigned char *inPtr, *inPtr1, *outPtr;
 
  inPtr = (unsigned char *) inData->GetScalarPointerForExtent(ext);
  outPtr = (unsigned char *) outData->GetScalarPointerForExtent(ext);
 
  // Get increments to march through inData
  inData->GetIncrements(inIncX, inIncY, inIncZ);
 
  // find the region to loop over
  rowLength = (ext[1] - ext[0]+1)*inIncX*inData->GetScalarSize();
  maxY = ext[3] - ext[2];
  maxZ = ext[5] - ext[4];

  inIncY *= inData->GetScalarSize();
  inIncZ *= inData->GetScalarSize();

  // Loop through outData pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    inPtr1 = inPtr + idxZ*inIncZ;
    for (idxY = 0; idxY <= maxY; idxY++)
      {
      memcpy(outPtr,inPtr1,rowLength);
      inPtr1 += inIncY;
      outPtr += rowLength;
      }
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBlendCompoundExecute(vtkImageBlend *self,
                                         int extent[6], 
                                         vtkImageData *inData, 
                                         T *inPtr,
                                         vtkImageData *tmpData, 
                                         float opacity,
                                         float threshold)
{
  unsigned long count = 0;
  unsigned long target;

  target = (unsigned long)((extent[3] - extent[2] + 1)*
			   (extent[5] - extent[4] + 1)/50.0);
  target++;
  
  // Get increments to march through data 

  int inIncX, inIncY, inIncZ;
  int inC;

  inData->GetContinuousIncrements(extent, inIncX, inIncY, inIncZ);
  inC = inData->GetNumberOfScalarComponents();

  int tmpIncX, tmpIncY, tmpIncZ;
  int tmpC;

  tmpData->GetContinuousIncrements(extent, tmpIncX, tmpIncY, tmpIncZ);
  tmpC = tmpData->GetNumberOfScalarComponents();

  float* tmpPtr = (float *)tmpData->GetScalarPointerForExtent(extent);  

  // Opacity

  float minA, maxA;
  float r;

  if (inData->GetScalarType() == VTK_DOUBLE ||
      inData->GetScalarType() == VTK_FLOAT)
    {
    minA = 0.0;
    maxA = 1.0;
    }
  else
    {
    minA = (float)inData->GetScalarTypeMin();
    maxA = (float)inData->GetScalarTypeMax();
    }

  r = opacity;
  opacity = opacity/(maxA-minA);

  if ((inC == 3 || inC == 1) && r <= threshold)
    {
    return;
    }

  // Loop through output pixels

  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (int idxY = extent[2]; !self->AbortExecute && idxY<=extent[3]; idxY++)
      {
      if (!(count%target))
        {
        self->UpdateProgress(count/(50.0*target));
        }
      count++;
      
      if (tmpC >= 3)
        {

        // RGB(A) blended with RGBA
        if (inC >= 4)
          { 
          for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
            {
            r = opacity * ((float)inPtr[3] - minA);
            if (r > threshold) 
              {
              tmpPtr[0] += (float)inPtr[0] * r;
              tmpPtr[1] += (float)inPtr[1] * r;
              tmpPtr[2] += (float)inPtr[2] * r;
              tmpPtr[3] += r;
              }
            tmpPtr += 4; 
            inPtr += inC;
            }
          }

        // RGB(A) blended with RGB
        else if (inC == 3)
          { 
          for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
            {
            tmpPtr[0] += (float)inPtr[0] * r;
            tmpPtr[1] += (float)inPtr[1] * r;
            tmpPtr[2] += (float)inPtr[2] * r;
            tmpPtr[3] += r;
            tmpPtr += 4; 
            inPtr += inC;
            }
          }

        // RGB(A) blended with luminance+alpha
        else if (inC == 2)
          { 
          for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
            {
            r = opacity * ((float)inPtr[1] - minA);
            if (r > threshold) 
              {
              tmpPtr[0] += (float)(*inPtr) * r;
              tmpPtr[1] += (float)(*inPtr) * r;
              tmpPtr[2] += (float)(*inPtr) * r;
              tmpPtr[3] += r;
              }
            tmpPtr += 4; 
            inPtr += 2;
            }
          }

        // RGB(A) blended with luminance
        else if (inC == 1)
          { 
          for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
            {
            tmpPtr[0] += (float)(*inPtr) * r;
            tmpPtr[1] += (float)(*inPtr) * r;
            tmpPtr[2] += (float)(*inPtr) * r;
            tmpPtr[3] += r;
            tmpPtr += 4; 
            inPtr++;
            }
          }
        }

      // luminance(+alpha) blended with luminance+alpha
      else if (inC == 2)
	{ 
        for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
	  {
          r = opacity * ((float)inPtr[1] - minA);
          if (r > threshold) 
            {
            tmpPtr[0] = (float)(*inPtr) * r;
            tmpPtr[1] += r;
            }
          tmpPtr += 2; 
	  inPtr += 2;
	  }
	}

      // luminance(+alpha) blended with luminance
      else
	{ 
        for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
	  {
	  tmpPtr[0] = (float)(*inPtr) * r;
          tmpPtr[1] += r;
          tmpPtr += 2; 
	  inPtr++;
	  }
	}

      tmpPtr += tmpIncY;
      inPtr += inIncY;
      }
    tmpPtr += tmpIncZ;
    inPtr += inIncZ;
    }
}


//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBlendCompoundTransferExecute(vtkImageBlend *self,
						 int extent[6], 
						 vtkImageData *outData, 
						 T *outPtr,
						 vtkImageData *tmpData)
{
  // Get increments to march through data 

  int outIncX, outIncY, outIncZ;
  int outC;

  outData->GetContinuousIncrements(extent, outIncX, outIncY, outIncZ);
  outC = outData->GetNumberOfScalarComponents();

  int tmpIncX, tmpIncY, tmpIncZ;
  int tmpC;

  tmpData->GetContinuousIncrements(extent, tmpIncX, tmpIncY, tmpIncZ);
  tmpC = tmpData->GetNumberOfScalarComponents();

  float* tmpPtr = (float *)tmpData->GetScalarPointerForExtent(extent);  

  // Loop through output pixels

  for (int idxZ = extent[4]; idxZ <= extent[5]; idxZ++)
    {
    for (int idxY = extent[2]; !self->AbortExecute && idxY<=extent[3]; idxY++)
      {
      if (tmpC >= 3)
        {
        for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
          {
          if (tmpPtr[3] != 0) 
            {
            outPtr[0] = T(tmpPtr[0] / tmpPtr[3]);
            outPtr[1] = T(tmpPtr[1] / tmpPtr[3]);
            outPtr[2] = T(tmpPtr[2] / tmpPtr[3]);
            } 
          else 
            {
            outPtr[0] = outPtr[1] = outPtr[2] = T(0);
            }
          tmpPtr += 4; 
          outPtr += outC;
          }
        }
      else 
        {
        for (int idxX = extent[0]; idxX <= extent[1]; idxX++)
          {
          if (tmpPtr[1] != 0) 
            {
            *outPtr = T(tmpPtr[0] / tmpPtr[1]);
            }
          else
	    {
            *outPtr = T(0);
	    }
          tmpPtr += 2;
          outPtr += outC;
          }
        }

      tmpPtr += tmpIncY;
      outPtr += outIncY;
      }
    tmpPtr += tmpIncZ;
    outPtr += outIncZ;
    }
}


//----------------------------------------------------------------------------
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageBlend::ThreadedExecute(vtkImageData **inData, 
				    vtkImageData *outData,
				    int outExt[6], 
				    int id)
{
  int extent[6];
  void *inPtr;
  void *outPtr;

  float opacity;

  vtkImageData *tmpData = NULL;
  
  // check

  if (inData[0]->GetNumberOfScalarComponents() > 4)
    {
    vtkErrorMacro("The first input can have a maximum of four components");
    return;
    }

  // init

  switch (this->BlendMode)
    {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      // copy the first image directly to the output
      vtkDebugMacro("Execute: copy input 0 to the output.");
      vtkImageBlendCopyData(inData[0], outData, outExt);
      break;

    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      tmpData = vtkImageData::New();
      if (tmpData == NULL) 
        {
        vtkErrorMacro(<< "Execute: Unable to allocate memory");
        return;
        }
      tmpData->SetExtent(outExt);
      tmpData->SetNumberOfScalarComponents(
        (outData->GetNumberOfScalarComponents() >= 3 ? 3 : 1) + 1);
      tmpData->SetScalarType(VTK_FLOAT);
      tmpData->AllocateScalars();
      memset((void *)tmpData->GetScalarPointer(), 0, 
             (outExt[1] - outExt[0] + 1) * 
             (outExt[3] - outExt[2] + 1) * 
             (outExt[5] - outExt[4] + 1) *
             tmpData->GetNumberOfScalarComponents() * 
             tmpData->GetScalarSize());
      break;

    default:
      vtkErrorMacro(<< "Execute: Unknown blending mode");
    }

  // process each input

  int first_index = (this->BlendMode == VTK_IMAGE_BLEND_MODE_NORMAL ? 1 : 0);
  for (int idx1 = first_index; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {

      // RGB with RGB, greyscale with greyscale

      if ((inData[idx1]->GetNumberOfScalarComponents()+1)/2 == 2 &&
	  (inData[0]->GetNumberOfScalarComponents()+1)/2 == 1)
	{
	vtkErrorMacro("input has too many components, can't blend RGB data \
                       into greyscale data");
	continue;
	}
      
      // this filter expects that input is the same type as output.
      
      if (inData[idx1]->GetScalarType() != outData->GetScalarType())
	{
	vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << 
	inData[idx1]->GetScalarType() << 
	"), must match output ScalarType (" << outData->GetScalarType() 
	<< ")");
	continue;
	}
      
      // input extents

      this->ComputeInputUpdateExtent(extent, outExt, idx1);

      int skip = 0;
      for (int i = 0; i < 3; i++)
        {
	if (outExt[2*i+1] < extent[2*i] || outExt[2*i] > extent[2*i+1])
	  {
          // extents don't overlap, skip this input
	  skip = 1; 
	  }
        }
      
      if (skip) 
        {
        vtkDebugMacro("Execute: skipping input.");
        continue;
        }
      
      opacity = this->GetOpacity(idx1);
          
      inPtr = inData[idx1]->GetScalarPointerForExtent(extent);

      // vtkDebugMacro("Execute: " << idx1 << "=>" << extent[0] << ", " << extent[1] << " / " << extent[2] << ", " << extent[3] << " / " << extent[4] << ", " << extent[5]);
      
      switch (this->BlendMode)
        {
        case VTK_IMAGE_BLEND_MODE_NORMAL:
          outPtr = outData->GetScalarPointerForExtent(extent);  
          // for performance reasons, use a special method for unsigned char
          if (inData[idx1]->GetScalarType() == VTK_UNSIGNED_CHAR)
            {
            vtkImageBlendExecuteChar(this, extent,
				     inData[idx1], (unsigned char *)(inPtr),
				     outData, (unsigned char *)(outPtr),
				     opacity, id);
            }
          else
            {
            switch (inData[idx1]->GetScalarType())
              {
              vtkTemplateMacro8(vtkImageBlendExecute, 
                                this, extent,
                                inData[idx1], (VTK_TT *)(inPtr),
                                outData, (VTK_TT *)(outPtr), 
                                opacity, id);
              default:
                vtkErrorMacro(<< "Execute: Unknown ScalarType");
                return;
              }
            }
          break;

        case VTK_IMAGE_BLEND_MODE_COMPOUND:
          switch (inData[idx1]->GetScalarType())
            {
            vtkTemplateMacro7(vtkImageBlendCompoundExecute, 
                              this, 
                              extent,
                              inData[idx1], 
                              (VTK_TT *)(inPtr), 
                              tmpData, 
                              opacity,
                              this->CompoundThreshold);
            default:
              vtkErrorMacro(<< "Execute: Unknown ScalarType");
              return;
            }
          break;

        default:
          vtkErrorMacro(<< "Execute: Unknown blending mode");
        }
      }
    }

  // conclude
  
  switch (this->BlendMode)
    {
    case VTK_IMAGE_BLEND_MODE_NORMAL:
      break;

    case VTK_IMAGE_BLEND_MODE_COMPOUND:
      outPtr = outData->GetScalarPointerForExtent(outExt);
      switch (outData->GetScalarType())
        {
        vtkTemplateMacro5(vtkImageBlendCompoundTransferExecute, 
                          this, 
                          outExt,
                          outData, 
                          (VTK_TT *)(outPtr), 
                          tmpData);
        default:
          vtkErrorMacro(<< "Execute: Unknown ScalarType");
          return;
        }
      tmpData->Delete();
      break;

    default:
      vtkErrorMacro(<< "Execute: Unknown blending mode");
    }
}


//----------------------------------------------------------------------------
void vtkImageBlend::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageMultipleInputFilter::PrintSelf(os, indent);
  int i;
  for (i = 0; i < this->GetNumberOfInputs(); i++)
    {
    os << indent << "Opacity(" << i << "): " << this->GetOpacity(i) << endl; 
    }
  os << indent << "Stencil: " << this->GetStencil() << endl;
  os << indent << "Blend Mode: " << this->GetBlendModeAsString() << endl
     << indent << "Compound threshold: " << this->CompoundThreshold << endl;
}

