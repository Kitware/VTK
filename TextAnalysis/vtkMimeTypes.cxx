/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMimeTypes.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include <vtkFileExtensionMimeTypeStrategy.h>
#include <vtkMimeTypes.h>
#include <vtkObjectFactory.h>
#include <vtkSmartPointer.h>

#include <boost/algorithm/string.hpp>

#include <algorithm>
#include <vector>

////////////////////////////////////////////////////////////////
// vtkMimeTypes::Implementation

class vtkMimeTypes::Implementation
{
public:
  std::vector<vtkMimeTypeStrategy*> Strategies;
};

////////////////////////////////////////////////////////////////
// vtkMimeTypes

vtkStandardNewMacro(vtkMimeTypes);

vtkMimeTypes::vtkMimeTypes() :
  Internal(new Implementation())
{
  // Add more sophisticated platform-specific strategies here ...
  
  // Last-but-not-least, our fallback strategy is to identify MIME type using file extensions
  this->Internal->Strategies.push_back(vtkFileExtensionMimeTypeStrategy::New());
}

vtkMimeTypes::~vtkMimeTypes()
{
  this->ClearStrategies();
  delete this->Internal;
}

void vtkMimeTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for(unsigned int i = 0; i != this->Internal->Strategies.size(); ++i)
    {
    os << indent << "Strategy: " << endl;
    this->Internal->Strategies[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

void vtkMimeTypes::ClearStrategies()
{
  for(unsigned int i = 0; i != this->Internal->Strategies.size(); ++i)
    this->Internal->Strategies[i]->UnRegister(NULL);
}

void vtkMimeTypes::PrependStrategy(vtkMimeTypeStrategy* strategy)
{
  if(!strategy)
    {
    vtkErrorMacro(<< "Cannot prepend NULL strategy.");
    return;
    }

  if(std::count(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy))
    {
    vtkErrorMacro(<< "Cannot prepend the same strategy twice.");
    return;
    }

  strategy->Register(NULL);
  this->Internal->Strategies.insert(this->Internal->Strategies.begin(), strategy);
}

void vtkMimeTypes::AppendStrategy(vtkMimeTypeStrategy* strategy)
{
  if(!strategy)
    {
    vtkErrorMacro(<< "Cannot append NULL strategy.");
    return;
    }

  if(std::count(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy))
    {
    vtkErrorMacro(<< "Cannot append the same strategy twice.");
    return;
    }

  strategy->Register(NULL);
  this->Internal->Strategies.insert(this->Internal->Strategies.end(), strategy);
}

vtkStdString vtkMimeTypes::Lookup(const vtkStdString& uri)
{
  return this->Lookup(uri, static_cast<const vtkTypeUInt8*>(0), static_cast<const vtkTypeUInt8*>(0));
}

vtkStdString vtkMimeTypes::Lookup(const char* begin, const char* end)
{
  return this->Lookup(vtkStdString(), reinterpret_cast<const vtkTypeUInt8*>(begin), reinterpret_cast<const vtkTypeUInt8*>(end));
}

vtkStdString vtkMimeTypes::Lookup(const vtkTypeUInt8* begin, const vtkTypeUInt8* end)
{
  return this->Lookup(vtkStdString(), begin, end);
}

vtkStdString vtkMimeTypes::Lookup(const vtkStdString& uri, const char* begin, const char* end)
{
  return this->Lookup(uri, reinterpret_cast<const vtkTypeUInt8*>(begin), reinterpret_cast<const vtkTypeUInt8*>(end));
}

vtkStdString vtkMimeTypes::Lookup(const vtkStdString& uri, const vtkTypeUInt8* begin, const vtkTypeUInt8* end)
{
  for(unsigned int i = 0; i != this->Internal->Strategies.size(); ++i)
    {
    const vtkStdString mime_type = this->Internal->Strategies[i]->Lookup(uri, std::min(begin, end), std::max(begin, end));
    if(mime_type.size())
      return mime_type;
    }
  return vtkStdString();
}

bool vtkMimeTypes::Match(const vtkStdString& pattern, const vtkStdString& type)
{
  std::vector<std::string> pattern_parts;
  boost::algorithm::split(pattern_parts, pattern, boost::algorithm::is_any_of("/"));
  if(pattern_parts.size() != 2)
    {
    vtkGenericWarningMacro(<< "Not a valid MIME pattern: " << pattern);
    return false;
    }

  std::vector<std::string> type_parts;
  // Special-case: we treat an empty string as-if it were "<empty>/</empty>"
  if(type.empty())
    {
    type_parts.push_back("");
    type_parts.push_back("");
    }
  else
    {
    boost::algorithm::split(type_parts, type, boost::algorithm::is_any_of("/"));
    }
  if(type_parts.size() != 2)
    {
    vtkGenericWarningMacro(<< "Not a valid MIME type: " << type);
    return false;
    }

  if(pattern_parts[0] != "*")
    {
    if(pattern_parts[0] != type_parts[0])
      return false;
    }

  if(pattern_parts[1] != "*")
    {
    if(pattern_parts[1] != type_parts[1])
      return false;
    }

  return true;
}

