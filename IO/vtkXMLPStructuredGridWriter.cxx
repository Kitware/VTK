/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridWriter.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPStructuredGridWriter.h"
#include "vtkObjectFactory.h"
#include "vtkXMLStructuredGridWriter.h"
#include "vtkStructuredGrid.h"

vtkCxxRevisionMacro(vtkXMLPStructuredGridWriter, "1.1");
vtkStandardNewMacro(vtkXMLPStructuredGridWriter);

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::vtkXMLPStructuredGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPStructuredGridWriter::~vtkXMLPStructuredGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredGridWriter::SetInput(vtkStructuredGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkStructuredGrid* vtkXMLPStructuredGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkStructuredGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDataSetName()
{
  return "PStructuredGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPStructuredGridWriter::GetDefaultFileExtension()
{
  return "pvts";
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
vtkXMLPStructuredGridWriter::CreateStructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLStructuredGridWriter* pWriter = vtkXMLStructuredGridWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}
