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
#include "vtkExecutive.h"
#include "vtkExtentTranslator.h"
#include "vtkErrorCode.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkStreamingDemandDrivenPipeline.h"

//----------------------------------------------------------------------------
vtkXMLPStructuredDataWriter::vtkXMLPStructuredDataWriter()
{
}

//----------------------------------------------------------------------------
vtkXMLPStructuredDataWriter::~vtkXMLPStructuredDataWriter()
{
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
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
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkExtentTranslator* et = vtkExtentTranslator::SafeDownCast(
    inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));

  et->SetNumberOfPieces(this->GetNumberOfPieces());
  et->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  et->SetPiece(index);
  et->SetGhostLevel(0);
  et->PieceToExtent();
  et->GetExtent(extent);

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
  int extent[6];
  vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  vtkExtentTranslator* et = vtkExtentTranslator::SafeDownCast(
    inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));

  et->SetNumberOfPieces(this->GetNumberOfPieces());
  et->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  et->SetPiece(index);
  et->SetGhostLevel(0);
  et->PieceToExtent();
  et->GetExtent(extent);

  vtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  pWriter->SetWriteExtent(extent);

  return pWriter;
}
