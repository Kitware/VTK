/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAssertPlainTextMimeTypeStrategy.cxx

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

#include <vtkAssertPlainTextMimeTypeStrategy.h>
#include <vtkObjectFactory.h>
#include <vtkStdString.h>

#include <boost/algorithm/string.hpp>

vtkCxxRevisionMacro(vtkAssertPlainTextMimeTypeStrategy, "1.1");
vtkStandardNewMacro(vtkAssertPlainTextMimeTypeStrategy);

vtkAssertPlainTextMimeTypeStrategy::vtkAssertPlainTextMimeTypeStrategy()
{
}

vtkAssertPlainTextMimeTypeStrategy::~vtkAssertPlainTextMimeTypeStrategy()
{
}

void vtkAssertPlainTextMimeTypeStrategy::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

vtkStdString vtkAssertPlainTextMimeTypeStrategy::Lookup(
  const vtkStdString& vtkNotUsed(uri), 
  const vtkTypeUInt8* vtkNotUsed(begin), 
  const vtkTypeUInt8* vtkNotUsed(end))
{
  return vtkStdString("text/plain");
}
