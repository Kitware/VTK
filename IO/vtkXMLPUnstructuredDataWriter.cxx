/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataWriter.cxx
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
#include "vtkXMLPUnstructuredDataWriter.h"
#include "vtkXMLUnstructuredDataWriter.h"
#include "vtkPointSet.h"

vtkCxxRevisionMacro(vtkXMLPUnstructuredDataWriter, "1.1");

//----------------------------------------------------------------------------
vtkXMLPUnstructuredDataWriter::vtkXMLPUnstructuredDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPUnstructuredDataWriter::~vtkXMLPUnstructuredDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataWriter::GetInputAsPointSet()
{
  if(this->NumberOfInputs < 1)
    {
    return 0;
    }
  
  return static_cast<vtkPointSet*>(this->Inputs[0]);
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPUnstructuredDataWriter::CreatePieceWriter(int index)
{
  vtkXMLUnstructuredDataWriter* pWriter = this->CreateUnstructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);
  
  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPUnstructuredDataWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  vtkPointSet* input = this->GetInputAsPointSet();  
  this->WritePPoints(input->GetPoints(), indent);
}
