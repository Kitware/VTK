/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridWriter.cxx
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
#include "vtkXMLPRectilinearGridWriter.h"
#include "vtkObjectFactory.h"
#include "vtkXMLRectilinearGridWriter.h"
#include "vtkRectilinearGrid.h"

vtkCxxRevisionMacro(vtkXMLPRectilinearGridWriter, "1.1");
vtkStandardNewMacro(vtkXMLPRectilinearGridWriter);

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::vtkXMLPRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::~vtkXMLPRectilinearGridWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::SetInput(vtkRectilinearGrid* input)
{
  this->vtkProcessObject::SetNthInput(0, input);
}

//----------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridWriter::GetInput()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkRectilinearGrid*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDataSetName()
{
  return "PRectilinearGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDefaultFileExtension()
{
  return "pvtr";
}

//----------------------------------------------------------------------------
vtkXMLStructuredDataWriter*
vtkXMLPRectilinearGridWriter::CreateStructuredPieceWriter()
{  
  // Create the writer for the piece.
  vtkXMLRectilinearGridWriter* pWriter = vtkXMLRectilinearGridWriter::New();
  pWriter->SetInput(this->GetInput());
  return pWriter;
}
