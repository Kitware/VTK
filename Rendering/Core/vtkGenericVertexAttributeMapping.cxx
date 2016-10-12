/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGenericVertexAttributeMapping.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGenericVertexAttributeMapping.h"

#include "vtkObjectFactory.h"
#include <string>
#include <vector>

#include <sstream>

class vtkGenericVertexAttributeMapping::vtkInternal
{
public:
  struct vtkInfo
  {
    std::string AttributeName;
    std::string ArrayName;
    int FieldAssociation;
    int Component;
    int TextureUnit;
  };

  typedef std::vector<vtkInfo> VectorType;
  VectorType Mappings;
};

vtkStandardNewMacro(vtkGenericVertexAttributeMapping);
//----------------------------------------------------------------------------
vtkGenericVertexAttributeMapping::vtkGenericVertexAttributeMapping()
{
  this->Internal = new vtkInternal();
}

//----------------------------------------------------------------------------
vtkGenericVertexAttributeMapping::~vtkGenericVertexAttributeMapping()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkGenericVertexAttributeMapping::AddMapping(
  const char* attributeName, const char* arrayName, int fieldAssociation,
  int component)
{
  if (!attributeName || !arrayName)
  {
    vtkErrorMacro("arrayName and attributeName cannot be null.");
    return;
  }

  if (this->RemoveMapping(attributeName))
  {
    vtkWarningMacro("Replacing existing mapping for attribute "
      << attributeName);
  }

  vtkInternal::vtkInfo info;
  info.AttributeName = attributeName;
  info.ArrayName = arrayName;
  info.FieldAssociation = fieldAssociation;
  info.Component = component;
  info.TextureUnit = -1;
  this->Internal->Mappings.push_back(info);
}

//----------------------------------------------------------------------------
void vtkGenericVertexAttributeMapping::AddMapping(
  int unit, const char* arrayName, int fieldAssociation,
  int component)
{
  std::ostringstream attributeName;
  attributeName << unit;

  if (this->RemoveMapping(attributeName.str().c_str()))
  {
    vtkWarningMacro("Replacing existing mapping for attribute "
      << attributeName.str().c_str());
  }

  vtkInternal::vtkInfo info;
  info.AttributeName = attributeName.str().c_str();
  info.ArrayName = arrayName;
  info.FieldAssociation = fieldAssociation;
  info.Component = component;
  info.TextureUnit = unit;
  this->Internal->Mappings.push_back(info);
}

//----------------------------------------------------------------------------
bool vtkGenericVertexAttributeMapping::RemoveMapping(const char* attributeName)
{
  vtkInternal::VectorType::iterator iter;
  for (iter=this->Internal->Mappings.begin();
    iter != this->Internal->Mappings.end(); ++iter)
  {
    if (iter->AttributeName == attributeName)
    {
      this->Internal->Mappings.erase(iter);
      return true;
    }
  }
  return false;
}


//----------------------------------------------------------------------------
void vtkGenericVertexAttributeMapping::RemoveAllMappings()
{
  this->Internal->Mappings.clear();
}

//----------------------------------------------------------------------------
unsigned int vtkGenericVertexAttributeMapping::GetNumberOfMappings()
{
  return static_cast<unsigned int>(this->Internal->Mappings.size());
}

//----------------------------------------------------------------------------
const char* vtkGenericVertexAttributeMapping::GetAttributeName(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    vtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].AttributeName.c_str();
}

//----------------------------------------------------------------------------
const char* vtkGenericVertexAttributeMapping::GetArrayName(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    vtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].ArrayName.c_str();
}

//----------------------------------------------------------------------------
int vtkGenericVertexAttributeMapping::GetFieldAssociation(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    vtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].FieldAssociation;
}

//----------------------------------------------------------------------------
int vtkGenericVertexAttributeMapping::GetComponent(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    vtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].Component;
}

//----------------------------------------------------------------------------
int vtkGenericVertexAttributeMapping::GetTextureUnit(unsigned int index)
{
  if (index >= this->Internal->Mappings.size())
  {
    vtkErrorMacro("Invalid index " << index);
    return 0;
  }
  return this->Internal->Mappings[index].TextureUnit;
}

//----------------------------------------------------------------------------
void vtkGenericVertexAttributeMapping::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
   vtkInternal::VectorType::iterator iter;
  for (iter=this->Internal->Mappings.begin();
    iter != this->Internal->Mappings.end(); ++iter)
  {
    os << indent << "Mapping: "
      << iter->AttributeName.c_str() << ", "
      << iter->ArrayName.c_str() << ", "
      << iter->FieldAssociation << ", "
      << iter->Component << endl;
  }
}

