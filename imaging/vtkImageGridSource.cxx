/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageGridSource.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to David G. Gobbi who developed this class.

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

#include "vtkImageGridSource.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkImageGridSource* vtkImageGridSource::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageGridSource");
  if(ret)
    {
    return (vtkImageGridSource*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageGridSource;
}

//----------------------------------------------------------------------------
vtkImageGridSource::vtkImageGridSource()
{
  this->DataExtent[0] = 0;  this->DataExtent[1] = 255;
  this->DataExtent[2] = 0;  this->DataExtent[3] = 255;
  this->DataExtent[4] = 0;  this->DataExtent[5] = 0;

  this->GridSpacing[0] = 10;
  this->GridSpacing[1] = 10;
  this->GridSpacing[2] = 0;

  this->GridOrigin[0] = 0;
  this->GridOrigin[1] = 0;
  this->GridOrigin[2] = 0;

  this->DataScalarType = VTK_FLOAT;

  this->DataOrigin[0] = this->DataOrigin[1] = this->DataOrigin[2] = 0.0;
  this->DataSpacing[0] = this->DataSpacing[1] = this->DataSpacing[2] = 1.0;

  this->LineValue = 1.0;
  this->FillValue = 0.0;
}

//----------------------------------------------------------------------------
void vtkImageGridSource::ExecuteInformation()
{
  vtkImageData *output = this->GetOutput();
  
  output->SetSpacing(this->DataSpacing);
  output->SetOrigin(this->DataOrigin);
  output->SetWholeExtent(this->DataExtent);
  output->SetScalarType(this->DataScalarType);
  output->SetNumberOfScalarComponents(1);
}

//----------------------------------------------------------------------------
template<class T>
static void vtkImageGridSourceExecute(vtkImageGridSource *self,
			       vtkImageData *data, T *outPtr,
			       int outExt[6], int id)
{
  int idxX, idxY, idxZ;
  int xval, yval, zval;
  int outIncX, outIncY, outIncZ;
  unsigned long count = 0;
  unsigned long target;
  
  int gridSpacing[3], gridOrigin[3];
  self->GetGridSpacing(gridSpacing);
  self->GetGridOrigin(gridOrigin); 

  T fillValue = T(self->GetFillValue());
  T lineValue = T(self->GetLineValue());

  // Get increments to march through data 
  data->GetContinuousIncrements(outExt, outIncX, outIncY, outIncZ);
  
  target = (unsigned long)((outExt[5]-outExt[4]+1)*
			   (outExt[3]-outExt[2]+1)/50.0);
  target++;

  // Loop through ouput pixel
  for (idxZ = outExt[4]; idxZ <= outExt[5]; idxZ++)
    {
    if (gridSpacing[2])
      {
      zval = (idxZ % gridSpacing[2] == gridOrigin[2]);
      }
    else
      {
      zval = 0;
      }
    for (idxY = outExt[2]; !self->GetAbortExecute() && idxY<=outExt[3]; idxY++)
      {
      if (gridSpacing[1])
	{
        yval = (idxY % gridSpacing[1] == gridOrigin[1]);
	}
      else
	{
        yval = 0;
	}
      if (id == 0)
	{
	if (!(count%target))
	  {
	  self->UpdateProgress(count/(50.0*target));
	  }
	count++;
	}

      if (gridSpacing[0])
	{
	for (idxX = outExt[0]; idxX <= outExt[1]; idxX++)
	  {
	  xval = (idxX % gridSpacing[0] == gridOrigin[0]); 

	  // not very efficient, but it gets the job done
	  *outPtr++ = ((zval|yval|xval) ? lineValue : fillValue);
	  }
	}
      else
	{
	for (idxX = outExt[0]; idxX <= outExt[1]; idxX++)
	  {
	  *outPtr++ = ((zval|yval) ? lineValue : fillValue);
	  }
	}
      outPtr += outIncY;
      }
    outPtr += outIncZ;
    }
}  

//----------------------------------------------------------------------------
void vtkImageGridSource::ExecuteData(vtkDataObject *output)
{
  vtkImageData *data = this->AllocateOutputData(output);
  int *outExt = data->GetExtent();
  void *outPtr = data->GetScalarPointerForExtent(outExt);
  
  // Call the correct templated function for the output
  switch (this->GetDataScalarType())
    {
    vtkTemplateMacro5(vtkImageGridSourceExecute, this, data,
		      (VTK_TT *)(outPtr), outExt, 0);
    default:
      vtkErrorMacro(<< "Execute: Unknown data type");
    }
}

//----------------------------------------------------------------------------
void vtkImageGridSource::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageSource::PrintSelf(os,indent);

  os << indent << "GridSpacing: (" << this->GridSpacing[0] << ", "
                                   << this->GridSpacing[1] << ", "
                                   << this->GridSpacing[2] << ")\n";
  os << indent << "GridOrigin: (" << this->GridOrigin[0] << ", "
                                  << this->GridOrigin[1] << ", "
                                  << this->GridOrigin[2] << ")\n";
  os << indent << "LineValue: " << this->LineValue << "\n"; 
  os << indent << "FillValue: " << this->FillValue << "\n";
  os << indent << "DataScalarType: " << 
    vtkImageScalarTypeNameMacro(this->DataScalarType) << "\n";
  os << indent << "DataExtent: ("  << this->DataExtent[0] << ", "
                                   << this->DataExtent[1] << ", "
                                   << this->DataExtent[2] << ", "
                                   << this->DataExtent[3] << ", "
                                   << this->DataExtent[4] << ", "
                                   << this->DataExtent[5] << ")\n";
  os << indent << "DataSpacing: (" << this->DataSpacing[0] << ", "
                                   << this->DataSpacing[1] << ", "
                                   << this->DataSpacing[2] << ")\n";
  os << indent << "DataOrigin: ("  << this->DataOrigin[0] << ", "
                                   << this->DataOrigin[1] << ", "
                                   << this->DataOrigin[2] << ")\n";
}


