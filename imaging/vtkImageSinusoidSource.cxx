/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.cxx
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

#include "vtkImageSinusoidSource.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageSinusoidSource* vtkImageSinusoidSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageSinusoidSource");
  if(ret)
    {
    return (vtkImageSinusoidSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageSinusoidSource;
}




//----------------------------------------------------------------------------
vtkImageSinusoidSource::vtkImageSinusoidSource()
{
  this->Direction[0] = 1.0;
  this->Direction[1] = 0.0;
  this->Direction[2] = 0.0;
  
  this->Amplitude = 255.0;
  this->Phase = 0.0;
  this->Period = 20.0;

  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;
  
}

void vtkImageSinusoidSource::SetDirection(float v[3])
{
  this->SetDirection(v[0],v[1],v[2]);
}

void vtkImageSinusoidSource::SetDirection(float v0, float v1, float v2)
{
  float sum;

  sum = v0*v0 + v1*v1 + v2*v2;

  if (sum == 0.0)
    {
    vtkErrorMacro("Zero direction vector");
    return;
    }
  
  // normalize
  sum = 1.0 / sqrt(sum);
  v0 *= sum;
  v1 *= sum;
  v2 *= sum;
  
  if (this->Direction[0] == v0 && this->Direction[1] == v1 
      && this->Direction[2] == v2)
    {
    return;
    }
  
  this->Direction[0] = v0;
  this->Direction[1] = v1;
  this->Direction[2] = v2;
  
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::SetWholeExtent(int xMin, int xMax, 
					    int yMin, int yMax,
					    int zMin, int zMax)
{
  int modified = 0;
  
  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin ;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax ;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin ;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax ;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin ;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax ;
    }
  if (modified)
    {
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkImageSinusoidSource::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();

  output->SetWholeExtent(this->WholeExtent);
  output->SetScalarType(VTK_FLOAT);
  output->SetNumberOfScalarComponents(1);
}

void vtkImageSinusoidSource::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  float *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int outIncX, outIncY, outIncZ;
  int *outExt;
  float sum;
  float yContrib, zContrib, xContrib;
  unsigned long count = 0;
  unsigned long target;
  
  if (data->GetScalarType() != VTK_FLOAT)
    {
    vtkErrorMacro("Execute: This source only outputs floats");
    }
  
  outExt = data->GetExtent();
  
  // find the region to loop over
  maxX = outExt[1] - outExt[0];
  maxY = outExt[3] - outExt[2]; 
  maxZ = outExt[5] - outExt[4];
  
  // Get increments to march through data 
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  outPtr = (float *) data->GetScalarPointer(outExt[0],outExt[2],outExt[4]);

  target = (unsigned long)((maxZ+1)*(maxY+1)/50.0);
  target++;

  // Loop through ouput pixels
  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    zContrib = this->Direction[2] * (idxZ + outExt[4]);
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count%target))
	{
	this->UpdateProgress(count/(50.0*target));
	}
      count++;
      yContrib = this->Direction[1] * (idxY + outExt[2]);
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	xContrib = this->Direction[0] * (float)(idxX + outExt[0]);
	// find dot product
	sum = zContrib + yContrib + xContrib;
	
	*outPtr = this->Amplitude * 
	  cos((6.2831853 * sum / this->Period) - this->Phase);
    	outPtr++;
	}
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}

void vtkImageSinusoidSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "Period: " << this->Period << "\n";
  os << indent << "Phase: " << this->Phase << "\n";
  os << indent << "Amplitude: " << this->Amplitude << "\n";
  os << indent << "Direction: ( "
     << this->Direction[0] << ", "
     << this->Direction[1] << ", "
     << this->Direction[2] << " )\n";

}

