/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkInformationQuadratureSchemeDefinitionVectorKey.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkInformationQuadratureSchemeDefinitionVectorKey.h"
#include "vtkInformation.h"
#include "vtkSmartPointer.h"
#include "vtkQuadratureSchemeDefinition.h"
#include "vtkCellType.h"
#include "vtkXMLDataElement.h"
#include <vtkstd/vector>


//============================================================================
class vtkInformationQuadratureSchemeDefinitionVectorValue: public vtkObjectBase
{
public:
  vtkTypeMacro(vtkInformationQuadratureSchemeDefinitionVectorValue, vtkObjectBase);
  //
  vtkInformationQuadratureSchemeDefinitionVectorValue()
  {
    // Typically we have one defintion per cell type.
    this->Vector.resize(VTK_NUMBER_OF_CELL_TYPES);
  }
  // Accessor.
  vtkstd::vector<vtkSmartPointer<vtkQuadratureSchemeDefinition> > &GetVector()
  {
    return this->Vector;
  }
private:
  vtkstd::vector<vtkSmartPointer<vtkQuadratureSchemeDefinition> > Vector;
};

//============================================================================

//----------------------------------------------------------------------------
vtkInformationQuadratureSchemeDefinitionVectorKey::vtkInformationQuadratureSchemeDefinitionVectorKey(
        const char* name, 
        const char* location)
        :
        vtkInformationKey(name, location)
{
  vtkCommonInformationKeyManager::Register(this);
}

//----------------------------------------------------------------------------
vtkInformationQuadratureSchemeDefinitionVectorKey::~vtkInformationQuadratureSchemeDefinitionVectorKey()
{
}

//----------------------------------------------------------------------------
vtkInformationQuadratureSchemeDefinitionVectorValue *
        vtkInformationQuadratureSchemeDefinitionVectorKey::GetQuadratureSchemeDefinitionVector(
                vtkInformation *info)
{
  // Grab the vector associated with this key.
  vtkInformationQuadratureSchemeDefinitionVectorValue* base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));

  // If we don't already have a vector then associated,
  // we will create it here.
  if(base == NULL)
    {
    base=new vtkInformationQuadratureSchemeDefinitionVectorValue;
    this->ConstructClass("vtkInformationQuadratureSchemeDefinitionVectorValue"); // For debug info
    this->SetAsObjectBase(info, base);
    base->Delete();
    }

  return base;
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::Append(
        vtkInformation* info,
        vtkQuadratureSchemeDefinition* aValue)
{
  //
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
  //
  base->GetVector().push_back(aValue);
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::Set(
        vtkInformation* info,
        vtkQuadratureSchemeDefinition *aValue,
        int i)
{
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
  int n=static_cast<int>(base->GetVector().size());
  if (i>=n)
    {
    base->GetVector().resize(i+1);
    }
  // Set.
  base->GetVector()[i]=aValue;
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::SetRange(
        vtkInformation* info,
        vtkQuadratureSchemeDefinition **sourceVec,
        int from,
        int to,
        int n)
{
  // Get the vector associated with this key, resize if this
  // set would run off the end.
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
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
// vtkSmartPointer<vtkQuadratureSchemeDefinition> *vtkInformationQuadratureSchemeDefinitionVectorKey::Get(
//         vtkInformation* info)
// {
//   vtkInformationQuadratureSchemeDefinitionVectorValue* base =
//     static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));
// 
//   return
//     (base!=NULL && !base->GetVector().empty())?(&base->GetVector()[0]):0;
// }


//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::GetRange(
        vtkInformation *info,
        vtkQuadratureSchemeDefinition **dest,
        int from,
        int to,
        int n)
{
  vtkInformationQuadratureSchemeDefinitionVectorValue* base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));

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

  // limit copy to what's there.
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
vtkQuadratureSchemeDefinition *vtkInformationQuadratureSchemeDefinitionVectorKey::Get(
        vtkInformation* info,
        int idx)
{
  vtkInformationQuadratureSchemeDefinitionVectorValue* base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));

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
int vtkInformationQuadratureSchemeDefinitionVectorKey::Size(vtkInformation* info)
{
  vtkInformationQuadratureSchemeDefinitionVectorValue* base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));

  return (base==NULL ? 0 : static_cast<int>(base->GetVector().size()));
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::Resize(vtkInformation* info, int size)
{
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
  base->GetVector().resize(size);
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::Clear(vtkInformation* info)
{
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
  base->GetVector().clear();
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::ShallowCopy(
        vtkInformation* source,
        vtkInformation* dest)
{
  // grab the source vector
  vtkInformationQuadratureSchemeDefinitionVectorValue* sourceBase =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(source));
  // grab failed, just set dest to 0
  if (sourceBase==0)
    {
    this->SetAsObjectBase(dest,0);
    return;
    }
  // Grab the dest vector
  vtkInformationQuadratureSchemeDefinitionVectorValue* destBase=this->GetQuadratureSchemeDefinitionVector(dest);
  // Explicitly size the dest
  int sourceSize=static_cast<int>(sourceBase->GetVector().size());
  destBase->GetVector().resize(sourceSize);
  // Vector operator= copy, using smart ptrs we get ref counted.
  destBase->GetVector()=sourceBase->GetVector();
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::DeepCopy(
        vtkInformation* source,
        vtkInformation* dest)
{
  // Grab the source vector.
  vtkInformationQuadratureSchemeDefinitionVectorValue* sourceBase =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(source));
  // Grab failed, set dest to 0 and bail.
  if (sourceBase==0)
    {
    this->SetAsObjectBase(dest,0);
    return;
    }
  // Grab the dest vector.
  vtkInformationQuadratureSchemeDefinitionVectorValue* destBase=this->GetQuadratureSchemeDefinitionVector(dest);
  // Explicitly size the dest.
  int sourceSize=static_cast<int>(sourceBase->GetVector().size());
  destBase->GetVector().resize(sourceSize);
  // Deep copy each definition.
  for (int i=0; i<sourceSize; ++i)
    {
    vtkQuadratureSchemeDefinition *srcDef=sourceBase->GetVector()[i];
    if (srcDef)
      {
      vtkQuadratureSchemeDefinition *destDef=vtkQuadratureSchemeDefinition::New();
      destDef->DeepCopy(srcDef);
      destBase->GetVector()[i]=destDef;
      destDef->Delete();
      }
    }
}

//-----------------------------------------------------------------------------
int vtkInformationQuadratureSchemeDefinitionVectorKey::SaveState(
        vtkInformation *info,
        vtkXMLDataElement *root)
{
  // Grab the vector associated with this key.
  vtkInformationQuadratureSchemeDefinitionVectorValue* base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));

  // If it doesn't exist or it's empty then we do nothing.
  int dictSize;
  if( base==NULL ||
      (dictSize=static_cast<int>(base->GetVector().size()))==0)
    {
    vtkGenericWarningMacro("Attempting to save an empty or non-existant key/value.");
    return 0;
    }

  // Quick sanity check, we're not nesting rather treating
  // this as a root, to be nested by the caller as needed.
  if (root->GetName()!=NULL
      || root->GetNumberOfNestedElements()>0)
    {
    vtkGenericWarningMacro("Can't save state to non-empty element.");
    return 0;
    }

  // Initialize the key
  root->SetName("InformationKey");
  root->SetAttribute("name", "DICTIONARY");
  root->SetAttribute("location","vtkQuadratureSchemeDefinition");
  // For each item in the array.
  for (int defnId=0; defnId<dictSize; ++defnId)
    {
    // Grab a definition.
    vtkQuadratureSchemeDefinition *def=base->GetVector()[defnId];
    if (def==NULL)
      {
      continue;
      }
    // Nest XML representaion.
    vtkXMLDataElement *e=vtkXMLDataElement::New();
    def->SaveState(e);
    root->AddNestedElement(e);
    e->Delete();
    }
  return 1;
}

