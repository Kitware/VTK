/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationObjectBaseVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationObjectBaseVectorKey.h"
#include "vtkInformation.h" // For vtkErrorWithObjectMacro
#include "vtkSmartPointer.h"
#include <vtkstd/vector>


//============================================================================
class vtkInformationObjectBaseVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationObjectBaseVectorValue, vtkObjectBase);
  vtkstd::vector<vtkSmartPointer<vtkObjectBase> > &GetVector()
  {
    return this->Vector;
  }
private:
  vtkstd::vector<vtkSmartPointer<vtkObjectBase> > Vector;
};


//============================================================================

//----------------------------------------------------------------------------
vtkInformationObjectBaseVectorKey::vtkInformationObjectBaseVectorKey(
        const char* name, 
        const char* location,
        const char* requiredClass)
        :
        vtkInformationKey(name, location),
        RequiredClass(requiredClass)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationObjectBaseVectorKey::~vtkInformationObjectBaseVectorKey()
{
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::PrintSelf(
        ostream& os,
        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkInformationObjectBaseVectorValue *
        vtkInformationObjectBaseVectorKey::GetObjectBaseVector(vtkInformation *info)
{
  // Grab the vector associated with this key.
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));

  // If we don't already have a vector then associated,
  // we will create it here.
  if(base==NULL)
    {
    base=new vtkInformationObjectBaseVectorValue;
    this->ConstructClass("vtkInformationObjectBaseVectorValue"); // For debug info
    this->SetAsObjectBase(info, base);
    base->Delete();
    }

  return base;
}

//----------------------------------------------------------------------------
bool vtkInformationObjectBaseVectorKey::ValidateDerivedType(
        vtkInformation* info,
        vtkObjectBase* aValue)
{
  // verify that type of aValue is compatible with
  // this conatiner.
  if(aValue!=NULL
     && this->RequiredClass!=NULL
     && !aValue->IsA(this->RequiredClass))
    {
    vtkErrorWithObjectMacro(
      info,
      "Cannot store object of type " << aValue->GetClassName()
      << " with key " << this->Location << "::" << this->Name
      << " which requires objects of type "
      << this->RequiredClass << ".");
    return false;
    }
  return true;
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Append(
        vtkInformation* info,
        vtkObjectBase* aValue)
{
  if (!this->ValidateDerivedType(info,aValue))
    {
    return;
    }
  //
  vtkInformationObjectBaseVectorValue* base=this->GetObjectBaseVector(info);
  //
  if (aValue!=NULL)
    {
    aValue->Register(base);
    }
  //
  base->GetVector().push_back(aValue);
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Set(
        vtkInformation* info,
        vtkObjectBase *aValue,
        int i)
{
  if (!this->ValidateDerivedType(info,aValue))
    {
    return;
    }
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationObjectBaseVectorValue* base=this->GetObjectBaseVector(info);
  int n=static_cast<int>(base->GetVector().size());
  if (i>=n)
    {
    base->GetVector().resize(i+1);
    }
  // Set.
  base->GetVector()[i]=aValue;
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::SetRange(
        vtkInformation* info,
        vtkObjectBase **sourceVec,
        int from,
        int to,
        int n)
{
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationObjectBaseVectorValue* base=this->GetObjectBaseVector(info);
  int m=static_cast<int>(base->GetVector().size());
  int reqsz=to+n;
  if (reqsz>m)
    {
    base->GetVector().resize(reqsz);
    }
  // Set.
  for (int i=0; i<n; ++i, ++from, ++to)
    {
    base->GetVector()[to]=sourceVec[from];
    }
}

// //----------------------------------------------------------------------------
// vtkSmartPointer<vtkObjectBase> *vtkInformationObjectBaseVectorKey::Get(
//         vtkInformation* info)
// {
//   vtkInformationObjectBaseVectorValue* base =
//     static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));
// 
//   return
//     (base!=NULL && !base->GetVector().empty())?(&base->GetVector()[0]):0;
// }


//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::GetRange(
        vtkInformation *info,
        vtkObjectBase **dest,
        int from,
        int to,
        int n)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));

  // Source vector exists?
  if (base==NULL)
    {
    vtkErrorWithObjectMacro(
      info,"Copy of empty vector has been requested.");
    return;
    }

  int m=static_cast<int>(base->GetVector().size());
  // check source start.
  if (from>=m)
    {
    vtkErrorWithObjectMacro(
      info,"Copy starting past the end of the vector has been requested.");
    return;
    }

  // limit copy to whats there.
  if (n>m-from+1)
    {
    vtkErrorWithObjectMacro(
      info,"Copy past the end of the vector has been requested.");
    n=m-from+1;
    }

  // copy
  for (int i=0; i<n; ++i, ++from, ++to)
    {
    dest[to]=base->GetVector()[from];
    }
}

//----------------------------------------------------------------------------
vtkObjectBase *vtkInformationObjectBaseVectorKey::Get(
        vtkInformation* info,
        int idx)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));

  if (base==NULL
      || idx>=static_cast<int>(base->GetVector().size()))
    {
    vtkErrorWithObjectMacro(info,
      "Information does not contain " << idx
      << " elements. Cannot return information value.");
    return NULL;
    }

  return base->GetVector()[idx];
}

//----------------------------------------------------------------------------
int vtkInformationObjectBaseVectorKey::Size(vtkInformation* info)
{
  vtkInformationObjectBaseVectorValue* base =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));

  return (base==NULL ? 0 : static_cast<int>(base->GetVector().size()));
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Resize(vtkInformation* info, int size)
{
  vtkInformationObjectBaseVectorValue* base=this->GetObjectBaseVector(info);
  base->GetVector().resize(size);
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Clear(vtkInformation* info)
{
  vtkInformationObjectBaseVectorValue* base=this->GetObjectBaseVector(info);
  base->GetVector().clear();
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::ShallowCopy(
        vtkInformation* source,
        vtkInformation* dest)
{
  vtkInformationObjectBaseVectorValue* sourceBase =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(source));

  if (sourceBase==0)
    {
    this->SetAsObjectBase(dest,0);
    return;
    }

  int sourceSize=static_cast<int>(sourceBase->GetVector().size());
  vtkInformationObjectBaseVectorValue* destBase=this->GetObjectBaseVector(dest);

  destBase->GetVector().resize(sourceSize);
  destBase->GetVector()=sourceBase->GetVector();
}

//----------------------------------------------------------------------------
void vtkInformationObjectBaseVectorKey::Print(ostream& os, vtkInformation* info)
{
  vtkIndent indent;
  // Grab the vector associated with this key.
  vtkInformationObjectBaseVectorValue *base =
    static_cast<vtkInformationObjectBaseVectorValue *>(this->GetAsObjectBase(info));
  // Print each valid item.
  if (base!=NULL)
    {
    int n=static_cast<int>(base->GetVector().size());
    if (n>0)
      {
      vtkObjectBase *itemBase=base->GetVector()[0];
      os << indent << "item " << 0 << "=";
      itemBase->PrintSelf(os,indent);
      os << endl;
      }
    for (int i=1; i<n; ++i)
      {
      os << indent << "item " << i << "=";
      vtkObjectBase *itemBase=base->GetVector()[i];
      if (itemBase!=NULL)
        {
        itemBase->PrintSelf(os,indent);
        }
      else
        {
        os << "NULL;";
        }
      os << endl;
      }
    }
}
