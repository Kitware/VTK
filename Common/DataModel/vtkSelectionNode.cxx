/*=========================================================================

  Program:   ParaView
  Module:    vtkSelectionNode.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelectionNode.h"

#include "vtkDataArrayRange.h"
#include "vtkDataObject.h"
#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iterator>
#include <utility>
#include <vector>

vtkStandardNewMacro(vtkSelectionNode);
vtkCxxSetObjectMacro(vtkSelectionNode, SelectionData, vtkDataSetAttributes);

const char vtkSelectionNode ::ContentTypeNames[vtkSelectionNode::NUM_CONTENT_TYPES][14] = {
  "SELECTIONS", // deprecated
  "GLOBALIDS",
  "PEDIGREEIDS",
  "VALUES",
  "INDICES",
  "FRUSTUM",
  "LOCATIONS",
  "THRESHOLDS",
  "BLOCKS",
  "QUERY",
  "USER",
};

const char vtkSelectionNode ::FieldTypeNames[vtkSelectionNode::NUM_FIELD_TYPES][8] = {
  "CELL",
  "POINT",
  "FIELD",
  "VERTEX",
  "EDGE",
  "ROW",
};

vtkInformationKeyMacro(vtkSelectionNode, CONTENT_TYPE, Integer);
vtkInformationKeyMacro(vtkSelectionNode, SOURCE, ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode, SOURCE_ID, Integer);
vtkInformationKeyMacro(vtkSelectionNode, PROP, ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode, PROP_ID, Integer);
vtkInformationKeyMacro(vtkSelectionNode, PROCESS_ID, Integer);
vtkInformationKeyMacro(vtkSelectionNode, COMPOSITE_INDEX, Integer);
vtkInformationKeyMacro(vtkSelectionNode, HIERARCHICAL_LEVEL, Integer);
vtkInformationKeyMacro(vtkSelectionNode, HIERARCHICAL_INDEX, Integer);
vtkInformationKeyMacro(vtkSelectionNode, FIELD_TYPE, Integer);
vtkInformationKeyMacro(vtkSelectionNode, EPSILON, Double);
vtkInformationKeyMacro(vtkSelectionNode, ZBUFFER_VALUE, Double);
vtkInformationKeyMacro(vtkSelectionNode, CONTAINING_CELLS, Integer);
vtkInformationKeyMacro(vtkSelectionNode, CONNECTED_LAYERS, Integer);
vtkInformationKeyMacro(vtkSelectionNode, PIXEL_COUNT, Integer);
vtkInformationKeyMacro(vtkSelectionNode, INVERSE, Integer);
vtkInformationKeyMacro(vtkSelectionNode, INDEXED_VERTICES, Integer);
vtkInformationKeyMacro(vtkSelectionNode, COMPONENT_NUMBER, Integer);

//----------------------------------------------------------------------------
vtkSelectionNode::vtkSelectionNode()
{
  this->SelectionData = vtkDataSetAttributes::New();
  this->Properties = vtkInformation::New();
  this->QueryString = nullptr;
}

//----------------------------------------------------------------------------
vtkSelectionNode::~vtkSelectionNode()
{
  this->Properties->Delete();
  if (this->SelectionData)
  {
    this->SelectionData->Delete();
  }
  this->SetQueryString(nullptr);
}

//----------------------------------------------------------------------------
void vtkSelectionNode::Initialize()
{
  this->Properties->Clear();
  if (this->SelectionData)
  {
    this->SelectionData->Initialize();
  }
  this->Modified();
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkSelectionNode::GetSelectionList()
{
  if (this->SelectionData && this->SelectionData->GetNumberOfArrays() > 0)
  {
    return this->SelectionData->GetAbstractArray(0);
  }
  return nullptr;
}

//----------------------------------------------------------------------------
void vtkSelectionNode::SetSelectionList(vtkAbstractArray* arr)
{
  if (!this->SelectionData)
  {
    this->SelectionData = vtkDataSetAttributes::New();
  }
  this->SelectionData->Initialize();
  this->SelectionData->AddArray(arr);
}

//----------------------------------------------------------------------------
void vtkSelectionNode::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ContentType: ";
  if (this->GetContentType() < SelectionContent::NUM_CONTENT_TYPES)
  {
    os << vtkSelectionNode::GetContentTypeAsString(this->GetContentType());
  }
  else
  {
    os << "UNKNOWN";
  }
  os << endl;

  os << indent << "FieldType: ";
  if (this->GetFieldType() < SelectionField::NUM_FIELD_TYPES)
  {
    os << vtkSelectionNode::GetFieldTypeAsString(this->GetFieldType());
  }
  else
  {
    os << "UNKNOWN";
  }
  os << endl;

  os << indent << "Properties: " << (this->Properties ? "" : "(none)") << endl;
  if (this->Properties)
  {
    this->Properties->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "SelectionData: " << (this->SelectionData ? "" : "(none)") << endl;
  if (this->SelectionData)
  {
    this->SelectionData->PrintSelf(os, indent.GetNextIndent());
  }
  os << indent << "QueryString: " << (this->QueryString ? this->QueryString : "nullptr") << endl;
}

//----------------------------------------------------------------------------
void vtkSelectionNode::ShallowCopy(vtkSelectionNode* input)
{
  if (!input)
  {
    return;
  }
  this->Initialize();
  this->Properties->Copy(input->Properties, 0);
  this->SelectionData->ShallowCopy(input->SelectionData);
  this->SetQueryString(input->GetQueryString());
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionNode::DeepCopy(vtkSelectionNode* input)
{
  if (!input)
  {
    return;
  }
  this->Initialize();
  this->Properties->Copy(input->Properties, 1);
  this->SelectionData->DeepCopy(input->SelectionData);
  this->SetQueryString(input->GetQueryString());
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelectionNode::SetContentType(int type)
{
  this->GetProperties()->Set(vtkSelectionNode::CONTENT_TYPE(), type);
}

//----------------------------------------------------------------------------
int vtkSelectionNode::GetContentType()
{
  if (this->GetProperties()->Has(vtkSelectionNode::CONTENT_TYPE()))
  {
    return this->GetProperties()->Get(vtkSelectionNode::CONTENT_TYPE());
  }
  return -1;
}

//----------------------------------------------------------------------------
const char* vtkSelectionNode::GetContentTypeAsString(int type)
{
  return vtkSelectionNode::ContentTypeNames[type];
}

//----------------------------------------------------------------------------
void vtkSelectionNode::SetFieldType(int type)
{
  this->GetProperties()->Set(vtkSelectionNode::FIELD_TYPE(), type);
}

//----------------------------------------------------------------------------
int vtkSelectionNode::GetFieldType()
{
  if (this->GetProperties()->Has(vtkSelectionNode::FIELD_TYPE()))
  {
    return this->GetProperties()->Get(vtkSelectionNode::FIELD_TYPE());
  }
  return -1;
}

//----------------------------------------------------------------------------
const char* vtkSelectionNode::GetFieldTypeAsString(int type)
{
  return vtkSelectionNode::FieldTypeNames[type];
}

//----------------------------------------------------------------------------
bool vtkSelectionNode::EqualProperties(vtkSelectionNode* other, bool fullcompare /*=true*/)
{
  if (!other)
  {
    return false;
  }

  vtkSmartPointer<vtkInformationIterator> iterSelf = vtkSmartPointer<vtkInformationIterator>::New();

  iterSelf->SetInformation(this->Properties);

  vtkInformation* otherProperties = other->GetProperties();
  for (iterSelf->InitTraversal(); !iterSelf->IsDoneWithTraversal(); iterSelf->GoToNextItem())
  {
    vtkInformationKey* key = iterSelf->GetCurrentKey();
    vtkInformationIntegerKey* ikey = vtkInformationIntegerKey::SafeDownCast(key);
    vtkInformationObjectBaseKey* okey = vtkInformationObjectBaseKey::SafeDownCast(key);
    if (ikey)
    {
      if (!otherProperties->Has(ikey) || this->Properties->Get(ikey) != otherProperties->Get(ikey))
      {
        return false;
      }
    }
    if (okey)
    {
      if (!otherProperties->Has(okey) || this->Properties->Get(okey) != otherProperties->Get(okey))
      {
        return false;
      }
    }
  }

  // Also check that array names match for certain content types
  // For VALUES and THRESHOLDS, names represent array where values come from.
  // For PEDIGREEIDS, names represent the domain of the selection.
  if (this->GetContentType() == vtkSelectionNode::VALUES ||
    this->GetContentType() == vtkSelectionNode::PEDIGREEIDS ||
    this->GetContentType() == vtkSelectionNode::THRESHOLDS)
  {
    int numArrays = other->SelectionData->GetNumberOfArrays();
    if (this->SelectionData->GetNumberOfArrays() != numArrays)
    {
      return false;
    }
    for (int a = 0; a < numArrays; ++a)
    {
      vtkAbstractArray* arr = this->SelectionData->GetAbstractArray(a);
      vtkAbstractArray* otherArr = other->SelectionData->GetAbstractArray(a);
      if (!arr->GetName() && otherArr->GetName())
      {
        return false;
      }
      if (arr->GetName() && !otherArr->GetName())
      {
        return false;
      }
      if (arr->GetName() && otherArr->GetName() && strcmp(arr->GetName(), otherArr->GetName()))
      {
        return false;
      }
    }
  }

  if (fullcompare)
  {
    return other->EqualProperties(this, false);
  }

  return true;
}

