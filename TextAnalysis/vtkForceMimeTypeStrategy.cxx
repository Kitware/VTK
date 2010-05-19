/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkForceMimeTypeStrategy.cxx

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

#include <vtkForceMimeTypeStrategy.h>
#include <vtkObjectFactory.h>
#include <vtkStdString.h>

#include <boost/algorithm/string.hpp>

vtkStandardNewMacro(vtkForceMimeTypeStrategy);

vtkForceMimeTypeStrategy::vtkForceMimeTypeStrategy() :
  MimeType(0)
{
  this->SetMimeType("text/plain");
}

vtkForceMimeTypeStrategy::~vtkForceMimeTypeStrategy()
{
  this->SetMimeType(0);
}

void vtkForceMimeTypeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "MimeType: " << (this->MimeType ? this->MimeType : "(none)") << endl;
}

vtkStdString vtkForceMimeTypeStrategy::Lookup(
  const vtkStdString& vtkNotUsed(uri), 
  const vtkTypeUInt8* vtkNotUsed(begin), 
  const vtkTypeUInt8* vtkNotUsed(end))
{
  return vtkStdString(this->MimeType ? this->MimeType : "");
}

