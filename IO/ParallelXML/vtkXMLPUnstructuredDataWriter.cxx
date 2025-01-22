// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPUnstructuredDataWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkPointSet.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLUnstructuredDataWriter.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkXMLPUnstructuredDataWriter::vtkXMLPUnstructuredDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPUnstructuredDataWriter::~vtkXMLPUnstructuredDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPointSet* vtkXMLPUnstructuredDataWriter::GetPointSetInput()
{
  return static_cast<vtkPointSet*>(this->GetInput());
}

//------------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPUnstructuredDataWriter::CreatePieceWriter(int index)
{
  vtkXMLUnstructuredDataWriter* pWriter = this->CreateUnstructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);

  return pWriter;
}

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredDataWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  vtkPointSet* input = this->GetPointSetInput();
  this->WritePPoints(input->GetPoints(), indent);
}
VTK_ABI_NAMESPACE_END
