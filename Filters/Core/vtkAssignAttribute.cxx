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
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include <ctype.h>

vtkStandardNewMacro(vtkAssignAttribute);

char vtkAssignAttribute::AttributeLocationNames[vtkAssignAttribute::NUM_ATTRIBUTE_LOCS][12] 
= { "POINT_DATA",
    "CELL_DATA",
    "VERTEX_DATA",
    "EDGE_DATA"};

char vtkAssignAttribute::AttributeNames[vtkDataSetAttributes::NUM_ATTRIBUTES][20]  = { {0} };

vtkAssignAttribute::vtkAssignAttribute()
{
  this->FieldName = 0;
  this->AttributeLocationAssignment = -1;
  this->AttributeType = -1;
  this->InputAttributeType = -1;
  this->FieldTypeAssignment = -1;

  //convert the attribute names to uppercase for local use
  if (vtkAssignAttribute::AttributeNames[0][0] == 0) 
    {
    for (int i = 0; i < vtkDataSetAttributes::NUM_ATTRIBUTES; i++)
      {
      int l = static_cast<int>(strlen(vtkDataSetAttributes::GetAttributeTypeAsString(i)));
      for (int c = 0; c < l && c < 19; c++)
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

  if ( (attributeLoc < 0) ||
       (attributeLoc > vtkAssignAttribute::NUM_ATTRIBUTE_LOCS) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  delete[] this->FieldName;
  this->FieldName = new char[strlen(fieldName)+1];
  strcpy(this->FieldName, fieldName);

  this->AttributeType = attributeType;
  this->AttributeLocationAssignment = attributeLoc;
  this->FieldTypeAssignment = vtkAssignAttribute::NAME;
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

  if ( (attributeLoc < 0) ||
       (attributeLoc > vtkAssignAttribute::NUM_ATTRIBUTE_LOCS) )
    {
    vtkErrorMacro("The source for the field is wrong.");
    return;
    }

  this->Modified();
  this->AttributeType = attributeType;
  this->InputAttributeType = inputAttributeType;
  this->AttributeLocationAssignment = attributeLoc;
  this->FieldTypeAssignment = vtkAssignAttribute::ATTRIBUTE;
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
  int numAttributeLocs = vtkAssignAttribute::NUM_ATTRIBUTE_LOCS;
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
      (this->AttributeLocationAssignment != -1) && (this->FieldTypeAssignment != -1))
    {
    int fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
    switch (this->AttributeLocationAssignment)
      {
      case POINT_DATA:
        fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_POINTS;
        break;
      case CELL_DATA:
        fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_CELLS;
        break;
      case VERTEX_DATA:
        fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_VERTICES;
        break;
      default:
        fieldAssociation = vtkDataObject::FIELD_ASSOCIATION_EDGES;
        break;
      }
    if (this->FieldTypeAssignment == vtkAssignAttribute::NAME && this->FieldName)
      {
      vtkDataObject::SetActiveAttribute(outInfo, fieldAssociation,
        this->FieldName, this->AttributeType);
      }
    else if (this->FieldTypeAssignment == vtkAssignAttribute::ATTRIBUTE  && 
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

  // get the input and output
  vtkDataObject *input = inInfo->Get(vtkDataObject::DATA_OBJECT());
  vtkDataObject *output = outInfo->Get(vtkDataObject::DATA_OBJECT());
  
  vtkDataSetAttributes* ods=0;
  if (vtkDataSet::SafeDownCast(input))
    {
    vtkDataSet *dsInput = vtkDataSet::SafeDownCast(input);
    vtkDataSet *dsOutput = vtkDataSet::SafeDownCast(output);
    // This has to be here because it initialized all field datas.
    dsOutput->CopyStructure( dsInput );

    if ( dsOutput->GetFieldData() && dsInput->GetFieldData() )
      {
      dsOutput->GetFieldData()->PassData( dsInput->GetFieldData() );
      }
    dsOutput->GetPointData()->PassData( dsInput->GetPointData() );
    dsOutput->GetCellData()->PassData( dsInput->GetCellData() );
    switch (this->AttributeLocationAssignment)
      {
      case vtkAssignAttribute::POINT_DATA:
        ods = dsOutput->GetPointData();
        break;
      case vtkAssignAttribute::CELL_DATA:
        ods = dsOutput->GetCellData();
        break;
      default:
        vtkErrorMacro(<<"Data must be point or cell for vtkDataSet");
        return 0;
      }
    }
  else
    {
    vtkGraph *graphInput = vtkGraph::SafeDownCast(input);
    vtkGraph *graphOutput = vtkGraph::SafeDownCast(output);
    graphOutput->ShallowCopy( graphInput );
    switch (this->AttributeLocationAssignment)
      {
      case vtkAssignAttribute::VERTEX_DATA:
        ods = graphOutput->GetVertexData();
        break;
      case vtkAssignAttribute::EDGE_DATA:
        ods = graphOutput->GetEdgeData();
        break;
      default:
        vtkErrorMacro(<<"Data must be vertex or edge for vtkGraph");
        return 0;
      }
    }

  if ((this->AttributeType != -1) &&
      (this->AttributeLocationAssignment != -1) && (this->FieldTypeAssignment != -1))
    {
    // Get the appropriate output DataSetAttributes
    if (this->FieldTypeAssignment == vtkAssignAttribute::NAME && this->FieldName)
      {
      ods->SetActiveAttribute(this->FieldName, this->AttributeType);
      }
    else if (this->FieldTypeAssignment == vtkAssignAttribute::ATTRIBUTE  && 
             (this->InputAttributeType != -1))
      {
      // If labeling an attribute as another attribute, we
      // need to get it's index and call SetActiveAttribute()
      // with that index
      //int attributeIndices[vtkDataSetAttributes::NUM_ATTRIBUTES];
      //ods->GetAttributeIndices(attributeIndices);
      // if (attributeIndices[this->InputAttributeType] != -1)
      vtkAbstractArray *oaa = ods->GetAbstractAttribute(this->InputAttributeType);
      if (oaa)
        {
        ods->SetActiveAttribute(oaa->GetName(),this->AttributeType);
        }
      }
    }

  return 1;
}

int vtkAssignAttribute::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  // This algorithm may accept a vtkPointSet or vtkGraph.
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
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
  os << indent << "Field type: " << this->FieldTypeAssignment << endl;
  os << indent << "Attribute type: " << this->AttributeType << endl;
  os << indent << "Input attribute type: " << this->InputAttributeType
     << endl;
  os << indent << "Attribute location: " << this->AttributeLocationAssignment << endl;
}
