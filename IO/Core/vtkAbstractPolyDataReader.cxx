/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAbstractPolyDataReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION See Also
// vtkOBJReader vtkPLYReader vtkSTLReader
#include "vtkAbstractPolyDataReader.h"

vtkAbstractPolyDataReader::vtkAbstractPolyDataReader()
  : vtkPolyDataAlgorithm()
{
}

vtkAbstractPolyDataReader::~vtkAbstractPolyDataReader()
{
}

void vtkAbstractPolyDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "FileName: " <<
    (this->FileName ? this->FileName : "NONE") << endl;
}
