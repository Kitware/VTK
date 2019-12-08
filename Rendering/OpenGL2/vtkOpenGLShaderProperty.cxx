/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkShaderProperty.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLShaderProperty.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLUniforms.h"
#include <algorithm>

vtkStandardNewMacro(vtkOpenGLShaderProperty);

vtkOpenGLShaderProperty::vtkOpenGLShaderProperty() = default;

vtkOpenGLShaderProperty::~vtkOpenGLShaderProperty() = default;

void vtkOpenGLShaderProperty::DeepCopy(vtkOpenGLShaderProperty* p)
{
  vtkShaderProperty::DeepCopy(p);
  this->UserShaderReplacements = p->UserShaderReplacements;
}

void vtkOpenGLShaderProperty::AddVertexShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    vtkShader::Vertex, originalValue, replaceFirst, replacementValue, replaceAll);
}

void vtkOpenGLShaderProperty::AddFragmentShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    vtkShader::Fragment, originalValue, replaceFirst, replacementValue, replaceAll);
}

void vtkOpenGLShaderProperty::AddGeometryShaderReplacement(const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  this->AddShaderReplacement(
    vtkShader::Geometry, originalValue, replaceFirst, replacementValue, replaceAll);
}

int vtkOpenGLShaderProperty::GetNumberOfShaderReplacements()
{
  return static_cast<int>(UserShaderReplacements.size());
}

std::string vtkOpenGLShaderProperty::GetNthShaderReplacementTypeAsString(vtkIdType index)
{
  if (index >= static_cast<vtkIdType>(this->UserShaderReplacements.size()))
  {
    vtkErrorMacro(<< "Trying to access out of bound shader replacement.");
    return std::string("");
  }
  ReplacementMap::iterator it = this->UserShaderReplacements.begin();
  std::advance(it, index);
  if (it->first.ShaderType == vtkShader::Vertex)
  {
    return std::string("Vertex");
  }
  else if (it->first.ShaderType == vtkShader::Fragment)
  {
    return std::string("Fragment");
  }
  else if (it->first.ShaderType == vtkShader::Geometry)
  {
    return std::string("Geometry");
  }
  return std::string("Unknown");
}

void vtkOpenGLShaderProperty::GetNthShaderReplacement(vtkIdType index, std::string& name,
  bool& replaceFirst, std::string& replacementValue, bool& replaceAll)
{
  if (index >= static_cast<vtkIdType>(this->UserShaderReplacements.size()))
  {
    vtkErrorMacro(<< "Trying to access out of bound shader replacement.");
  }
  ReplacementMap::iterator it = this->UserShaderReplacements.begin();
  std::advance(it, index);
  name = it->first.OriginalValue;
  replaceFirst = it->first.ReplaceFirst;
  replacementValue = it->second.Replacement;
  replaceAll = it->second.ReplaceAll;
}

void vtkOpenGLShaderProperty::ClearVertexShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(vtkShader::Vertex, originalValue, replaceFirst);
}

void vtkOpenGLShaderProperty::ClearFragmentShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(vtkShader::Fragment, originalValue, replaceFirst);
}

void vtkOpenGLShaderProperty::ClearGeometryShaderReplacement(
  const std::string& originalValue, bool replaceFirst)
{
  this->ClearShaderReplacement(vtkShader::Geometry, originalValue, replaceFirst);
}

void vtkOpenGLShaderProperty::ClearAllVertexShaderReplacements()
{
  this->ClearAllShaderReplacements(vtkShader::Vertex);
}

void vtkOpenGLShaderProperty::ClearAllFragmentShaderReplacements()
{
  this->ClearAllShaderReplacements(vtkShader::Fragment);
}

void vtkOpenGLShaderProperty::ClearAllGeometryShaderReplacements()
{
  this->ClearAllShaderReplacements(vtkShader::Geometry);
}

//-----------------------------------------------------------------------------
void vtkOpenGLShaderProperty::ClearAllShaderReplacements()
{
  this->SetVertexShaderCode(nullptr);
  this->SetFragmentShaderCode(nullptr);
  this->SetGeometryShaderCode(nullptr);
  this->UserShaderReplacements.clear();
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLShaderProperty::AddShaderReplacement(
  vtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue,
  bool replaceFirst, // do this replacement before the default
  const std::string& replacementValue, bool replaceAll)
{
  vtkShader::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  vtkShader::ReplacementValue values;
  values.Replacement = replacementValue;
  values.ReplaceAll = replaceAll;

  this->UserShaderReplacements[spec] = values;
  this->Modified();
}

//-----------------------------------------------------------------------------
void vtkOpenGLShaderProperty::ClearShaderReplacement(
  vtkShader::Type shaderType, // vertex, fragment, etc
  const std::string& originalValue, bool replaceFirst)
{
  vtkShader::ReplacementSpec spec;
  spec.ShaderType = shaderType;
  spec.OriginalValue = originalValue;
  spec.ReplaceFirst = replaceFirst;

  typedef std::map<vtkShader::ReplacementSpec, vtkShader::ReplacementValue>::iterator RIter;
  RIter found = this->UserShaderReplacements.find(spec);
  if (found != this->UserShaderReplacements.end())
  {
    this->UserShaderReplacements.erase(found);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLShaderProperty::ClearAllShaderReplacements(vtkShader::Type shaderType)
{
  bool modified = false;
  // First clear all shader code
  if ((shaderType == vtkShader::Vertex) && this->VertexShaderCode)
  {
    this->SetVertexShaderCode(nullptr);
    modified = true;
  }
  else if ((shaderType == vtkShader::Fragment) && this->FragmentShaderCode)
  {
    this->SetFragmentShaderCode(nullptr);
    modified = true;
  }

  // Now clear custom tag replacements
  std::map<vtkShader::ReplacementSpec, vtkShader::ReplacementValue>::iterator rIter;
  for (rIter = this->UserShaderReplacements.begin(); rIter != this->UserShaderReplacements.end();)
  {
    if (rIter->first.ShaderType == shaderType)
    {
      this->UserShaderReplacements.erase(rIter++);
      modified = true;
    }
    else
    {
      ++rIter;
    }
  }
  if (modified)
  {
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
void vtkOpenGLShaderProperty::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
