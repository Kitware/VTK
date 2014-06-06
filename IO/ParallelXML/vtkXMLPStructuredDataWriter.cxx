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
#include "vtkCommunicator.h"
#include "vtkMultiProcessController.h"

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
int vtkXMLPStructuredDataWriter::WriteInternal()
{
  int retVal = this->Superclass::WriteInternal();
  this->Extents.clear();
  return retVal;
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
  if (this->Extents.find(index) != this->Extents.end())
    {
    this->WriteVectorAttribute("Extent", 6, &this->Extents[index][0]);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
      {
      return;
      }
    }
  this->Superclass::WritePPieceAttributes(index);
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPStructuredDataWriter::CreatePieceWriter(int index)
{
  vtkXMLStructuredDataWriter* pWriter = this->CreateStructuredPieceWriter();
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);
  pWriter->SetGhostLevel(this->GhostLevel);

  return pWriter;
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::WritePieces()
{
  int result = this->Superclass::WritePieces();

  // The code below gathers extens from all processes to write in the
  // meta-file. Note that the extent of each piece was already stored by
  // each writer in WritePiece(). This is gathering it all to root node.
  if (result)
    {
    if (this->Controller)
      {
      // Even though the logic is pretty straightforward, we need to
      // do a fair amount of work to use GatherV. Each rank simply
      // serializes its extents to 7 int blocks - piece number and 6
      // extent values. Then we gather this all to root.
      int rank = this->Controller->GetLocalProcessId();
      int nRanks = this->Controller->GetNumberOfProcesses();

      int nPiecesTotal = 0;
      vtkIdType nPieces = this->Extents.size();

      vtkIdType* offsets = 0;
      vtkIdType* nPiecesAll = 0;
      vtkIdType* recvLengths = 0;
      if (rank == 0)
        {
        nPiecesAll = new vtkIdType[nRanks];
        recvLengths = new vtkIdType[nRanks];
        offsets = new vtkIdType[nRanks];
        }
      this->Controller->Gather(&nPieces, nPiecesAll, 1, 0);
      if (rank == 0)
        {
        for (int i=0; i<nRanks; i++)
          {
          offsets[i] = nPiecesTotal*7;
          nPiecesTotal += nPiecesAll[i];
          recvLengths[i] = nPiecesAll[i]*7;
          }
        }
      int* sendBuffer = 0;
      int sendSize = nPieces*7;
      if (nPieces > 0)
        {
        sendBuffer = new int[sendSize];
        ExtentsType::iterator iter = this->Extents.begin();
        for (int count = 0; iter != this->Extents.end(); iter++, count++)
          {
          sendBuffer[count*7] = iter->first;
          memcpy(&sendBuffer[count*7+1], &iter->second[0], 6*sizeof(int));
          }
        }
      int* recvBuffer = 0;
      if (rank == 0)
        {
        recvBuffer = new int[nPiecesTotal*7];
        }
      this->Controller->GatherV(sendBuffer, recvBuffer, sendSize,
        recvLengths, offsets, 0);

      if (rank == 0)
        {
        // Add all received values to Extents.
        // These are later written in WritePPieceAttributes()
        for (int i=1; i<nRanks; i++)
          {
          for (int j=0; j<nPiecesAll[i]; j++)
            {
            int* buffer = recvBuffer + offsets[i] + j*7;
            this->Extents[*buffer] =
              std::vector<int>(buffer+1, buffer+7);
            }
          }
        }

      delete[] nPiecesAll;
      delete[] recvBuffer;
      delete[] offsets;
      delete[] recvLengths;
      delete[] sendBuffer;
      }
    }
  return result;
}

//----------------------------------------------------------------------------
int vtkXMLPStructuredDataWriter::WritePiece(int index)
{
  int result = this->Superclass::WritePiece(index);
  if (result)
    {
    // Store the extent of this piece in Extents. This is later used
    // in WritePPieceAttributes to write the summary file.
    vtkDataSet* input = this->GetInputAsDataSet();
    int* ext = input->GetInformation()->Get(vtkDataObject::DATA_EXTENT());
    this->Extents[index] = std::vector<int>(ext, ext+6);
    }
  return result;
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
    this->StartPiece, this->GetNumberOfPieces(), this->GhostLevel);

  return 1;
}
