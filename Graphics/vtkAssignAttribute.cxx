/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignAttribute.cxx
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
#include "vtkAssignAttribute.h"
#include "vtkObjectFactory.h"
#include "vtkDataSetAttributes.h"

char vtkAssignAttribute::AttributeLocationNames[2][12] 
= { "POINT_DATA",
    "CELL_DATA" };

char vtkAssignAttribute::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10] 
=  { "SCALARS",
     "VECTORS",
     "NORMALS",
     "TCOORDS",
     "TENSORS" };

//--------------------------------------------------------------------------
vtkAssignAttribute* vtkAssignAttribute::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkAssignAttribute");
  if(ret)
    {
    return (vtkAssignAttribute*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkAssignAttribute;
}

vtkAssignAttribute::vtkAssignAttribute()
{
  this->FieldName = 0;
  this->AttributeLocation = -1;
  this->AttributeType = -1;
  this->InputAttributeType = -1;
  this->FieldType = -1;
}

vtkAssignAttribute::~vtkAssignAttribute()
{
  delete[] this->FieldName;
  this->FieldName = 0;
}

void vtkAssignAttribute::Assign(const char* fieldName, int attributeType, 
				int attributeLoc)
{
  if (!fieldName)
    {
    return;
    }

  if ( (attributeType < 0) || 
       (attributeType > vtkDataSetAttributes::NUM_ATTRIBUTES) )
    {
    vtkErrorMacro("Wrong attribute type.");
    return;
    }

  if ( (attributeLoc !=  vtkAssignAttribute::POINT_DATA) &&
       (attributeLoc !=  vtkAssignAttribute::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  delete[] this->FieldName;
  this->FieldName = new char[strlen(fieldName)+1];
  strcpy(this->FieldName, fieldName);

  this->AttributeType = attributeType;
  this->AttributeLocation = attributeLoc;
  this->FieldType = vtkAssignAttribute::NAME;
}

void vtkAssignAttribute::Assign(int inputAttributeType, int attributeType, 
				int attributeLoc)
{
  if ( (attributeType < 0) || 
       (attributeType > vtkDataSetAttributes::NUM_ATTRIBUTES) ||
       (inputAttributeType < 0) || 
       (inputAttributeType > vtkDataSetAttributes::NUM_ATTRIBUTES))
    {
    vtkErrorMacro("Wrong attribute type.");
    return;
    }

  if ( (attributeLoc !=  vtkAssignAttribute::POINT_DATA) &&
       (attributeLoc !=  vtkAssignAttribute::CELL_DATA) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  this->AttributeType = attributeType;
  this->InputAttributeType = inputAttributeType;
  this->AttributeLocation = attributeLoc;
  this->FieldType = vtkAssignAttribute::ATTRIBUTE;
}

void vtkAssignAttribute::Assign(const char* name, 
				const char* attributeType, 
				const char* attributeLoc)
{
  if (!name || !attributeType || !attributeLoc)
    {
    return;
    }

  int numAttr = vtkDataSetAttributes::NUM_ATTRIBUTES;
  int numAttributeLocs = 2;
  int i;

  // Convert strings to ints and call the appropriate Assign()
  int inputAttributeType=-1;
  for(i=0; i<numAttr; i++)
    {
    if (!strcmp(name, AttributeNames[i]))
      {
      inputAttributeType = i;
      break;
      }
    }

  int attrType=-1;
  for(i=0; i<numAttr; i++)
    {
    if (!strcmp(attributeType, AttributeNames[i]))
      {
      attrType = i;
      break;
      }
    }
  if ( attrType == -1 )
    {
    vtkErrorMacro("Target attribute type is invalid.");
    return;
    }

  int loc=-1;
  for(i=0; i<numAttributeLocs; i++)
    {
    if (!strcmp(attributeLoc, AttributeLocationNames[i]))
      {
      loc = i;
      break;
      }
    }
  if (loc == -1)
    {
    vtkErrorMacro("Target location for the attribute is invalid.");
    return;
    }

  if ( inputAttributeType == -1 )
    {
    this->Assign(name, attrType, loc);
    }
  else
    {
    this->Assign(inputAttributeType, attrType, loc);
    }
}

void vtkAssignAttribute::Execute()
{
  vtkDataSet *input = static_cast<vtkDataSet*>(this->GetInput());
  vtkDataSet *output = static_cast<vtkDataSet*>(this->GetOutput());

  // This has to be here because it initialized all field datas.
  output->CopyStructure( input );
  
  // Pass all. (data object's field data is passed by the
  // superclass after this method)
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  if ((this->AttributeType != -1) &&
      (this->AttributeLocation != -1) && (this->FieldType != -1))
    {
    vtkDataSetAttributes* ods;
    // Get the appropriate output DataSetAttributes
    switch (this->AttributeLocation)
      {
      case vtkAssignAttribute::POINT_DATA:
	ods = output->GetPointData();
	break;
      case vtkAssignAttribute::CELL_DATA:
	ods = output->GetCellData();
	break;
      }
    if (this->FieldType == vtkAssignAttribute::NAME && this->FieldName)
      {
      ods->SetActiveAttribute(this->FieldName, this->AttributeType);
      }
    else if (this->FieldType == vtkAssignAttribute::ATTRIBUTE  && 
	     (this->InputAttributeType != -1))
      {
      // If labeling an attribute as another attribute, we
      // need to get it's index and call SetActiveAttribute()
      // with that index
      int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
      ods->GetAttributeIndices(attributeIndices);
      if (attributeIndices[this->InputAttributeType] != -1)
	{
	ods->SetActiveAttribute(attributeIndices[this->InputAttributeType], 
				this->AttributeType);
	}
      }
    }
}

void vtkAssignAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkDataSetToDataSetFilter::PrintSelf(os,indent);
  os << indent << "Field name: ";
  if (this->FieldName)
    {
    os << this->FieldName << endl;
    }
  else
    {
    os << "(none)" << endl;
    }
  os << indent << "Field type: " << this->FieldType << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Input attribute type: " << this->InputAttributeType
     << endl;
  os << indent << "Attribute location: " << this->AttributeLocation << endl;
}
