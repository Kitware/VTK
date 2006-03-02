/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssignAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkAssignAttribute.h"

#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include <ctype.h>

vtkCxxRevisionMacro(vtkAssignAttribute, "1.16");
vtkStandardNewMacro(vtkAssignAttribute);

char vtkAssignAttribute::AttributeLocationNames[2][12] 
= { "POINT_DATA",
    "CELL_DATA" };

char vtkAssignAttribute::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][10]  = { 0 };

vtkAssignAttribute::vtkAssignAttribute()
{
  this->FieldName = 0;
  this->AttributeLocation = -1;
  this->AttributeType = -1;
  this->InputAttributeType = -1;
  this->FieldType = -1;

  //convert the attribute names to uppercase for local use
  if (vtkAssignAttribute::AttributeNames[0][0] == 0) 
    {
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
      {
      int l = strlen(vtkDataSetAttributes::GetAttributeTypeAsString(i));
      for (int c = 0; c < l && c < 10; c++)
        {
        vtkAssignAttribute::AttributeNames[i][c] = 
          toupper(vtkDataSetAttributes::GetAttributeTypeAsString(i)[c]);
        }
      }
    }
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

int vtkAssignAttribute::RequestInformation(vtkInformation *vtkNotUsed(request),
                                           vtkInformationVector **inputVector,
                                           vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  if ((this->AttributeType != -1) &&
      (this->AttributeLocation != -1) && (this->FieldType != -1))
    {
    int fieldAssociation = this->AttributeLocation == POINT_DATA ?
      vtkDataObject::FIELD_ASSOCIATION_POINTS : vtkDataObject::FIELD_ASSOCIATION_CELLS;
    if (this->FieldType == vtkAssignAttribute::NAME && this->FieldName)
      {
      vtkDataObject::SetActiveAttribute(outInfo, fieldAssociation,
        this->FieldName, this->AttributeType);
      }
    else if (this->FieldType == vtkAssignAttribute::ATTRIBUTE  && 
      this->InputAttributeType != -1)
      {
      vtkInformation *inputAttributeInfo = vtkDataObject::GetActiveFieldInformation(
        inInfo, fieldAssociation, this->InputAttributeType);
      if (inputAttributeInfo) // do we have an active field of requested type
        {
        vtkDataObject::SetActiveAttribute(outInfo, fieldAssociation,
          inputAttributeInfo->Get( vtkDataObject::FIELD_NAME() ), 
          this->AttributeType);
        }
      }
    }

  return 1;
}


int vtkAssignAttribute::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and ouptut
  vtkDataSet *input = vtkDataSet::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet *output = vtkDataSet::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // This has to be here because it initialized all field datas.
  output->CopyStructure( input );

  if ( output->GetFieldData() && input->GetFieldData() )
    {
    output->GetFieldData()->PassData( input->GetFieldData() );
    }
  output->GetPointData()->PassData( input->GetPointData() );
  output->GetCellData()->PassData( input->GetCellData() );

  if ((this->AttributeType != -1) &&
      (this->AttributeLocation != -1) && (this->FieldType != -1))
    {
    vtkDataSetAttributes* ods=0;
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
      //int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
      //ods->GetAttributeIndices(attributeIndices);
      // if (attributeIndices[this->InputAttributeType] != -1)
      vtkDataArray *oda = ods->GetAttribute(this->InputAttributeType);
      if (oda)
        {
        ods->SetActiveAttribute(oda->GetName(),this->AttributeType);
        }
      }
    }

  return 1;
}

void vtkAssignAttribute::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
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
