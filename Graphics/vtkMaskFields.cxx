/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMaskFields.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMaskFields.h"

#include "vtkCellData.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include <ctype.h>

vtkStandardNewMacro(vtkMaskFields);

char vtkMaskFields::FieldLocationNames[3][12] 
= { "OBJECT_DATA",
    "POINT_DATA",
    "CELL_DATA" };
char vtkMaskFields::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10]  = { {0} };

vtkMaskFields::vtkMaskFields()
{

  this->CopyFieldFlags = 0;
  this->NumberOfFieldFlags = 0;
  this->CopyAllOn();

  //convert the attribute names to uppercase for local use
  if (vtkMaskFields::AttributeNames[0][0] == 0) 
    {
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
      {
      int l = static_cast<int>(strlen(vtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 10; c++)
        {
        vtkMaskFields::AttributeNames[i][c] = 
          toupper(vtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
        }
      }
    }
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

int vtkMaskFields::GetAttributeLocation(const char* attributeLoc)
{
  int numAttributeLocs = 3;
  int loc=-1;

  if (!attributeLoc)
    {
      return loc;
    }

  for (int i=0; i<numAttributeLocs; i++)
    {
      if (!strcmp(attributeLoc, FieldLocationNames[i]))
        {
          loc = i;
          break;
        }
    }
  return loc;
}

int vtkMaskFields::GetAttributeType(const char* attributeType)
{

  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int attrType=-1;

  if (!attributeType)
    {
      return attrType;
    }

  for (int i=0; i<numAttr; i++)
    {
      if (!strcmp(attributeType, AttributeNames[i]))
        {
          attrType = i;
          break;
        }
    }
  return attrType;
}

void vtkMaskFields::CopyAttributeOn(const char* attributeLoc, 
                                    const char* attributeType)
{
  if (!attributeType || !attributeLoc)
    {
      return;
    }

  // Convert strings to ints and call the appropriate CopyAttributeOn()

  int attrType = this->GetAttributeType(attributeType);
  if ( attrType == -1 )
    {
      vtkErrorMacro("Target attribute type is invalid.");
      return;
    }
  
  int loc = this->GetAttributeLocation(attributeLoc);
  if (loc == -1)
    {
      vtkErrorMacro("Target location for the attribute is invalid.");
      return;
    }

  this->CopyAttributeOn(loc, attrType);

}

void vtkMaskFields::CopyAttributeOff(const char* attributeLoc, 
                                     const char* attributeType)
{
  if (!attributeType || !attributeLoc)
    {
      return;
    }

  // Convert strings to ints and call the appropriate CopyAttributeOn()

  int attrType = this->GetAttributeType(attributeType);
  if ( attrType == -1 )
    {
      vtkErrorMacro("Target attribute type is invalid.");
      return;
    }
  
  int loc = this->GetAttributeLocation(attributeLoc);
  if (loc == -1)
    {
      vtkErrorMacro("Target location for the attribute is invalid.");
      return;
    }

  this->CopyAttributeOff(loc, attrType);

}

void vtkMaskFields::CopyFieldOn(const char* fieldLoc, 
                                const char* name)
{
  if (!name || !fieldLoc)
    {
      return;
    }

  // Convert strings to ints and call the appropriate CopyAttributeOn()  
  int loc = this->GetAttributeLocation(fieldLoc);
  if (loc == -1)
    {
      vtkErrorMacro("Target location for the attribute is invalid.");
      return;
    }

  this->CopyFieldOn(loc, name);

}

void vtkMaskFields::CopyFieldOff(const char* fieldLoc, 
                                 const char* name)
{
  if (!name || !fieldLoc)
    {
      return;
    }

  // Convert strings to ints and call the appropriate CopyAttributeOn()  
  int loc = this->GetAttributeLocation(fieldLoc);
  if (loc == -1)
    {
      vtkErrorMacro("Target location for the attribute is invalid.");
      return;
    }

  this->CopyFieldOff(loc, name);

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

int vtkMaskFields::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

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
    output->GetCellData()->CopyAllOff();
    int ai;
    for (ai = 0; ai < vtkDataSetAttributes::NUM_ATTRIBUTES; ai++)
      {
      output->GetPointData()->SetCopyAttribute(1, ai);
      output->GetCellData()->SetCopyAttribute(1, ai);
      }

  } else if (this->CopyFields && !this->CopyAttributes) {
    vtkDebugMacro("Copying only fields.");
    output->GetPointData()->CopyAllOn();
    output->GetCellData()->CopyAllOn();
    int ai;
    for (ai = 0; ai < vtkDataSetAttributes::NUM_ATTRIBUTES; ai++)
      {
      output->GetPointData()->SetCopyAttribute(0, ai);
      output->GetCellData()->SetCopyAttribute(0, ai);
      }
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

  // Pass all.
  if ( output->GetFieldData() && input->GetFieldData() )
    {
    output->GetFieldData()->PassData( input->GetFieldData() );
    }
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  return 1;
}

void vtkMaskFields::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Number of field flags: " << this->NumberOfFieldFlags << endl;

  os << indent << "CopyFields: " << this->CopyFields << endl;

  os << indent << "CopyAttributes: " << this->CopyAttributes << endl;


}
