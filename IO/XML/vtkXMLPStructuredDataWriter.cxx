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
#include "vtkErrorCode.h"
#include "vtkDataSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
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
  int* wExt = this->GetInputInformation(0, 0)->Get(
    vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  this->WriteVectorAttribute("WholeExtent", 6, wExt);
  this->Superclass::WritePrimaryElementAttributes(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::WritePPieceAttributes(int index)
{
  vtkDataSet* input = this->GetInputAsDataSet();
  int* extent = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());

  this->WriteVectorAttribute("Extent", 6, extent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
    return;
    }
  this->Superclass::WritePPieceAttributes(index);
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPStructuredDataWriter::CreatePieceWriter(int vtkNotUsed(index))
{
  // int extent[6];
  // vtkInformation* inInfo = this->GetExecutive()->GetInputInformation(0, 0);
  // // TODO (berk)
  // // Need to fix this
  // vtkExtentTranslator* et = 0;  vtkExtentTranslator::SafeDownCast(
  //   inInfo->Get(vtkStreamingDemandDrivenPipeline::EXTENT_TRANSLATOR()));

  // et->SetNumberOfPieces(this->GetNumberOfPieces());
  // et->SetWholeExtent(inInfo->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()));
  // et->SetPiece(index);
  // et->SetGhostLevel(0);
  // et->PieceToExtent();
  // et->GetExtent(extent);

  // TODO (berk)
  // We need to ask for a piece from the input somehow. Maybe RequestUpdateExtent
  // is enough but needs to be tested.
  vtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  // pWriter->SetWriteExtent(extent);

  return pWriter;
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::ProcessRequest(vtkInformation* request,
                                                vtkInformationVector** inputVector,
                                                vtkInformationVector* outputVector)
{
  // generate the data
  if(request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
    {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
    }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);

  // The code below asks for an extent based on the number of pieces and
  // the first piece. This is mainly for the sake of other filters/writers
  // that may internally use this writer. Those writers usually delegate
  // RequestUpdateExtent() to the internal writer and expect it to do the
  // right thing. Before this change, vtkXMLPStructuredDataWriter did not
  // bother setting the update extent because it is taken care of by the
  // vtkXMLStructuredDataWriter during WritePiece() (see vtkXMLPDataWriter).
  // This is fine when vtkXMLPStructuredDataWriter's input is connected
  // to the actual pipeline but causes problems when it is not, which happens
  // in the situation described above. Here I picked the StartPiece as
  // the default. This will not effect the behavior when writing multiple
  // pieces because that does its own RequestUpdateExtent with the right
  // piece information.

  vtkStreamingDemandDrivenPipeline::SetUpdateExtent(inInfo,
    this->StartPiece, this->GetNumberOfPieces(), 0);

  return 1;
}
