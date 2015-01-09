/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRepresentationPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRepresentationPainter.h"

#include "vtkObjectFactory.h"

// Return NULL if no override is supplied.
vtkAbstractObjectFactoryNewMacro(vtkRepresentationPainter)
//-----------------------------------------------------------------------------
vtkRepresentationPainter::vtkRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
vtkRepresentationPainter::~vtkRepresentationPainter()
{
}

//-----------------------------------------------------------------------------
void vtkRepresentationPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
