/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSinusoidSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include <math.h>
#include "vtkImageCache.h"
#include "vtkImageSinusoidSource.h"

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

void vtkImageSinusoidSource::SetDirection(float *v)
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
void vtkImageSinusoidSource::UpdateImageInformation()
{
  this->CheckCache();
  this->Output->SetWholeExtent(this->WholeExtent);
  this->Output->SetScalarType(VTK_FLOAT);
  this->Output->SetNumberOfScalarComponents(1);
}

void vtkImageSinusoidSource::Execute(vtkImageData *data)
{
  float *outPtr;
  int idxX, idxY, idxZ;
  int maxX, maxY, maxZ;
  int outIncX, outIncY, outIncZ;
  int *outExt;
  float sum;
  float yContrib, zContrib;
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
    zContrib = this->Direction[2] - (idxZ + outExt[4]);
    for (idxY = 0; !this->AbortExecute && idxY <= maxY; idxY++)
      {
      if (!(count%target))
	{
	this->UpdateProgress(count/(50.0*target));
	}
      count++;
      yContrib = this->Direction[1] - (idxY + outExt[2]);
      for (idxX = 0; idxX <= maxX; idxX++)
	{
	// find dot product
	sum = zContrib + yContrib;
	sum = sum + (float)(idxX + outExt[0]) * this->Direction[0];
	
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

