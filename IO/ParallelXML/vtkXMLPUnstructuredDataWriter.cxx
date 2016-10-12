/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPUnstructuredDataWriter.h"

#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkErrorCode.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLUnstructuredDataWriter.h"


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
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataWriter::GetInputAsPointSet()
{
  return static_cast<vtkPointSet*>(this->GetInput());
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
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  vtkPointSet* input = this->GetInputAsPointSet();
  this->WritePPoints(input->GetPoints(), indent);
}
