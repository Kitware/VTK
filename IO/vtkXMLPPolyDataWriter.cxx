/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPPolyDataWriter.h"
#include "vtkObjectFactory.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkPolyData.h"

vtkCxxRevisionMacro(vtkXMLPPolyDataWriter, "1.2");
vtkStandardNewMacro(vtkXMLPPolyDataWriter);

//----------------------------------------------------------------------------
vtkXMLPPolyDataWriter::vtkXMLPPolyDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPPolyDataWriter::~vtkXMLPPolyDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPPolyDataWriter::SetInput(vtkPolyData* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkPolyData* vtkXMLPPolyDataWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkPolyData*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPPolyDataWriter::GetDataSetName()
{
  return "PPolyData";
}

//----------------------------------------------------------------------------
const char* vtkXMLPPolyDataWriter::GetDefaultFileExtension()
{
  return "pvtp";
}

//----------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter*
vtkXMLPPolyDataWriter::CreateUnstructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLPolyDataWriter* pWriter = vtkXMLPolyDataWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}
