/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageBlend.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
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
  int i;
  this->Opacity = new double[10];
  this->OpacityArrayLength = 10;
  for (i = 0; i < this->OpacityArrayLength; i++)
    {
    this->Opacity[i] = 1.0;
    }
}

//----------------------------------------------------------------------------
vtkImageBlend::~vtkImageBlend()
{
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
  double *tmp;

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
    tmp = new double[this->OpacityArrayLength+10];
    for (i = 0; i < this->OpacityArrayLength; i++)
      {
      tmp[i] = this->Opacity[i];
      }
    this->OpacityArrayLength += 10;
    for (; i < this->OpacityArrayLength; i++)
      {
      tmp[i] = 1.0;
      }
    delete [] this->Opacity;
    this->Opacity = tmp;
    this->Opacity[idx] = 0.0;
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
  int i;
  double *tmp;
  if (idx >= this->OpacityArrayLength)
    {
    tmp = new double[this->OpacityArrayLength+10];
    for (i = 0; i < this->OpacityArrayLength; i++)
      {
      tmp[i] = this->Opacity[i];
      }
    this->OpacityArrayLength += 10;
    for (; i < this->OpacityArrayLength; i++)
      {
      tmp[i] = 1.0;
      }
    delete [] this->Opacity;
    this->Opacity = tmp;
    }
  return this->Opacity[idx];
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

  // Clip, just to make sure we hit _some_ of the input extent
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
void vtkImageBlend::UpdateData(vtkDataObject *outObject)
{
  int idx,singleInput;

  // check to see if we have more than one input
  singleInput = 1;
  for (idx = 1; idx < this->NumberOfInputs; idx++)
    {
    if (this->GetInput(idx) != NULL)
      {
      singleInput = 0;
      }
    }
  if (singleInput)
    {
    vtkDebugMacro("InternalUpdate: single input, passing data");

    vtkImageData *outData = (vtkImageData *)(outObject);
    vtkImageData *inData = this->GetInput();
 
    // Make sure the Input has been set.
    if ( ! inData)
      {
      vtkErrorMacro(<< "Input is not set.");
      return;
      }

    inData->SetUpdateExtent(outData->GetUpdateExtent());
    inData->Update();
    outData->SetExtent(inData->GetExtent());
    outData->GetPointData()->PassData(inData->GetPointData());
    outData->DataHasBeenGenerated();
    }
  else // multiple inputs
    {
    this->vtkImageMultipleInputFilter::UpdateData(outObject);
    }
}

//----------------------------------------------------------------------------
// This templated function executes the filter for any type of data.
template <class T>
static void vtkImageBlendExecute(vtkImageBlend *self, int id, 
			  int inExt[6], vtkImageData *inData, T *inPtr,
			  int vtkNotUsed(outExt)[6], vtkImageData *outData, 
			  T *outPtr, float opacity)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int inC, outC;
  unsigned long count = 0;
  unsigned long target;
  float minA,maxA;
  float r,f;

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

  // find the region to loop over
  maxX = inExt[1] - inExt[0] + 1; 
  maxY = inExt[3] - inExt[2] + 1; 
  maxZ = inExt[5] - inExt[4] + 1;

  target = (unsigned long)((maxZ*maxY)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(inExt, outIncX, outIncY, outIncZ);

  // Loop through output pixels
  for (idxZ = 0; idxZ < maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY < maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      if (outC >= 3 && inC >= 4)
	{ // RGB(A) blended with RGBA
	for (idxX = 0; idxX < maxX; idxX++)
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
      else if (outC >= 3 && inC == 3)
	{ // RGB(A) blended with RGB
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  outPtr[0] = T(outPtr[0]*f + inPtr[0]*r);
	  outPtr[1] = T(outPtr[1]*f + inPtr[1]*r);
	  outPtr[2] = T(outPtr[2]*f + inPtr[2]*r);
	  outPtr += outC; 
	  inPtr += inC;
	  }
	}
      else if (outC >= 3 && inC == 2)
	{ // RGB(A) blended with luminance+alpha
	for (idxX = 0; idxX < maxX; idxX++)
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
      else if (outC >= 3 && inC == 1)
	{ // RGB(A) blended with luminance
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  outPtr[0] = T(outPtr[0]*f + (*inPtr)*r);
	  outPtr[1] = T(outPtr[1]*f + (*inPtr)*r);
	  outPtr[2] = T(outPtr[2]*f + (*inPtr)*r);
	  outPtr += outC; 
	  inPtr++;
	  }
	}
      else if (inC == 2)
	{ // luminance(+alpha) blended with luminance+alpha
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  r = opacity*(inPtr[1]-minA);
	  f = 1.0-r;
	  *outPtr = T((*outPtr)*f + (*inPtr)*r);
	  outPtr += outC; 
	  inPtr += 2;
	  }
	}
      else
	{ // luminance(+alpha) blended with luminance
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  *outPtr = T((*outPtr)*f + (*inPtr)*r);
	  outPtr += outC; 
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
// This templated function executes the filter specifically for char data
template <class T>
static void vtkImageBlendExecuteChar(vtkImageBlend *self, int id, 
			  int inExt[6], vtkImageData *inData, T *inPtr,
			  int vtkNotUsed(outExt)[6], vtkImageData *outData, 
                          T *outPtr, float opacity)
{
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int inIncX, inIncY, inIncZ;
  int outIncX, outIncY, outIncZ;
  int inC, outC;
  unsigned short r,f;
  unsigned long count = 0;
  unsigned long target;

  r = (unsigned short)(255*opacity);
  f = 255-r;

  inC = inData->GetNumberOfScalarComponents();
  outC = outData->GetNumberOfScalarComponents();

  // find the region to loop over
  maxX = inExt[1] - inExt[0] + 1; 
  maxY = inExt[3] - inExt[2] + 1; 
  maxZ = inExt[5] - inExt[4] + 1;

  target = (unsigned long)((maxZ*maxY)/50.0);
  target++;
  
  // Get increments to march through data 
  inData->GetContinuousIncrements(inExt, inIncX, inIncY, inIncZ);
  outData->GetContinuousIncrements(inExt, outIncX, outIncY, outIncZ);

   // Loop through ouput pixels
  for (idxZ = 0; idxZ < maxZ; idxZ++)
    {
    for (idxY = 0; !self->AbortExecute && idxY < maxY; idxY++)
      {
      if (!id) 
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}
      if (outC >= 3 && inC >= 4)
	{ // RGB(A) blended with RGBA
	for (idxX = 0; idxX < maxX; idxX++)
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
      else if (outC >= 3 && inC == 3)
	{ // RGB(A) blended with RGB
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  outPtr[0] = (outPtr[0]*f + inPtr[0]*r) >> 8;
	  outPtr[1] = (outPtr[1]*f + inPtr[1]*r) >> 8;
	  outPtr[2] = (outPtr[2]*f + inPtr[2]*r) >> 8;
	  outPtr += outC; 
	  inPtr += inC;
	  }
	}
      else if (outC >= 3 && inC == 2)
	{ // RGB(A) blended with luminance+alpha
	for (idxX = 0; idxX < maxX; idxX++)
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
      else if (outC >= 3 && inC == 1)
	{ // RGB(A) blended with luminance
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  outPtr[0] = (outPtr[0]*f + (*inPtr)*r) >> 8;
	  outPtr[1] = (outPtr[1]*f + (*inPtr)*r) >> 8;
	  outPtr[2] = (outPtr[2]*f + (*inPtr)*r) >> 8;
	  outPtr += outC; 
	  inPtr++;
	  }
	}
      else if (inC == 2)
	{ // luminance(+alpha) blended with luminance+alpha
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  r = (unsigned short)(inPtr[1]*opacity);
	  f = 255-r;
	  *outPtr = ((*outPtr)*f + (*inPtr)*r) >> 8;
	  outPtr += outC; 
	  inPtr += 2;
	  }
	}
      else
	{ // luminance(+alpha) blended with luminance
	for (idxX = 0; idxX < maxX; idxX++)
	  {
	  *outPtr = ((*outPtr)*f + (*inPtr)*r) >> 8;
	  outPtr += outC; 
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
// This method is passed a input and output regions, and executes the filter
// algorithm to fill the output from the inputs.
// It just executes a switch statement to call the correct function for
// the regions data types.
void vtkImageBlend::ThreadedExecute(vtkImageData **inData, 
				    vtkImageData *outData,
				    int outExt[6], int id)
{
  int idx1,i,skip;
  int inExt[6];
  void *inPtr;
  void *outPtr;
  float opacity;

  if (inData[0]->GetNumberOfScalarComponents() > 4)
    {
    vtkErrorMacro("The first input can have a maximum of four components");
    return;
    }

  // copy the first image directly to the output
  vtkDebugMacro("Execute: copy input 0 to the output.");
  vtkImageBlendCopyData(inData[0], outData, outExt);
  
  for (idx1 = 1; idx1 < this->NumberOfInputs; ++idx1)
    {
    if (inData[idx1] != NULL)
      {
      this->ComputeInputUpdateExtent(inExt, outExt, idx1);
      if ((inData[idx1]->GetNumberOfScalarComponents()+1)/2 == 2 &&
	  (inData[0]->GetNumberOfScalarComponents()+1)/2 == 1)
	{
	vtkErrorMacro("input has too many components, can't blend RGB data into greyscale data");
	return;
	}
	
      // this filter expects that input is the same type as output.
      if (inData[idx1]->GetScalarType() != outData->GetScalarType())
	{
	vtkErrorMacro(<< "Execute: input" << idx1 << " ScalarType (" << 
	inData[idx1]->GetScalarType() << 
	"), must match output ScalarType (" << outData->GetScalarType() 
	<< ")");
	return;
	}

      skip = 0;
      for (i = 0; i < 3; i++)
	{
	if (outExt[2*i+1] < inExt[2*i] || outExt[2*i] > inExt[2*i+1])
	  {
	  skip = 1; // extents don't overlap, skip this input
	  }
	}
      if (skip) 
	{
	vtkDebugMacro("Execute: skipping input.");
	continue;
	}

      opacity = this->GetOpacity(idx1);

      inPtr = inData[idx1]->GetScalarPointerForExtent(inExt);
      outPtr = outData->GetScalarPointerForExtent(inExt);  

      switch (inData[idx1]->GetScalarType())
	{
	case VTK_DOUBLE:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (double *)(inPtr),
			       outExt, outData, (double *)(outPtr),
			       opacity);
	  break;
	case VTK_FLOAT:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (float *)(inPtr),
			       outExt, outData, (float *)(outPtr),
			       opacity);
	  break;
	case VTK_UNSIGNED_LONG:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (unsigned long *)(inPtr),
			       outExt,outData,(unsigned long *)(outPtr),
			       opacity);
	  break;
	case VTK_LONG:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (long *)(inPtr),
			       outExt, outData, (long *)(outPtr),
			       opacity);
	  break;
	case VTK_UNSIGNED_INT:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (unsigned int *)(inPtr),
			       outExt, outData, (unsigned int *)(outPtr),
			       opacity);
	  break;
	case VTK_INT:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (int *)(inPtr),
			       outExt, outData, (int *)(outPtr),
			       opacity);
	  break;
	case VTK_SHORT:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (short *)(inPtr),
			       outExt, outData, (short *)(outPtr),
			       opacity);
	  break;
	case VTK_UNSIGNED_SHORT:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (unsigned short *)(inPtr),
			       outExt,outData,(unsigned short *)(outPtr),
			       opacity);
	  break;	
	case VTK_CHAR:
	  vtkImageBlendExecute(this, id, inExt,
			       inData[idx1], (char *)(inPtr),
			       outExt,outData,(char *)(outPtr),
			       opacity);
	  break;	
	case VTK_UNSIGNED_CHAR:
	  vtkImageBlendExecuteChar(this, id, inExt,
				   inData[idx1], (unsigned char *)(inPtr),
				   outExt,outData,(unsigned char *)(outPtr),
				   opacity);
	  break;
	default:
	  vtkErrorMacro(<< "Execute: Unknown ScalarType");
	  return;
	}
      }
    }
}



//----------------------------------------------------------------------------
void vtkImageBlend::PrintSelf(ostream& os, vtkIndent indent)
{
  int i;
  for (i = 0; i < this->GetNumberOfInputs(); i++)
    {
    os << indent << "Opacity(" << i << "): " << this->GetOpacity(i) << "\n"; 
    }
  this->vtkImageMultipleInputFilter::PrintSelf(os, indent);
}