//----------------------------------------------------------------------------
void vtkSelectionNode::UnionSelectionList(vtkSelectionNode* other)
{
  int type = this->Properties->Get(CONTENT_TYPE());
  switch (type)
  {
    case GLOBALIDS:
    case PEDIGREEIDS:
    case VALUES:
    case INDICES:
    case LOCATIONS:
    case THRESHOLDS:
    case BLOCKS:
    {
      vtkDataSetAttributes* fd1 = this->GetSelectionData();
      vtkDataSetAttributes* fd2 = other->GetSelectionData();
      if (fd1->GetNumberOfArrays() != fd2->GetNumberOfArrays())
      {
        vtkErrorMacro(<< "Cannot take the union where the number of arrays do not match.");
      }
      for (int i = 0; i < fd1->GetNumberOfArrays(); i++)
      {
        vtkAbstractArray* aa1 = fd1->GetAbstractArray(i);
        vtkAbstractArray* aa2 = nullptr;
        if (i == 0 && type != VALUES && type != THRESHOLDS)
        {
          aa2 = fd2->GetAbstractArray(i);
        }
        else
        {
          aa2 = fd2->GetAbstractArray(aa1->GetName());
        }
        if (!aa2)
        {
          vtkErrorMacro(<< "Could not find array with name " << aa1->GetName()
                        << " in other selection.");
          return;
        }
        if (aa1->GetDataType() != aa2->GetDataType())
        {
          vtkErrorMacro(<< "Cannot take the union where selection list types "
                        << "do not match.");
          return;
        }
        if (aa1->GetNumberOfComponents() != aa2->GetNumberOfComponents())
        {
          vtkErrorMacro(<< "Cannot take the union where selection list number "
                        << "of components do not match.");
          return;
        }
        // If it is the same array, we are done.
        if (aa1 == aa2)
        {
          return;
        }
        int numComps = aa2->GetNumberOfComponents();
        vtkIdType numTuples = aa2->GetNumberOfTuples();
        for (vtkIdType j = 0; j < numTuples; j++)
        {
          // Avoid duplicates on single-component arrays.
          if (numComps != 1 || aa1->LookupValue(aa2->GetVariantValue(j)) == -1)
          {
            aa1->InsertNextTuple(j, aa2);
          }
        }
      }
      break;
    }
    case FRUSTUM:
    case USER:
    default:
    {
      vtkErrorMacro(<< "Do not know how to take the union of content type " << type << ".");
      return;
    }
  }
}

