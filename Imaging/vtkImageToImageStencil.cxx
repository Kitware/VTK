/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageToImageStencil.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G Gobbi who developed this class.

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
#include "vtkImageToImageStencil.h"
#include "vtkPolyData.h"
#include "vtkOBBTree.h"
#include "vtkObjectFactory.h"


//----------------------------------------------------------------------------
vtkImageToImageStencil* vtkImageToImageStencil::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageToImageStencil");
  if(ret)
    {
    return (vtkImageToImageStencil*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageToImageStencil;
}

//----------------------------------------------------------------------------
vtkImageToImageStencil::vtkImageToImageStencil()
{
  this->UpperThreshold = VTK_LARGE_FLOAT;
  this->LowerThreshold = -VTK_LARGE_FLOAT;
}

//----------------------------------------------------------------------------
vtkImageToImageStencil::~vtkImageToImageStencil()
{
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageStencilSource::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "UpperThreshold: " << this->UpperThreshold << "\n";
  os << indent << "LowerThreshold: " << this->LowerThreshold << "\n";
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::SetInput(vtkImageData *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkImageToImageStencil::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkImageData *)(this->Inputs[0]);
}

//----------------------------------------------------------------------------
// The values greater than or equal to the value match.
void vtkImageToImageStencil::ThresholdByUpper(float thresh)
{
  if (this->LowerThreshold != thresh || this->UpperThreshold < VTK_LARGE_FLOAT)
    {
    this->LowerThreshold = thresh;
    this->UpperThreshold = VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values less than or equal to the value match.
void vtkImageToImageStencil::ThresholdByLower(float thresh)
{
  if (this->UpperThreshold != thresh || this->LowerThreshold > -VTK_LARGE_FLOAT)
    {
    this->UpperThreshold = thresh;
    this->LowerThreshold = -VTK_LARGE_FLOAT;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
// The values in a range (inclusive) match
void vtkImageToImageStencil::ThresholdBetween(float lower, float upper)
{
  if (this->LowerThreshold != lower || this->UpperThreshold != upper)
    {
    this->LowerThreshold = lower;
    this->UpperThreshold = upper;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageToImageStencil::ThreadedExecute(vtkImageStencilData *data,
					     int outExt[6], int id)
{
  vtkImageData *inData = this->GetInput();
  if (!inData)
    {
    return;
    }

  int *inExt = inData->GetExtent();
  int *inWholeExt = inData->GetWholeExtent();
  vtkScalars *inScalars = inData->GetPointData()->GetScalars();
  float upperThreshold = this->UpperThreshold;
  float lowerThreshold = this->LowerThreshold;

  // clip the extent with the image data extent
  int extent[6];
  for (int i = 0; i < 3; i++)
    {
    int lo = 2*i;
    extent[lo] = outExt[lo];
    if (extent[lo] < inWholeExt[lo])
      {
      extent[lo] = inWholeExt[lo];
      }
    int hi = 2*i + 1;
    extent[hi] = outExt[hi];
    if (extent[hi] > inWholeExt[hi])
      {
      extent[hi] = inWholeExt[hi];
      }
    if (extent[lo] > extent[hi])
      {
      return;
      }
    }

  // for keeping track of progress
  unsigned long count = 0;
  unsigned long target = (unsigned long)
    ((extent[5] - extent[4] + 1)*(extent[3] - extent[2] + 1)/50.0);
  target++;

  for (int idZ = extent[4]; idZ <= extent[5]; idZ++)
    {
    for (int idY = extent[2]; idY <= extent[3]; idY++)
      {
      if (id == 0)
	{ // update progress if we're the main thread
	if (count%target == 0) 
	  {
	  this->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      int state = 1; // inside or outside, start outside
      int r1 = extent[0];
      int r2 = extent[1];

      // index into scalar array
      int idS = ((inExt[1] - inExt[0] + 1)*
		 ((inExt[3] - inExt[2] + 1)*(idZ - inExt[4]) +
		  (idY - inExt[2])) + (extent[0] - inExt[0]));

      for (int idX = extent[0]; idX <= extent[1]; idX++)
	{
	int newstate = 1;
	float value = inScalars->GetScalar(idS++);
	if (value >= lowerThreshold && value <= upperThreshold)
	  {
          newstate = -1;
	  if (newstate != state)
	    { // sub extent starts
	    r1 = idX;
	    }
	  }
	else if (newstate != state)
	  { // sub extent ends
	  r2 = idX - 1;
	  data->InsertNextExtent(r1, r2, idY, idZ);
	  }
	state = newstate;
	} // for idX
      if (state < 0)
	{ // if inside at end, cap off the sub extent
	data->InsertNextExtent(r1, extent[1], idY, idZ);
	}
      } // for idY
    } // for idZ
}
