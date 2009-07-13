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

#include <vector>

////////////////////////////////////////////////////////////////
// vtkMimeTypes::implementation

class vtkMimeTypes::implementation
{
public:
  implementation()
  {
    // Add more sophisticated platform-specific strategies here ...
    
    // Last-but-not-least, our fallback strategy is to identify MIME type using file extensions
    Strategies.push_back(vtkFileExtensionMimeTypeStrategy::New());
  }

  ~implementation()
  {
    for(unsigned int i = 0; i != this->Strategies.size(); ++i)
      this->Strategies[i]->Delete();
  }

  vtkstd::vector<vtkMimeTypeStrategy*> Strategies;
};

////////////////////////////////////////////////////////////////
// vtkMimeTypes

vtkCxxRevisionMacro(vtkMimeTypes, "1.2");
vtkStandardNewMacro(vtkMimeTypes);

vtkMimeTypes::vtkMimeTypes() :
  Implementation(new implementation())
{
}

vtkMimeTypes::~vtkMimeTypes()
{
  delete this->Implementation;
}

void vtkMimeTypes::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  for(unsigned int i = 0; i != this->Implementation->Strategies.size(); ++i)
    {
    os << indent << "Strategy: " << endl;
    this->Implementation->Strategies[i]->PrintSelf(os, indent.GetNextIndent());
    }
}

vtkStdString vtkMimeTypes::Lookup(const vtkStdString& path)
{
  for(unsigned int i = 0; i != this->Implementation->Strategies.size(); ++i)
    {
    const vtkStdString mime_type = this->Implementation->Strategies[i]->Lookup(path);
    if(mime_type.size())
      return mime_type;
    }
  return vtkStdString();
}