//-----------------------------------------------------------------------------
int vtkInformationQuadratureSchemeDefinitionVectorKey::RestoreState(
        vtkInformation *info,
        vtkXMLDataElement *root)
{
  // Grab or create the vector associated with this key.
  vtkInformationQuadratureSchemeDefinitionVectorValue* base=this->GetQuadratureSchemeDefinitionVector(info);
  // clear what ever state we have to avoid confusion.
  base->GetVector().clear();
  base->GetVector().resize(VTK_NUMBER_OF_CELL_TYPES);

  // Quick sanity check to validate that we were passed a
  // valid dictionary tag.
  if ((strcmp(root->GetName(),"InformationKey")!=0)
      || (strcmp(root->GetAttribute("name"),"DICTIONARY")!=0)
      || (strcmp(root->GetAttribute("location"),"vtkQuadratureSchemeDefinition")!=0))
    {
    vtkGenericWarningMacro("State cannot be loaded from <"
                    << root->GetName() << " "
                    << "name=\"" << root->GetAttribute("name") << "\" "
                    << "location=\"" << root->GetAttribute("location") << "\".");
    return 0;
    }
  // Process all nested tags. Each is assumed to be a valid definition
  // tag. If any of the tags are invalid or not definition tags they
  // will be skipped, and warnings will be generated. 
  int nDefns=root->GetNumberOfNestedElements();
  for (int defnId=0; defnId<nDefns; ++defnId)
    {
    vtkXMLDataElement *e=root->GetNestedElement(defnId);
    vtkQuadratureSchemeDefinition *def=vtkQuadratureSchemeDefinition::New();
    if (def->RestoreState(e))
      {
      base->GetVector()[def->GetCellType()]=def;
      }
    def->Delete();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::PrintSelf(
        ostream& os,
        vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void vtkInformationQuadratureSchemeDefinitionVectorKey::Print(
        ostream& os,
        vtkInformation* info)
{
  vtkIndent indent;
  // Grab the vector associated with this key.
  vtkInformationQuadratureSchemeDefinitionVectorValue *base =
    static_cast<vtkInformationQuadratureSchemeDefinitionVectorValue *>(this->GetAsObjectBase(info));
  // Print each valid item.
  if (base!=NULL)
    {
    int n=static_cast<int>(base->GetVector().size());
    for (int i=0; i<n; ++i)
      {
      os << indent << "item " << i << "=";
      vtkQuadratureSchemeDefinition *itemBase=base->GetVector()[i];
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
