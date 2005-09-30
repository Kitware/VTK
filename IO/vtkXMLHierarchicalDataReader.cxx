/*=========================================================================

  Program:   ParaView
  Module:    vtkXMLHierarchicalDataReader.cxx

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLHierarchicalDataReader.h"

#include "vtkCompositeDataPipeline.h"
#include "vtkHierarchicalDataSet.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkXMLHierarchicalDataReader, "1.5");
vtkStandardNewMacro(vtkXMLHierarchicalDataReader);

//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::vtkXMLHierarchicalDataReader()
{
}

//----------------------------------------------------------------------------
vtkXMLHierarchicalDataReader::~vtkXMLHierarchicalDataReader()
{
}

//----------------------------------------------------------------------------
void vtkXMLHierarchicalDataReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
const char* vtkXMLHierarchicalDataReader::GetDataSetName()
{
  return "vtkHierarchicalDataSet";
}

