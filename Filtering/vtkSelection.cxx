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

#include <vtkstd/map>
#include <vtkstd/vector>

vtkCxxRevisionMacro(vtkSelection, "1.10");
vtkStandardNewMacro(vtkSelection);

vtkCxxSetObjectMacro(vtkSelection, SelectionList, vtkAbstractArray);

vtkInformationKeyMacro(vtkSelection,CONTENT_TYPE,Integer);
vtkInformationKeyMacro(vtkSelection,SOURCE,ObjectBase);
vtkInformationKeyMacro(vtkSelection,SOURCE_ID,Integer);
vtkInformationKeyMacro(vtkSelection,PROP,ObjectBase);
vtkInformationKeyMacro(vtkSelection,PROP_ID,Integer);
vtkInformationKeyMacro(vtkSelection,PROCESS_ID,Integer);
vtkInformationKeyMacro(vtkSelection,GROUP,Integer);
vtkInformationKeyMacro(vtkSelection,BLOCK,Integer);
vtkInformationKeyMacro(vtkSelection,FIELD_TYPE,Integer);
vtkInformationKeyMacro(vtkSelection,ARRAY_NAME,String);
vtkInformationKeyMacro(vtkSelection,EPSILON,Double);
vtkInformationKeyMacro(vtkSelection,PRESERVE_TOPOLOGY,Integer);
vtkInformationKeyMacro(vtkSelection,CONTAINING_CELLS,Integer);
vtkInformationKeyMacro(vtkSelection,PIXEL_COUNT,Integer);
vtkInformationKeyMacro(vtkSelection,INVERSE,Integer);

struct vtkSelectionInternals
{
  vtkstd::vector<vtkSmartPointer<vtkSelection> > Children;
};


//----------------------------------------------------------------------------
vtkSelection::vtkSelection()
{
  this->Internal = new vtkSelectionInternals;
  this->SelectionList = 0;
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
  if (this->SelectionList)
    {
    this->SelectionList->Delete();
    }
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
  if (this->SelectionList)
    {
    this->SelectionList->Delete();
    }
  this->SelectionList = 0;
  this->Properties->Clear();

  this->Modified();
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
void vtkSelection::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SelectionList:";
  if (this->SelectionList)
    {
    this->SelectionList->PrintSelf(os, indent.GetNextIndent());
    /*
    vtkDataArray *da = vtkDataArray::SafeDownCast(this->SelectionList);
    if (da)
      {
      vtkIdType c = da->GetNumberOfComponents();
      vtkIdType t = da->GetNumberOfTuples();
      for (int i = 0; i < t; i++)
        {
        for (int j = 0; j < c; j++)
          {
          os << indent <<  da->GetComponent(i,j) << " ";
          }
        cerr << endl;
        }
      }
    */
    }
  else
    {
    os << "(none)" << endl;
    }

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

  this->Properties->Copy(input->Properties, 0);
  this->SetSelectionList(input->SelectionList);

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

  this->Properties->Copy(input->Properties, 1);
  if (input->SelectionList)
    {
    this->SelectionList = input->SelectionList->NewInstance();
    this->SelectionList->DeepCopy(input->SelectionList);
    }

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

