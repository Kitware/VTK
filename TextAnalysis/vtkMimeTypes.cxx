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

#include <vtkstd/algorithm>
#include <vtkstd/vector>

////////////////////////////////////////////////////////////////
// vtkMimeTypes::Implementation

class vtkMimeTypes::Implementation
{
public:
  vtkstd::vector<vtkMimeTypeStrategy*> Strategies;
};

////////////////////////////////////////////////////////////////
// vtkMimeTypes

vtkCxxRevisionMacro(vtkMimeTypes, "1.5");
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
    this->Internal->Strategies[i]->Delete();
}

void vtkMimeTypes::PrependStrategy(vtkMimeTypeStrategy* strategy)
{
  this->Internal->Strategies.erase(
    vtkstd::remove(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy),
    this->Internal->Strategies.end());

  this->Internal->Strategies.insert(this->Internal->Strategies.begin(), strategy);
}

void vtkMimeTypes::AppendStrategy(vtkMimeTypeStrategy* strategy)
{
  this->Internal->Strategies.erase(
    vtkstd::remove(this->Internal->Strategies.begin(), this->Internal->Strategies.end(), strategy),
    this->Internal->Strategies.end());

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
    const vtkStdString mime_type = this->Internal->Strategies[i]->Lookup(uri, vtkstd::min(begin, end), vtkstd::max(begin, end));
    if(mime_type.size())
      return mime_type;
    }
  return vtkStdString();
}

