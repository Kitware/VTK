/*=========================================================================

  Program:   ParaView
  Module:    vtkSelection.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSelection.h"

#include "vtkAbstractArray.h"
#include "vtkFieldData.h"
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
#include "vtkTable.h"

#include <vtkstd/map>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSelection, "1.19");
vtkStandardNewMacro(vtkSelection);

vtkInformationKeyMacro(vtkSelection,CONTENT_TYPE,Integer);
vtkInformationKeyMacro(vtkSelection,SOURCE,ObjectBase);
vtkInformationKeyMacro(vtkSelection,SOURCE_ID,Integer);
vtkInformationKeyMacro(vtkSelection,PROP,ObjectBase);
vtkInformationKeyMacro(vtkSelection,PROP_ID,Integer);
vtkInformationKeyMacro(vtkSelection,PROCESS_ID,Integer);
vtkInformationKeyMacro(vtkSelection,GROUP,Integer);
vtkInformationKeyMacro(vtkSelection,BLOCK,Integer);
vtkInformationKeyMacro(vtkSelection,FIELD_TYPE,Integer);
vtkInformationKeyMacro(vtkSelection,EPSILON,Double);
vtkInformationKeyMacro(vtkSelection,PRESERVE_TOPOLOGY,Integer);
vtkInformationKeyMacro(vtkSelection,CONTAINING_CELLS,Integer);
vtkInformationKeyMacro(vtkSelection,PIXEL_COUNT,Integer);
vtkInformationKeyMacro(vtkSelection,INVERSE,Integer);
vtkInformationKeyMacro(vtkSelection,SHOW_BOUNDS,Integer);
vtkInformationKeyMacro(vtkSelection,INDEXED_VERTICES,Integer);

struct vtkSelectionInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSelection> > Children;
};


//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
{
  this->Internal = new vtkSelectionInternals;
  this->ParentNode = 0;
  this->Properties = vtkInformation::New();

  this->Information->Set(vtkDataObject::DATA_EXTENT_TYPE(), VTK_PIECES_EXTENT);
  this->Information->Set(vtkDataObject::DATA_PIECE_NUMBER(), -1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_PIECES(), 1);
  this->Information->Set(vtkDataObject::DATA_NUMBER_OF_GHOST_LEVELS(), 0);
}

//----------------------------------------------------------------------------
vtkSelection::~vtkSelection()
{
  delete this->Internal;
  this->ParentNode = 0;
  this->Properties->Delete();
}

//----------------------------------------------------------------------------
// Restore object to initial state. Release memory back to system.
void vtkSelection::Initialize()
{
  this->Superclass::Initialize();
  this->Clear();
  this->ParentNode = 0;
}

//----------------------------------------------------------------------------
void vtkSelection::Clear()
{
  delete this->Internal;
  this->Internal = new vtkSelectionInternals;
  this->Properties->Clear();

  this->Modified();
}

//----------------------------------------------------------------------------
vtkAbstractArray* vtkSelection::GetSelectionList()
{
  if (this->FieldData && this->FieldData->GetNumberOfArrays() > 0)
    {
    return this->FieldData->GetAbstractArray(0);
    }
  return 0;
}

//----------------------------------------------------------------------------
void vtkSelection::SetSelectionList(vtkAbstractArray* arr)
{
  if (!this->FieldData)
    {
    this->FieldData = vtkFieldData::New();
    }
  this->FieldData->Initialize();
  this->FieldData->AddArray(arr);  
}

//----------------------------------------------------------------------------
unsigned int vtkSelection::GetNumberOfChildren()
{
  return this->Internal->Children.size();
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetChild(unsigned int idx)
{
  if (idx >= this->GetNumberOfChildren())
    {
    return 0;
    }
  return this->Internal->Children[idx];
}

//----------------------------------------------------------------------------
void vtkSelection::AddChild(vtkSelection* child)
{
  if (!child)
    {
    return;
    }

  // Make sure that child is not already added
  unsigned int numChildren = this->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    if (this->GetChild(i) == child)
      {
      return;
      }
    }
  this->Internal->Children.push_back(child);
  child->ParentNode = this;

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveChild(unsigned int idx)
{
  if (idx >= this->GetNumberOfChildren())
    {
    return;
    }
  vtkstd::vector<vtkSmartPointer<vtkSelection> >::iterator iter =
    this->Internal->Children.begin();
  iter->GetPointer()->ParentNode = 0;
  this->Internal->Children.erase(iter+idx);

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveChild(vtkSelection* child)
{
  if (!child)
    {
    return;
    }

  unsigned int numChildren = this->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    if (this->GetChild(i) == child)
      {
      child->ParentNode = 0;
      this->RemoveChild(i);
      return;
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::RemoveAllChildren()
{
  vtkstd::vector<vtkSmartPointer<vtkSelection> >::iterator iter =
    this->Internal->Children.begin();
  for (; iter != this->Internal->Children.end(); ++iter)
    {
    iter->GetPointer()->ParentNode = 0;
    }
  this->Internal->Children.clear();
  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Properties:";
  if (this->Properties)
    {
    this->Properties->PrintSelf(os, indent.GetNextIndent());
    }
  else
    {
    os << "(none)" << endl;
    }

  os << indent << "ParentNode: ";
  if (this->ParentNode)
    {
    os << this->ParentNode;
    }
  else
    {
    os << "(none)";
    }
  os << endl;

  unsigned int numChildren = this->GetNumberOfChildren();
  os << indent << "Number of children: " << numChildren << endl;
  os << indent << "Children: " << endl;
  for (unsigned int i=0; i<numChildren; i++)
    {
    os << indent << "Child #" << i << endl;
    this->GetChild(i)->PrintSelf(os, indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
void vtkSelection::ShallowCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
    {
    return;
    }
  
  this->Initialize();
  
  this->Superclass::ShallowCopy(src);
  
  this->Properties->Copy(input->Properties, 0);

  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelection* newChild = vtkSelection::New();
    newChild->ShallowCopy(input->GetChild(i));
    this->AddChild(newChild);
    newChild->Delete();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::DeepCopy(vtkDataObject* src)
{
  vtkSelection *input = vtkSelection::SafeDownCast(src);
  if (!input)
    {
    return;
    }

  this->Superclass::DeepCopy(src);
  
  this->Properties->Copy(input->Properties, 1);

  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelection* newChild = vtkSelection::New();
    newChild->DeepCopy(input->GetChild(i));
    this->AddChild(newChild);
    newChild->Delete();
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::CopyChildren(vtkSelection* input)
{
  if (!this->Properties->Has(CONTENT_TYPE()) ||
      this->Properties->Get(CONTENT_TYPE()) != SELECTIONS)
    {
    return;
    }
  if (!input->Properties->Has(CONTENT_TYPE()) ||
      input->Properties->Get(CONTENT_TYPE()) != SELECTIONS)
    {
    return;
    }

  unsigned int numChildren = input->GetNumberOfChildren();
  for (unsigned int i=0; i<numChildren; i++)
    {
    vtkSelection* child = input->GetChild(i);
    if (child->Properties->Has(CONTENT_TYPE()) && 
        child->Properties->Get(CONTENT_TYPE()) == SELECTIONS)
      {
      // TODO: Handle trees
      }
    else
      {
      vtkSelection* newChild = vtkSelection::New();
      newChild->DeepCopy(child);
      this->AddChild(newChild);
      newChild->Delete();
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkSelection::SetContentType(int type)
{
  this->GetProperties()->Set(vtkSelection::CONTENT_TYPE(), type);
}

//----------------------------------------------------------------------------
int vtkSelection::GetContentType()
{
  if (this->GetProperties()->Has(vtkSelection::CONTENT_TYPE()))
    {
    return this->GetProperties()->Get(vtkSelection::CONTENT_TYPE());
    }
  return -1;
}

//----------------------------------------------------------------------------
void vtkSelection::SetFieldType(int type)
{
  this->GetProperties()->Set(vtkSelection::FIELD_TYPE(), type);
}

//----------------------------------------------------------------------------
int vtkSelection::GetFieldType()
{
  if (this->GetProperties()->Has(vtkSelection::FIELD_TYPE()))
    {
    return this->GetProperties()->Get(vtkSelection::FIELD_TYPE());
    }
  return -1;
}

//----------------------------------------------------------------------------
bool vtkSelection::EqualProperties(vtkSelection* other, 
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

  if (fullcompare)
    {
    return other->EqualProperties(this, false);
    }

  return true;
}

//----------------------------------------------------------------------------
void vtkSelection::UnionSelectionList(vtkSelection* other)
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
      {
      vtkAbstractArray* aa1 = this->GetSelectionList();
      vtkAbstractArray* aa2 = other->GetSelectionList();
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
      // TODO: avoid duplicates.
      for (vtkIdType i = 0; i < aa2->GetNumberOfTuples(); i++)
        {
        aa1->InsertNextTuple(i, aa2);
        }
      break;
      }
  case SELECTIONS:
  case COMPOSITE_SELECTIONS:
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
void vtkSelection::Union(vtkSelection* s)
{
  if (s->GetContentType() == SELECTIONS)
    {
    // Merge the selection with any of our children.
    for (unsigned int cc=0; cc < s->GetNumberOfChildren(); cc++)
      {
      s->GetChild(cc)->Union(s);
      }
    return;
    }

  // Now we are assured "s" is a leaf node.
  if (this->GetContentType() == SELECTIONS)
    {
    // Attempt to merge "s" with any of our children, if possible.
    // If not, a clone of "s" gets added as a new child.
    bool merged = false;
    for (unsigned int cc=0; cc < this->GetNumberOfChildren(); cc++)
      {
      vtkSelection* child = this->GetChild(cc);
      if (child->GetContentType() == SELECTIONS)
        {
        vtkErrorMacro("Selection trees deeper than 1 level are not handled.");
        return;
        }
      if (child->EqualProperties(s))
        {
        child->UnionSelectionList(s);
        merged = true;
        break;
        }
      }
    if (!merged)
      {
      vtkSelection* clone = vtkSelection::New();
      clone->ShallowCopy(s);
      this->AddChild(clone);
      clone->Delete();
      }

    }
  else if (this->EqualProperties(s))
    {
    this->UnionSelectionList(s);
    }
  else
    {
    if (this->GetParentNode())
      {
      // sanity check to ensure we don't create trees deeper than 1 level.
      vtkErrorMacro("Cannot merge. Sanity check for depth of tree failed.");
      return;
      }
    
    vtkSelection* clone = vtkSelection::New();
    clone->ShallowCopy(this);
    this->Initialize();
    this->SetContentType(vtkSelection::SELECTIONS);
    this->AddChild(clone);
    clone->Delete();

    clone = vtkSelection::New();
    clone->ShallowCopy(s);
    this->AddChild(clone);
    clone->Delete();
    }
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If properties is modified,
// then this object is modified as well.
unsigned long vtkSelection::GetMTime()
{
  unsigned long mTime=this->MTime.GetMTime();
  unsigned long propMTime;

  if ( this->Properties != NULL )
    {
    propMTime = this->Properties->GetMTime();
    mTime = ( propMTime > mTime ? propMTime : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformation* info)
{
  return info? vtkSelection::SafeDownCast(info->Get(DATA_OBJECT())) : 0;
}

//----------------------------------------------------------------------------
vtkSelection* vtkSelection::GetData(vtkInformationVector* v, int i)
{
  return vtkSelection::GetData(v->GetInformationObject(i));
}
