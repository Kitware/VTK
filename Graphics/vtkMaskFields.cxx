/*=========================================================================

Program:   Visualization Toolkit
Module:    vtkMaskFields.cxx
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
#include "vtkMaskFields.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"

vtkCxxRevisionMacro(vtkMaskFields, "1.1");
vtkStandardNewMacro(vtkMaskFields);

vtkMaskFields::vtkMaskFields()
{

  this->CopyFieldFlags = 0;
  this->NumberOfFieldFlags = 0;
  this->CopyAllOn();

}

vtkMaskFields::~vtkMaskFields()
{

  this->ClearFieldFlags();

}

void vtkMaskFields::CopyFieldOnOff(int fieldLocation,
                                   const char* field, int onOff)
{
  if (!field) { return; }
  
  int index;
  // If the array is in the list, simply set IsCopied to onOff
  if ((index=this->FindFlag(field, fieldLocation)) != -1)
    {
      this->CopyFieldFlags[index].IsCopied = onOff;
    }
  else
    {
      // We need to reallocate the list of fields
      vtkMaskFields::CopyFieldFlag* newFlags =
        new vtkMaskFields::CopyFieldFlag[this->NumberOfFieldFlags+1];
      // Copy old flags (pointer copy for name)
      for(int i=0; i<this->NumberOfFieldFlags; i++)
        {
          newFlags[i].Name = this->CopyFieldFlags[i].Name;
          newFlags[i].Type = this->CopyFieldFlags[i].Type;
          newFlags[i].Location = this->CopyFieldFlags[i].Location;
          newFlags[i].IsCopied = this->CopyFieldFlags[i].IsCopied;
        }
      // Copy new flag (strcpy)
      char* newName = new char[strlen(field)+1];
      strcpy(newName, field);
      newFlags[this->NumberOfFieldFlags].Name = newName;
      newFlags[this->NumberOfFieldFlags].Type = -1;
      newFlags[this->NumberOfFieldFlags].Location = fieldLocation;
      newFlags[this->NumberOfFieldFlags].IsCopied = onOff;
      this->NumberOfFieldFlags++;
      delete[] this->CopyFieldFlags;
      this->CopyFieldFlags = newFlags;
    }
  this->Modified();
}

void vtkMaskFields::CopyAttributeOnOff(int attributeLocation,
                                       int attributeType, int onOff)
{
  
  int index;
  // If the array is in the list, simply set IsCopied to onOff
  if ((index=this->FindFlag(attributeType, attributeLocation)) != -1)
    {
      this->CopyFieldFlags[index].IsCopied = onOff;
    }
  else
    {
      // We need to reallocate the list of fields
      vtkMaskFields::CopyFieldFlag* newFlags =
        new vtkMaskFields::CopyFieldFlag[this->NumberOfFieldFlags+1];
      // Copy old flags (pointer copy for name)
      for(int i=0; i<this->NumberOfFieldFlags; i++)
        {
          newFlags[i].Name = this->CopyFieldFlags[i].Name;
          newFlags[i].Type = this->CopyFieldFlags[i].Type;
          newFlags[i].Location = this->CopyFieldFlags[i].Location;
          newFlags[i].IsCopied = this->CopyFieldFlags[i].IsCopied;
        }
      // Copy new flag
      newFlags[this->NumberOfFieldFlags].Name = 0;
      newFlags[this->NumberOfFieldFlags].Type = attributeType;
      newFlags[this->NumberOfFieldFlags].Location = attributeLocation;
      newFlags[this->NumberOfFieldFlags].IsCopied = onOff;
      this->NumberOfFieldFlags++;
      delete[] this->CopyFieldFlags;
      this->CopyFieldFlags = newFlags;
    }
  this->Modified();
}

// Turn on copying of all data.
void vtkMaskFields::CopyAllOn()
{
  this->CopyFields = 1;
  this->CopyAttributes = 1;
  this->Modified();
}

// Turn off copying of all data.
void vtkMaskFields::CopyAllOff()
{
  this->CopyFields = 0;
  this->CopyAttributes = 0;
  this->Modified();
}

// Deallocate and clear the list of fields.
void vtkMaskFields::ClearFieldFlags()
{
  if (this->NumberOfFieldFlags > 0)
    {
      for(int i=0; i<this->NumberOfFieldFlags; i++)
        {
          delete[] this->CopyFieldFlags[i].Name;
        }
    }
  delete[] this->CopyFieldFlags;
  this->CopyFieldFlags=0;
  this->NumberOfFieldFlags=0;
}

// Find if field is in CopyFieldFlags.
// If it is, it returns the index otherwise it returns -1
int vtkMaskFields::FindFlag(const char* field, int loc)
{
  if ( !field ) return -1;
  for(int i=0; i<this->NumberOfFieldFlags; i++)
    {
      if (this->CopyFieldFlags[i].Name &&
          !strcmp(field, this->CopyFieldFlags[i].Name) &&
          this->CopyFieldFlags[i].Location == loc)
        {
          return i;
        }
    }
  return -1;
}

// Find if field is in CopyFieldFlags.
// If it is, it returns the index otherwise it returns -1
int vtkMaskFields::FindFlag(int attributeType, int loc)
{
  for(int i=0; i<this->NumberOfFieldFlags; i++)
    {
      if (this->CopyFieldFlags[i].Type == attributeType &&
          this->CopyFieldFlags[i].Location == loc)
        {
          return i;
        }
    }
  return -1;
}

// If there is no flag for this array, return -1.
// If there is one: return 0 if off, 1 if on
int vtkMaskFields::GetFlag(const char* field, int loc)
{
  int index = this->FindFlag(field, loc);
  if ( index == -1 )
    {
      return -1;
    }
  else 
    {
      return  this->CopyFieldFlags[index].IsCopied;
    }
}

// If there is no flag for this array, return -1.
// If there is one: return 0 if off, 1 if on
int vtkMaskFields::GetFlag(int arrayType, int loc)
{
  int index = this->FindFlag(arrayType, loc);
  if ( index == -1 )
    {
      return -1;
    }
  else 
    {
      return  this->CopyFieldFlags[index].IsCopied;
    }
}

void vtkMaskFields::Execute()
{
  vtkDataSet *input = static_cast<vtkDataSet*>(this->GetInput());
  vtkDataSet *output = static_cast<vtkDataSet*>(this->GetOutput());

  // This has to be here because it initializes all field data.
  output->CopyStructure( input );

  if (this->CopyFields && this->CopyAttributes) {
    vtkDebugMacro("Copying both fields and attributes.");
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();

    output->GetFieldData()->CopyAllOn();

  } else if (!this->CopyFields && this->CopyAttributes) {
    vtkDebugMacro("Copying only attributes.");
    output->GetPointData()->CopyAllOff();
    output->GetPointData()->CopyScalarsOn();
    output->GetPointData()->CopyVectorsOn();
    output->GetPointData()->CopyTensorsOn();
    output->GetPointData()->CopyNormalsOn();
    output->GetPointData()->CopyTCoordsOn();

    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyScalarsOn();
    output->GetCellData()->CopyVectorsOn();
    output->GetCellData()->CopyTensorsOn();
    output->GetCellData()->CopyNormalsOn();
    output->GetCellData()->CopyTCoordsOn();

  } else if (this->CopyFields && !this->CopyAttributes) {
    vtkDebugMacro("Copying only fields.");
    output->GetPointData()->CopyAllOn();
    output->GetPointData()->CopyScalarsOff();
    output->GetPointData()->CopyVectorsOff();
    output->GetPointData()->CopyTensorsOff();
    output->GetPointData()->CopyNormalsOff();
    output->GetPointData()->CopyTCoordsOff();

    output->GetCellData()->CopyAllOn();
    output->GetCellData()->CopyScalarsOff();
    output->GetCellData()->CopyVectorsOff();
    output->GetCellData()->CopyTensorsOff();
    output->GetCellData()->CopyNormalsOff();
    output->GetCellData()->CopyTCoordsOff();

    output->GetFieldData()->CopyAllOn();

  } else if (!this->CopyFields && !this->CopyAttributes) {
    vtkDebugMacro("Global copying off for fields and attributes.");
    output->GetPointData()->CopyAllOff();
    output->GetCellData()->CopyAllOff();

    output->GetFieldData()->CopyAllOff();

  }

  // individual flags take precedence, so all of above will be
  // overridden by individual flags...
  for(int i=0; i<this->NumberOfFieldFlags; ++i) {

    switch (this->CopyFieldFlags[i].Location) 
      {
      case vtkMaskFields::POINT_DATA:

        if (this->CopyFieldFlags[i].Type > -1) { // attribute data
          output->GetPointData()->SetCopyAttribute(this->CopyFieldFlags[i].Type,this->CopyFieldFlags[i].IsCopied);
        
        } else { // field data
          if (this->CopyFieldFlags[i].IsCopied == 1) {
          output->GetPointData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
          } else {
            output->GetPointData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
          }
        }
        break;
      case  vtkMaskFields::CELL_DATA:
        if (this->CopyFieldFlags[i].Type > -1) { // attribute data
          output->GetCellData()->SetCopyAttribute(this->CopyFieldFlags[i].Type,this->CopyFieldFlags[i].IsCopied);
        } else { // field data
          if (this->CopyFieldFlags[i].IsCopied == 1) {
            output->GetCellData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
          } else {
            output->GetCellData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
          }
        }
        break;
      case vtkMaskFields::OBJECT_DATA:
        if (this->CopyFieldFlags[i].IsCopied == 1) {
          output->GetFieldData()->CopyFieldOn(this->CopyFieldFlags[i].Name);
        } else {
          output->GetFieldData()->CopyFieldOff(this->CopyFieldFlags[i].Name);
        }
        break;
      default:
        vtkErrorMacro("unknown location field");
        break;
      }
  }

  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

}

void vtkMaskFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of field flags: " << this->NumberOfFieldFlags << endl;

  os << indent << "CopyFields: " << this->CopyFields << endl;

  os << indent << "CopyAttributes: " << this->CopyAttributes << endl;


}
