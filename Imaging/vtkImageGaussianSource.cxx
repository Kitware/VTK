/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGaussianSource.cxx
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

#include "vtkImageGaussianSource.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageGaussianSource* vtkImageGaussianSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageGaussianSource");
  if(ret)
    {
    return (vtkImageGaussianSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageGaussianSource;
}




//----------------------------------------------------------------------------
vtkImageGaussianSource::vtkImageGaussianSource()
{
  this->Maximum = 10.0;
  this->Center[0] = 0.0;
  this->Center[1] = 0.0;
  this->Center[2] = 0.0;

  this->WholeExtent[0] = 0;  this->WholeExtent[1] = 255;
  this->WholeExtent[2] = 0;  this->WholeExtent[3] = 255;
  this->WholeExtent[4] = 0;  this->WholeExtent[5] = 0;
  this->StandardDeviation = 100.0;
}


//----------------------------------------------------------------------------
void vtkImageGaussianSource::SetWholeExtent(int xMin, int xMax, 
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
void vtkImageGaussianSource::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  
  output->SetWholeExtent(this->WholeExtent);
  output->SetScalarType(VTK_FLOAT);
  output->SetNumberOfScalarComponents(1);
}

void vtkImageGaussianSource::ExecuteData(vtkDataObject *output)
{
  float *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int outIncX, outIncY, outIncZ;
  int *outExt;
  float sum;
  float yContrib, zContrib;
  float temp, temp2;
  unsigned long count = 0;
  unsigned long target;
  
  vtkImageData *data = this->AllocateOutputData(output);

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
  temp2 = 1.0 / (2.0 * this->StandardDeviation * this->StandardDeviation);

  for (idxZ = 0; idxZ <= maxZ; idxZ++)
    {
    zContrib = this->Center[2] - (idxZ + outExt[4]);
    zContrib = zContrib*zContrib;
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count%target))
	{
	this->UpdateProgress(count/(50.0*target));
	}
      count++;
      yContrib = this->Center[1] - (idxY + outExt[2]);
      yContrib = yContrib*yContrib;
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// Pixel operation
	sum = zContrib + yContrib;
	temp = this->Center[0] - (idxX + outExt[0]);
	sum = sum + (temp * temp);
	*outPtr = this->Maximum * exp(-sum * temp2);
	outPtr++;
	}
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}

void vtkImageGaussianSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "Maximum: " << this->Maximum << "\n";

  os << indent << "StandardDeviation: " << this->StandardDeviation << "\n";

  os << indent << "Center: ( "
     << this->Center[0] << ", "
     << this->Center[1] << ", "
     << this->Center[2] << " )\n";

}


