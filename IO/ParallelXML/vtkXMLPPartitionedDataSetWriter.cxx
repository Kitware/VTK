/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPartitionedDataSetWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Hide VTK_DEPRECATED_IN_9_1_0() warnings for this class.
#define VTK_DEPRECATION_LEVEL 0

#include "vtkXMLPPartitionedDataSetWriter.h"

#include "vtkObjectFactory.h"

//------------------------------------------------------------------------------
vtkStandardNewMacro(vtkXMLPPartitionedDataSetWriter);

//------------------------------------------------------------------------------
vtkXMLPPartitionedDataSetWriter::vtkXMLPPartitionedDataSetWriter()
{
  VTK_LEGACY_REPLACED_BODY(
    vtkXMLPPartitionedDataSetWriter, "VTK 9.1", vtkXMLPPartitionedDataSetWriter);
}

//------------------------------------------------------------------------------
vtkXMLPPartitionedDataSetWriter::~vtkXMLPPartitionedDataSetWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPPartitionedDataSetWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