//----------------------------------------------------------------------------
void vtkSelectionNode::SubtractSelectionList(vtkSelectionNode* other)
{
  int type = this->Properties->Get(CONTENT_TYPE());
  switch (type)
  {
    case GLOBALIDS:
    case INDICES:
    case PEDIGREEIDS:
    {
      vtkDataSetAttributes* fd1 = this->GetSelectionData();
      vtkDataSetAttributes* fd2 = other->GetSelectionData();
      if (fd1->GetNumberOfArrays() != fd2->GetNumberOfArrays())
      {
        vtkErrorMacro(<< "Cannot take subtract selections if the number of arrays do not match.");
        return;
      }
      if (fd1->GetNumberOfArrays() != 1 || fd2->GetNumberOfArrays() != 1)
      {
        vtkErrorMacro(<< "Cannot subtract selections with more than one array.");
        return;
      }

      if (fd1->GetArray(0)->GetDataType() != VTK_ID_TYPE ||
        fd2->GetArray(0)->GetDataType() != VTK_ID_TYPE)
      {
        vtkErrorMacro(<< "Can only subtract selections with vtkIdTypeArray lists.");
        return;
      }

      vtkIdTypeArray* fd1_array = (vtkIdTypeArray*)fd1->GetArray(0);
      vtkIdTypeArray* fd2_array = (vtkIdTypeArray*)fd2->GetArray(0);

      if (fd1_array->GetNumberOfComponents() != 1 || fd2_array->GetNumberOfComponents() != 1)
      {
        vtkErrorMacro("Can only subtract selections with single component arrays.");
        return;
      }

      auto fd1Range = vtk::DataArrayValueRange<1>(fd1_array);
      auto fd2Range = vtk::DataArrayValueRange<1>(fd2_array);

      // make sure both arrays are sorted
      std::sort(fd1Range.begin(), fd1Range.end());
      std::sort(fd2Range.begin(), fd2Range.end());

      // set_difference result is bounded by the first set:
      std::vector<vtkIdType> result;
      result.resize(static_cast<std::size_t>(fd1Range.size()));

      auto diffEnd = std::set_difference(
        fd1Range.cbegin(), fd1Range.cend(), fd2Range.cbegin(), fd2Range.cend(), result.begin());
      result.erase(diffEnd, result.end());

      // Copy the result back into fd1_array:
      fd1_array->Reset();
      fd1_array->SetNumberOfTuples(static_cast<vtkIdType>(result.size()));
      fd1Range = vtk::DataArrayValueRange<1>(fd1_array);
      std::copy(result.cbegin(), result.cend(), fd1Range.begin());
      break;
    }
    case BLOCKS:
    case FRUSTUM:
    case LOCATIONS:
    case THRESHOLDS:
    case VALUES:
    case USER:
    default:
    {
      vtkErrorMacro(<< "Do not know how to subtract the given content type " << type << ".");
    }
  };
}

