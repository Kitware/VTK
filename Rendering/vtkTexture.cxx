/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTexture.cxx
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
#include <stdlib.h>
#include "vtkTexture.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkGraphicsFactory.h"

// Construct object and initialize.
vtkTexture::vtkTexture()
{
  this->Repeat = 1;
  this->Interpolate = 0;
  this->Quality = VTK_TEXTURE_QUALITY_DEFAULT;

  this->LookupTable = NULL;
  this->MappedScalars = NULL;
  this->MapColorScalarsThroughLookupTable = 0;

  this->SelfAdjustingTableRange = 0;
}

vtkTexture::~vtkTexture()
{
  if (this->MappedScalars)
    {
    this->MappedScalars->Delete();
    }

  if (this->LookupTable != NULL) 
    {
    this->LookupTable->UnRegister(this);
    }

  this->SetInput(NULL);
}

// return the correct type of Texture 
vtkTexture *vtkTexture::New()
{  
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkGraphicsFactory::CreateInstance("vtkTexture");
  return (vtkTexture*)ret;
}

vtkImageData *vtkTexture::GetInput()
{
   if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return vtkImageData::SafeDownCast(this->Inputs[0]); 
}

void vtkTexture::SetInput( vtkImageData *input )
{
  this->vtkProcessObject::SetNthInput(0, input);
}

void vtkTexture::SetLookupTable(vtkLookupTable *lut)
{
  if ( this->LookupTable != lut ) 
    {
    if ( this->LookupTable != NULL ) {this->LookupTable->UnRegister(this);}
    this->LookupTable = lut;
    if ( this->LookupTable != NULL ) {this->LookupTable->Register(this);}
    this->Modified();
    }
}

void vtkTexture::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProcessObject::PrintSelf(os,indent);

  os << indent << "Interpolate: " << (this->Interpolate ? "On\n" : "Off\n");
  os << indent << "Repeat:      " << (this->Repeat ? "On\n" : "Off\n");
  os << indent << "Quality:     ";
  switch (this->Quality)
    {
    case VTK_TEXTURE_QUALITY_DEFAULT:
      os << "Default\n";
      break;
    case VTK_TEXTURE_QUALITY_16BIT:
      os << "16Bit\n";
      break;
    case VTK_TEXTURE_QUALITY_32BIT:
      os << "32Bit\n";
      break;
    }
  os << indent << "MapColorScalarsThroughLookupTable: " << (this->MapColorScalarsThroughLookupTable  ? "On\n" : "Off\n");

  if ( this->GetInput() )
    {
    os << indent << "Input: (" << (void *)this->GetInput() << ")\n";
    }
  else
    {
    os << indent << "Input: (none)\n";
    }
  if ( this->LookupTable )
    {
    os << indent << "LookupTable:\n";
    this->LookupTable->PrintSelf (os, indent.GetNextIndent ());
    }
  else
    {
    os << indent << "LookupTable: (none)\n";
    }

  if ( this->MappedScalars )
    {
    os << indent << "Mapped Scalars: " << this->MappedScalars << "\n";
    }
  else
    {
    os << indent << "Mapped Scalars: (none)\n";
    }
}

unsigned char *vtkTexture::MapScalarsToColors (vtkScalars *scalars)
{
  int numPts = scalars->GetNumberOfScalars ();
  vtkScalars *mappedScalars;

  // if there is no lookup table, create one
  if (this->LookupTable == NULL)
    {
    this->LookupTable = vtkLookupTable::New();
    this->LookupTable->Register(this);
    this->LookupTable->Delete();
    this->LookupTable->Build ();
    this->SelfAdjustingTableRange = 1;
    }
  else
    {
    this->SelfAdjustingTableRange = 0;
    }
  // if there is no pixmap, create one
  if (!this->MappedScalars)
    {
    this->MappedScalars = vtkScalars::New(VTK_UNSIGNED_CHAR,4);
    }      
  
  // if the texture created its own lookup table, set the Table Range
  // to the range of the scalar data.
  if (this->SelfAdjustingTableRange)
    {
    this->LookupTable->SetTableRange (scalars->GetRange());
    }
  
  // map the scalars to colors
  mappedScalars = this->MappedScalars;
  mappedScalars->SetNumberOfScalars(numPts);
  unsigned char *cptr = (unsigned char *)mappedScalars->GetVoidPointer(0);

  this->LookupTable->MapScalarsThroughTable(scalars,cptr);
  
  return cptr;
}

void vtkTexture::Render(vtkRenderer *ren)
{
  vtkImageData *input = this->GetInput();
  
  if (input) //load texture map
    {
    // We do not want more than requested.
    input->RequestExactExtentOn();
    
    // Updating the whole extent may not be necessary.
    input->UpdateInformation();
    input->SetUpdateExtentToWholeExtent();
    input->Update();
    this->Load(ren);
    }
}







