/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPUnstructuredGridWriter.h"
#include "vtkObjectFactory.h"
#include "vtkXMLUnstructuredGridWriter.h"
#include "vtkUnstructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLPUnstructuredGridWriter, "1.2");
vtkStandardNewMacro(vtkXMLPUnstructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::vtkXMLPUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::~vtkXMLPUnstructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredGridWriter::SetInput(vtkUnstructuredGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkUnstructuredGrid* vtkXMLPUnstructuredGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkUnstructuredGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "pvtu";
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter*
vtkXMLPUnstructuredGridWriter::CreateUnstructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLUnstructuredGridWriter* pWriter = vtkXMLUnstructuredGridWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}
