/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIndent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkIndent.h"
#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkIndent* vtkIndent::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkIndent");
  if(ret)
    {
    return reinterpret_cast<vtkIndent*>(ret);
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkIndent;
}


#define VTK_STD_INDENT 2
#define VTK_NUMBER_OF_BLANKS 40

static const char blanks[VTK_NUMBER_OF_BLANKS+1]="                                        ";

// Determine the next indentation level. Keep indenting by two until the
// max of forty.
vtkIndent vtkIndent::GetNextIndent()
{
  int indent = this->Indent + VTK_STD_INDENT;
  if ( indent > VTK_NUMBER_OF_BLANKS )
    {
    indent = VTK_NUMBER_OF_BLANKS;
    }
  return vtkIndent(indent);
}

// Print out the indentation. Basically output a bunch of spaces.
ostream& operator<<(ostream& os, const vtkIndent& ind)
{
  os << blanks + (VTK_NUMBER_OF_BLANKS-ind.Indent) ;
  return os;
}

