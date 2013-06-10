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

#include "vtkDataSetAttributes.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerKey.h"
#include "vtkInformationIterator.h"
#include "vtkInformationObjectBaseKey.h"
#include "vtkInformationStringKey.h"
#include "vtkInformationDoubleKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <set>
#include <iterator>

vtkStandardNewMacro(vtkSelectionNode);
vtkCxxSetObjectMacro(vtkSelectionNode, SelectionData, vtkDataSetAttributes);


vtkInformationKeyMacro(vtkSelectionNode,CONTENT_TYPE,Integer);
vtkInformationKeyMacro(vtkSelectionNode,SOURCE,ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode,SOURCE_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,PROP,ObjectBase);
vtkInformationKeyMacro(vtkSelectionNode,PROP_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,PROCESS_ID,Integer);
vtkInformationKeyMacro(vtkSelectionNode,COMPOSITE_INDEX,Integer);
vtkInformationKeyMacro(vtkSelectionNode,HIERARCHICAL_LEVEL,Integer);
vtkInformationKeyMacro(vtkSelectionNode,HIERARCHICAL_INDEX,Integer);
vtkInformationKeyMacro(vtkSelectionNode,FIELD_TYPE,Integer);
vtkInformationKeyMacro(vtkSelectionNode,EPSILON,Double);
vtkInformationKeyMacro(vtkSelectionNode,CONTAINING_CELLS,Integer);
vtkInformationKeyMacro(vtkSelectionNode,PIXEL_COUNT,Integer);
vtkInformationKeyMacro(vtkSelectionNode,INVERSE,Integer);
vtkInformationKeyMacro(vtkSelectionNode,INDEXED_VERTICES,Integer);
vtkInformationKeyMacro(vtkSelectionNode,COMPONENT_NUMBER,Integer);

//----------------------------------------------------------------------------
vtkSelectionNode::vtkSelectionNode()
{
  this->SelectionData = vtkDataSetAttributes::New();
  this->Properties = vtkInformation::New();
  this->QueryString = 0;
}

