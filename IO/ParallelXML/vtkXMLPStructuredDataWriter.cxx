// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPStructuredDataWriter.h"
#include "vtkCommunicator.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkExecutive.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkXMLStructuredDataWriter.h"

//------------------------------------------------------------------------------
VTK_ABI_NAMESPACE_BEGIN
vtkXMLPStructuredDataWriter::vtkXMLPStructuredDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPStructuredDataWriter::~vtkXMLPStructuredDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::WriteInternal()
{
  int retVal = this->Superclass::WriteInternal();
  if (retVal == 0 || !this->GetContinuingExecution())
  {
    this->Extents.clear();
  }
  return retVal;
}

//------------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::WritePrimaryElementAttributes(ostream& os, vtkIndent indent)
{
  int* wExt =
    this->GetInputInformation(0, 0)->Get(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
  this->WriteVectorAttribute("WholeExtent", 6, wExt);
  this->Superclass::WritePrimaryElementAttributes(os, indent);
}

//------------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::WritePPieceAttributes(int index)
{
  if (this->Extents.find(index) != this->Extents.end())
  {
    this->WriteVectorAttribute("Extent", 6, this->Extents[index].data());
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return;
    }
  }
  this->Superclass::WritePPieceAttributes(index);
}

//------------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPStructuredDataWriter::CreatePieceWriter(int index)
{
  vtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);

  return pWriter;
}

//------------------------------------------------------------------------------
void vtkXMLPStructuredDataWriter::PrepareSummaryFile()
{
  this->Superclass::PrepareSummaryFile();

  // The code below gathers extens from all processes to write in the
  // meta-file. Note that the extent of each piece was already stored by
  // each writer in WritePiece(). This is gathering it all to root node.
  if (this->Controller)
  {
    // Even though the logic is pretty straightforward, we need to
    // do a fair amount of work to use GatherV. Each rank simply
    // serializes its extents to 7 int blocks - piece number and 6
    // extent values. Then we gather this all to root.
    int rank = this->Controller->GetLocalProcessId();
    int nRanks = this->Controller->GetNumberOfProcesses();

    int nPiecesTotal = 0;
    vtkIdType nPieces = static_cast<vtkIdType>(this->Extents.size());

    std::vector<vtkIdType> offsets(nRanks);
    std::vector<vtkIdType> nPiecesAll(nRanks);
    std::vector<vtkIdType> recvLengths(nRanks);

    this->Controller->AllGather(&nPieces, nPiecesAll.data(), 1);
    for (int i = 0; i < nRanks; i++)
    {
      offsets[i] = nPiecesTotal * 7;
      nPiecesTotal += nPiecesAll[i];
      recvLengths[i] = nPiecesAll[i] * 7;
    }

    std::vector<int> sendBuffer;
    int sendSize = nPieces * 7;
    if (nPieces > 0)
    {
      sendBuffer.resize(sendSize);
      ExtentsType::iterator iter = this->Extents.begin();
      for (int count = 0; iter != this->Extents.end(); ++iter, ++count)
      {
        sendBuffer[count * 7] = iter->first;
        memcpy(&sendBuffer[count * 7 + 1], iter->second.data(), 6 * sizeof(int));
      }
    }
    std::vector<int> recvBuffer;
    if (rank == 0)
    {
      recvBuffer.resize(nPiecesTotal * 7);
    }
    this->Controller->GatherV(
      sendBuffer.data(), recvBuffer.data(), sendSize, recvLengths.data(), offsets.data(), 0);

    if (rank == 0)
    {
      // Add all received values to Extents.
      // These are later written in WritePPieceAttributes()
      for (int i = 1; i < nRanks; i++)
      {
        for (int j = 0; j < nPiecesAll[i]; j++)
        {
          int* buffer = recvBuffer.data() + offsets[i] + j * 7;
          this->Extents[*buffer] = std::vector<int>(buffer + 1, buffer + 7);
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::WritePiece(int index)
{
  int result = this->Superclass::WritePiece(index);
  if (result)
  {
    // Store the extent of this piece in Extents. This is later used
    // in WritePPieceAttributes to write the summary file.
    vtkDataSet* input = this->GetDataSetInput();
    int* ext = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());
    this->Extents[index] = std::vector<int>(ext, ext + 6);
  }
  return result;
}
VTK_ABI_NAMESPACE_END