//----------------------------------------------------------------------------
vtkMTimeType vtkSelectionNode::GetMTime()
{
  vtkMTimeType mTime = this->MTime.GetMTime();
  vtkMTimeType propMTime;
  vtkMTimeType fieldMTime;
  if (this->Properties)
  {
    propMTime = this->Properties->GetMTime();
    mTime = (propMTime > mTime ? propMTime : mTime);
  }
  if (this->SelectionData)
  {
    fieldMTime = this->SelectionData->GetMTime();
    mTime = (fieldMTime > mTime ? fieldMTime : mTime);
  }
  return mTime;
}

//----------------------------------------------------------------------------
int vtkSelectionNode::ConvertSelectionFieldToAttributeType(int selectionField)
{
  switch (selectionField)
  {
    case vtkSelectionNode::CELL:
      return vtkDataObject::CELL;
    case vtkSelectionNode::POINT:
      return vtkDataObject::POINT;
    case vtkSelectionNode::FIELD:
      return vtkDataObject::FIELD;
    case vtkSelectionNode::VERTEX:
      return vtkDataObject::VERTEX;
    case vtkSelectionNode::EDGE:
      return vtkDataObject::EDGE;
    case vtkSelectionNode::ROW:
      return vtkDataObject::ROW;
    default:
      vtkGenericWarningMacro("Invalid selection field type: " << selectionField);
      return vtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES;
  }
}

//----------------------------------------------------------------------------
int vtkSelectionNode::ConvertAttributeTypeToSelectionField(int attrType)
{
  switch (attrType)
  {
    case vtkDataObject::CELL:
      return vtkSelectionNode::CELL;
    case vtkDataObject::POINT:
      return vtkSelectionNode::POINT;
    case vtkDataObject::FIELD:
      return vtkSelectionNode::FIELD;
    case vtkDataObject::VERTEX:
      return vtkSelectionNode::VERTEX;
    case vtkDataObject::EDGE:
      return vtkSelectionNode::EDGE;
    case vtkDataObject::ROW:
      return vtkSelectionNode::ROW;
    default:
      vtkGenericWarningMacro("Invalid attribute type: " << attrType);
      return vtkSelectionNode::CELL;
  }
}
