/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBooleanTexture.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
#include "vtkBooleanTexture.h"
#include "vtkUnsignedCharArray.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkBooleanTexture* vtkBooleanTexture::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkBooleanTexture");
  if(ret)
    {
    return (vtkBooleanTexture*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkBooleanTexture;
}




vtkBooleanTexture::vtkBooleanTexture()
{
  this->Thickness = 0;

  this->XSize = this->YSize = 12;

  this->InIn[0] = this->InIn[1] = 255;
  this->InOut[0] = this->InOut[1] = 255;
  this->OutIn[0] = this->OutIn[1] = 255;
  this->OutOut[0] = this->OutOut[1] = 255;
  this->OnOn[0] = this->OnOn[1] = 255;
  this->OnIn[0] = this->OnIn[1] = 255;
  this->OnOut[0] = this->OnOut[1] = 255;
  this->InOn[0] = this->InOn[1] = 255;
  this->OutOn[0] = this->OutOn[1] = 255;
}

void vtkBooleanTexture::Execute()
{
  int numPts, i, j;
  vtkUnsignedCharArray *newScalars;
  int midILower, midJLower, midIUpper, midJUpper;
  vtkStructuredPoints *output = this->GetOutput();
  
  if ( (numPts = this->XSize * this->YSize) < 1 )
    {
    vtkErrorMacro(<<"Bad texture (xsize,ysize) specification!");
    return;
    }

  output->SetDimensions(this->XSize,this->YSize,1);
  newScalars = vtkUnsignedCharArray::New();
  newScalars->SetNumberOfComponents(2);
  newScalars->Allocate(numPts);
//
// Compute size of various regions
//
  midILower = (int) ((float)(this->XSize - 1) / 2.0 - this->Thickness / 2.0);
  midJLower = (int) ((float)(this->YSize - 1) / 2.0 - this->Thickness / 2.0);
  midIUpper = (int) ((float)(this->XSize - 1) / 2.0 + this->Thickness / 2.0);
  midJUpper = (int) ((float)(this->YSize - 1) / 2.0 + this->Thickness / 2.0);
//
// Create texture map
//
  for (j = 0; j < this->YSize; j++) 
    {
    for (i = 0; i < this->XSize; i++) 
      {
      if (i < midILower && j < midJLower) 
	{
        newScalars->InsertNextValue(this->InIn[0]);
        newScalars->InsertNextValue(this->InIn[1]);
	}
      else if (i > midIUpper && j < midJLower) 
	{
        newScalars->InsertNextValue(this->OutIn[0]);
        newScalars->InsertNextValue(this->OutIn[1]);
	}
      else if (i < midILower && j > midJUpper)
	{
        newScalars->InsertNextValue(this->InOut[0]);
        newScalars->InsertNextValue(this->InOut[1]);
	}
      else if (i > midIUpper && j > midJUpper)
	{
        newScalars->InsertNextValue(this->OutOut[0]);
        newScalars->InsertNextValue(this->OutOut[1]);
	}
      else if ((i >= midILower && i <= midIUpper) && (j >= midJLower && j <= midJUpper))
	{
        newScalars->InsertNextValue(this->OnOn[0]);
        newScalars->InsertNextValue(this->OnOn[1]);
	}
      else if ((i >= midILower && i <= midIUpper) && j < midJLower)
	{
        newScalars->InsertNextValue(this->OnIn[0]);
        newScalars->InsertNextValue(this->OnIn[1]);
	}
      else if ((i >= midILower && i <= midIUpper) && j > midJUpper)
	{
        newScalars->InsertNextValue(this->OnOut[0]);
        newScalars->InsertNextValue(this->OnOut[1]);
	}
      else if (i < midILower && (j >= midJLower && j <= midJUpper))
	{
        newScalars->InsertNextValue(this->InOn[0]);
        newScalars->InsertNextValue(this->InOn[1]);
	}
      else if (i > midIUpper && (j >= midJLower && j <= midJUpper))
	{
        newScalars->InsertNextValue(this->OutOn[0]);
        newScalars->InsertNextValue(this->OutOn[1]);
	}
      }
    }

//
// Update ourselves
//
  output->GetPointData()->SetScalars(newScalars);
  newScalars->Delete();
}

void vtkBooleanTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkStructuredPointsSource::PrintSelf(os,indent);

  os << indent << "X Size: " << this->XSize << "\n";
  os << indent << "Y Size: " << this->YSize << "\n";

  os << indent << "Thickness: " << this->Thickness << "\n";
  os << indent << "In/In: (" << this->InIn[0] << "," << this->InIn[1] << ")\n";
  os << indent << "In/Out: (" << this->InOut[0] << "," << this->InOut[1] << ")\n";
  os << indent << "Out/In: (" << this->OutIn[0] << "," << this->OutIn[1] << ")\n";
  os << indent << "Out/Out: (" << this->OutOut[0] << "," << this->OutOut[1] << ")\n";
  os << indent << "On/On: (" << this->OnOn[0] << "," << this->OnOn[1] << ")\n";
  os << indent << "On/In: (" << this->OnIn[0] << "," << this->OnIn[1] << ")\n";
  os << indent << "On/Out: (" << this->OnOut[0] << "," << this->OnOut[1] << ")\n";
  os << indent << "In/On: (" << this->InOn[0] << "," << this->InOn[1] << ")\n";
  os << indent << "Out/On: (" << this->OutOn[0] << "," << this->OutOn[1] << ")\n";
}

