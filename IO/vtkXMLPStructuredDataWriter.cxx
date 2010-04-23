/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPStructuredDataWriter.h"
#include "vtkXMLStructuredDataWriter.h"
#include "vtkExtentTranslator.h"
#include "vtkErrorCode.h"
#include "vtkDataSet.h"

vtkCxxSetObjectMacro(vtkXMLPStructuredDataWriter, ExtentTranslator,
                     vtkExtentTranslator);

//----------------------------------------------------------------------------
vtkXMLPStructuredDataWriter::vtkXMLPStructuredDataWriter()
{
  this->ExtentTranslator = vtkExtentTranslator::New();
}

//----------------------------------------------------------------------------
vtkXMLPStructuredDataWriter::~vtkXMLPStructuredDataWriter()
{
  if (this->ExtentTranslator)
    {
    this->ExtentTranslator->Delete();
    this->ExtentTranslator = 0;
    }
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  if(this->ExtentTranslator)
    {
    os << indent << "ExtentTranslator: " << this->ExtentTranslator << "\n";
    }
  else
    {
    os << indent << "ExtentTranslator: (none)\n";
    }
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::WritePrimaryElementAttributes(ostream &os, vtkIndent indent)
{
  vtkDataSet* input = this->GetInputAsDataSet();  
  this->WriteVectorAttribute("WholeExtent", 6, input->GetWholeExtent());
  this->Superclass::WritePrimaryElementAttributes(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::WritePPieceAttributes(int index)
{
  int extent[6];
  this->ExtentTranslator->SetPiece(index);
  this->ExtentTranslator->SetGhostLevel(0);
  this->ExtentTranslator->PieceToExtent();
  this->ExtentTranslator->GetExtent(extent);
  
  this->WriteVectorAttribute("Extent", 6, extent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  this->Superclass::WritePPieceAttributes(index);
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPStructuredDataWriter::CreatePieceWriter(int index)
{
  vtkDataSet* input = this->GetInputAsDataSet();
  
  this->ExtentTranslator->SetWholeExtent(input->GetWholeExtent());
  this->ExtentTranslator->SetNumberOfPieces(this->NumberOfPieces);
  this->ExtentTranslator->SetPiece(index);
  this->ExtentTranslator->SetGhostLevel(this->GhostLevel);
  this->ExtentTranslator->PieceToExtent();  
  
  vtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  pWriter->SetWriteExtent(this->ExtentTranslator->GetExtent());
  
  return pWriter;
}
