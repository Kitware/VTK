/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageChangeInformation.cxx
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

#include "vtkImageChangeInformation.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkImageChangeInformation* vtkImageChangeInformation::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageChangeInformation");
  if(ret)
    {
    return (vtkImageChangeInformation*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageChangeInformation;
}

//----------------------------------------------------------------------------
vtkImageChangeInformation::vtkImageChangeInformation()
{
  this->InformationInput = NULL;
  this->CenterImage = 0;

  for (int i = 0; i < 3; i++)
    {
    this->OutputExtentStart[i] = VTK_INT_MAX;
    this->ExtentTranslation[i] = 0;
    this->FinalExtentTranslation[i] = VTK_INT_MAX;

    this->OutputSpacing[i] = VTK_FLOAT_MAX;
    this->SpacingScale[i] = 1.0f;

    this->OutputOrigin[i] = VTK_FLOAT_MAX;
    this->OriginScale[i] = 1.0f;
    this->OriginTranslation[i] = 0.0f;
    }
}

//----------------------------------------------------------------------------
vtkImageChangeInformation::~vtkImageChangeInformation()
{
  if (this->InformationInput)
    {
    this->SetInformationInput(NULL);
    }
}

//----------------------------------------------------------------------------
void vtkImageChangeInformation::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageToImageFilter::PrintSelf(os,indent);

  os << indent << "InformationInput: (" 
     << this->InformationInput << ")" << endl;

  os << indent << "CenterImage : "
     << (this->CenterImage ? "On":"Off") << endl;

  os << indent << "OutputExtentStart: (" 
     << this->OutputExtentStart[0] << ","
     << this->OutputExtentStart[1] << ","
     << this->OutputExtentStart[2] << endl;

  os << indent << "ExtentTranslation: (" 
     << this->ExtentTranslation[0] << ","
     << this->ExtentTranslation[1] << ","
     << this->ExtentTranslation[2] << endl;

  os << indent << "OutputSpacing: (" 
     << this->OutputSpacing[0] << ","
     << this->OutputSpacing[1] << ","
     << this->OutputSpacing[2] << endl;

  os << indent << "SpacingScale: (" 
     << this->SpacingScale[0] << ","
     << this->SpacingScale[1] << ","
     << this->SpacingScale[2] << endl;

  os << indent << "OutputOrigin: (" 
     << this->OutputOrigin[0] << ","
     << this->OutputOrigin[1] << ","
     << this->OutputOrigin[2] << endl;

  os << indent << "OriginScale: (" 
     << this->OriginScale[0] << ","
     << this->OriginScale[1] << ","
     << this->OriginScale[2] << endl;

  os << indent << "OriginTranslation: (" 
     << this->OriginTranslation[0] << ","
     << this->OriginTranslation[1] << ","
     << this->OriginTranslation[2] << endl;
}

//----------------------------------------------------------------------------
// Change the information
void vtkImageChangeInformation::ExecuteInformation(vtkImageData *inData, 
						   vtkImageData *outData)
{
  int i;
  int extent[6], inExtent[6];
  float spacing[3], origin[3];
  
  inData->GetWholeExtent(inExtent);

  if (this->InformationInput)
    {
    this->InformationInput->UpdateInformation();    
    this->InformationInput->GetOrigin(origin);
    this->InformationInput->GetSpacing(spacing);

    this->InformationInput->GetWholeExtent(extent);
    for (i = 0; i < 3; i++)
      {
      extent[2*i+1] = extent[2*i] - inExtent[2*i] + inExtent[2*i+1];
      }
    }
  else
    {
    inData->GetWholeExtent(extent);
    inData->GetOrigin(origin);
    inData->GetSpacing(spacing);
    }

  for (i = 0; i < 3; i++)
    {
    if (this->OutputSpacing[i] != VTK_FLOAT_MAX)
      {
      spacing[i] = this->OutputSpacing[i];
      }

    if (this->OutputOrigin[i] != VTK_FLOAT_MAX)
      {
      origin[i] = this->OutputOrigin[i];
      }

    if (this->OutputExtentStart[i] != VTK_INT_MAX)
      {
      extent[2*i+1] += this->OutputExtentStart[i] - extent[2*i];
      extent[2*i] = this->OutputExtentStart[i];
      }
    }

  if (this->CenterImage)
    {
    for (i = 0; i < 3; i++)
      {
      origin[i] = -(extent[2*i] + extent[2*i+1])*spacing[i]/2;
      }
    }

  for (i = 0; i < 3; i++)
    {
    spacing[i] = spacing[i]*this->SpacingScale[i];
    origin[i] = origin[i]*this->OriginScale[i] + this->OriginTranslation[i];
    extent[2*i] = extent[2*i] + this->ExtentTranslation[i];
    extent[2*i+1] = extent[2*i+1] + this->ExtentTranslation[i];
    this->FinalExtentTranslation[i] = extent[2*i] - inExtent[2*i];
    }

  outData->SetWholeExtent(extent);
  outData->SetSpacing(spacing);
  outData->SetOrigin(origin);
}


//----------------------------------------------------------------------------
// This method simply copies by reference the input data to the output.
void vtkImageChangeInformation::UpdateData(vtkDataObject *data)
{
  if (this->FinalExtentTranslation[0] == VTK_INT_MAX)
    {
    vtkErrorMacro("Bug in code.");
    return;
    }

  vtkImageData *inData = this->GetInput();
  vtkImageData *outData = (vtkImageData*)(data);
  int i;
  int extent[6];
  
  // Make sure the Input has been set.
  if (!inData)
    {
    vtkErrorMacro("Input is not set.");
    return;
    }

  outData->GetUpdateExtent(extent);
  for (i = 0; i < 3; ++i)
    {
    extent[i*2] -= this->FinalExtentTranslation[i];
    extent[i*2+1] -= this->FinalExtentTranslation[i];
    }  
  inData->SetUpdateExtent(extent);
  inData->Update();

  // since inData can be larger than update extent.
  inData->GetExtent(extent);
  for (i = 0; i < 3; ++i)
    {
    extent[i*2] += this->FinalExtentTranslation[i];
    extent[i*2+1] += this->FinalExtentTranslation[i];
    }
  outData->SetExtent(extent);
  outData->GetPointData()->PassData(inData->GetPointData());
  outData->DataHasBeenGenerated();
  
  // release input data
  if (inData->ShouldIReleaseData())
    {
    inData->ReleaseData();
    }
}

//----------------------------------------------------------------------------
void vtkImageChangeInformation::ComputeInputUpdateExtent(int inExt[6], 
							 int outExt[6])
{
  if (this->FinalExtentTranslation[0] == VTK_INT_MAX)
    {
    vtkErrorMacro("Bug in code.");
    return;
    }

  inExt[0] = outExt[0] - this->FinalExtentTranslation[0];
  inExt[1] = outExt[1] - this->FinalExtentTranslation[0];
  inExt[2] = outExt[2] - this->FinalExtentTranslation[1];
  inExt[3] = outExt[3] - this->FinalExtentTranslation[1];
  inExt[4] = outExt[4] - this->FinalExtentTranslation[2];
  inExt[5] = outExt[5] - this->FinalExtentTranslation[2];
}
