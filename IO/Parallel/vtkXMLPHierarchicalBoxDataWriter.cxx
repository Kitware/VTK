/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPHierarchicalBoxDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPHierarchicalBoxDataWriter.h"

#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPHierarchicalBoxDataWriter);

//----------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::vtkXMLPHierarchicalBoxDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPHierarchicalBoxDataWriter::~vtkXMLPHierarchicalBoxDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPHierarchicalBoxDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