//----------------------------------------------------------------------------
vtkSelectionNode::~vtkSelectionNode()
{
  this->Properties->Delete();
  if (this->SelectionData)
    {
    this->SelectionData->Delete();
    }
  this->SetQueryString(0);
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
  return 0;
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
  switch (this->GetContentType())
    {
    case GLOBALIDS:
      os << "GLOBALIDS";
      break;
    case PEDIGREEIDS:
      os << "PEDIGREEIDS";
      break;
    case VALUES:
      os << "VALUES";
      break;
    case INDICES:
      os << "INDICES";
      break;
    case FRUSTUM:
      os << "FRUSTUM";
      break;
    case LOCATIONS:
      os << "LOCATIONS";
      break;
    case THRESHOLDS:
      os << "THRESHOLDS";
      break;
    case BLOCKS:
      os << "BLOCKS";
      break;
    default:
      os << "UNKNOWN";
      break;
    }
  os << endl;
  os << indent << "FieldType: ";
  switch (this->GetFieldType())
    {
    case CELL:
      os << "CELL";
      break;
    case POINT:
      os << "POINT";
      break;
    case FIELD:
      os << "FIELD";
      break;
    case VERTEX:
      os << "VERTEX";
      break;
    case EDGE:
      os << "EDGE";
      break;
    case ROW:
      os << "ROW";
      break;
    default:
      os << "UNKNOWN";
      break;
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
  os << indent << "QueryString: " << (this->QueryString ? this->QueryString : "NULL") << endl;
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
bool vtkSelectionNode::EqualProperties(vtkSelectionNode* other,
  bool fullcompare/*=true*/)
{
  if (!other)
    {
    return false;
    }

  vtkSmartPointer<vtkInformationIterator> iterSelf =
    vtkSmartPointer<vtkInformationIterator>::New();

  iterSelf->SetInformation(this->Properties);

  vtkInformation* otherProperties = other->GetProperties();
  for (iterSelf->InitTraversal(); !iterSelf->IsDoneWithTraversal();
    iterSelf->GoToNextItem())
    {
    vtkInformationKey* key = iterSelf->GetCurrentKey();
    vtkInformationIntegerKey* ikey =
      vtkInformationIntegerKey::SafeDownCast(key);
    vtkInformationObjectBaseKey* okey =
      vtkInformationObjectBaseKey::SafeDownCast(key);
    if (ikey)
      {
      if (!otherProperties->Has(ikey) ||
        this->Properties->Get(ikey) != otherProperties->Get(ikey))
        {
        return false;
        }
      }
    if (okey)
      {
      if (!otherProperties->Has(okey) ||
        this->Properties->Get(okey) != otherProperties->Get(okey))
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
        vtkAbstractArray* aa2 = 0;
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
          vtkErrorMacro(<< "Could not find array with name "
                        << aa1->GetName() << " in other selection.");
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
  default:
      {
      vtkErrorMacro(<< "Do not know how to take the union of content type "
        << type << ".");
      return;
      }
    }
}

//----------------------------------------------------------------------------
void vtkSelectionNode::SubtractSelectionList(vtkSelectionNode* other)
{
  int type = this->Properties->Get(CONTENT_TYPE());
  switch(type)
    {
    case GLOBALIDS:
    case INDICES:
    case PEDIGREEIDS:
      {
      vtkDataSetAttributes * fd1 = this->GetSelectionData();
      vtkDataSetAttributes * fd2 = other->GetSelectionData();
      if(fd1->GetNumberOfArrays() != fd2->GetNumberOfArrays())
        {
        vtkErrorMacro(<< "Cannot take subtract selections if the number of arrays do not match.");
        }
        if( fd1->GetNumberOfArrays() != 1 || fd2->GetNumberOfArrays() != 1)
          {
          vtkErrorMacro(<<"Cannot subtract selections with more than one array.");
          return;
          }

        if( fd1->GetArray(0)->GetDataType() != VTK_ID_TYPE || fd2->GetArray(0)->GetDataType() != VTK_ID_TYPE )
          {
          vtkErrorMacro(<<"Can only subtract selections with vtkIdTypeArray lists.");
          }

          vtkIdTypeArray * fd1_array = (vtkIdTypeArray*)fd1->GetArray(0);
          vtkIdTypeArray * fd2_array = (vtkIdTypeArray*)fd2->GetArray(0);

          vtkIdType fd1_N = fd1_array->GetNumberOfTuples();
          vtkIdType fd2_N = fd2_array->GetNumberOfTuples();

          vtkIdType * fd1_P = (vtkIdType*)fd1_array->GetVoidPointer(0);
          vtkIdType * fd2_P = (vtkIdType*)fd2_array->GetVoidPointer(0);

          // make sure both arrays are sorted
          std::sort( fd1_P, fd1_P + fd1_N );
          std::sort( fd2_P, fd2_P + fd2_N );

          std::set<vtkIdType> result;

          std::set_difference(fd1_P, fd1_P + fd1_N,
                                 fd2_P, fd2_P + fd2_N,
                                 std::inserter(result, result.end()));

          fd1_array->Reset();
          for(std::set<vtkIdType>::const_iterator p = result.begin(); p!=result.end(); ++p)
            {
            fd1_array->InsertNextValue( *p );
            }
      break;
      }
    case BLOCKS:
    case FRUSTUM:
    case LOCATIONS:
    case THRESHOLDS:
    case VALUES:
    default:
      {
      vtkErrorMacro(<< "Do not know how to subtract the given content type " << type << ".");
      }
    };
}

//----------------------------------------------------------------------------
unsigned long vtkSelectionNode::GetMTime()
{
  unsigned long mTime = this->MTime.GetMTime();
  unsigned long propMTime;
  unsigned long fieldMTime;
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
