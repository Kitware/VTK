/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectWriter.cxx
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
#include "vtkDataObjectWriter.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkDataObjectWriter* vtkDataObjectWriter::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkDataObjectWriter");
  if(ret)
    {
    return (vtkDataObjectWriter*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkDataObjectWriter;
}




vtkDataObjectWriter::vtkDataObjectWriter()
{
  this->Writer = vtkDataWriter::New();
}

vtkDataObjectWriter::~vtkDataObjectWriter()
{
  this->Writer->Delete();
}

//----------------------------------------------------------------------------
void vtkDataObjectWriter::SetInput(vtkDataObject *input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkDataObject *vtkDataObjectWriter::GetInput()
{
  if (this->NumberOfInputs < 1)
    {
    return NULL;
    }
  
  return (vtkDataObject *)(this->Inputs[0]);
}

// Write FieldData data to file
void vtkDataObjectWriter::WriteData()
{
  ostream *fp;
  vtkFieldData *f=this->GetInput()->GetFieldData();

  vtkDebugMacro(<<"Writing vtk FieldData data...");

  if ( !(fp=this->Writer->OpenVTKFile()) || !this->Writer->WriteHeader(fp) )
    {
    return;
    }
  //
  // Write FieldData data specific stuff
  //
  this->Writer->WriteFieldData(fp, f);
  
  this->Writer->CloseVTKFile(fp);  
}

void vtkDataObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkWriter::PrintSelf(os,indent);
  
  os << indent << "File Name: " 
     << (this->Writer->GetFileName() ? this->Writer->GetFileName() : "(none)") << "\n";

  if ( this->Writer->GetFileType() == VTK_BINARY )
    {
    os << indent << "File Type: BINARY\n";
    }
  else
    {
    os << indent << "File Type: ASCII\n";
    }

  if ( this->Writer->GetHeader() )
    {
    os << indent << "Header: " << this->Writer->GetHeader() << "\n";
    }
  else
    {
    os << indent << "Header: (None)\n";
    }

  if ( this->Writer->GetFieldDataName() )
    {
    os << indent << "Field Data Name: " << this->Writer->GetFieldDataName() << "\n";
    }
  else
    {
    os << indent << "Field Data Name: (None)\n";
    }

}

