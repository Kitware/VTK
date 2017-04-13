/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPDataWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkDataSet.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cassert>
#include <sstream>
#include <vtksys/SystemTools.hxx>

vtkCxxSetObjectMacro(vtkXMLPDataWriter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkXMLPDataWriter::vtkXMLPDataWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteSummaryFile = 1;

  this->UseSubdirectory = false;

  this->PathName = 0;
  this->FileNameBase = 0;
  this->FileNameExtension = 0;
  this->PieceFileNameExtension = 0;

  // Setup a callback for the internal writer to report progress.
  this->ProgressObserver = vtkCallbackCommand::New();
  this->ProgressObserver->SetCallback(
    &vtkXMLPDataWriter::ProgressCallbackFunction);
  this->ProgressObserver->SetClientData(this);

  this->Controller = 0;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->ContinuingExecution = false;
  this->CurrentPiece = -1;
  this->PieceWrittenFlags = NULL;
}

//----------------------------------------------------------------------------
vtkXMLPDataWriter::~vtkXMLPDataWriter()
{
  delete [] this->PathName;
  delete [] this->FileNameBase;
  delete [] this->FileNameExtension;
  delete [] this->PieceFileNameExtension;
  delete [] this->PieceWrittenFlags;
  this->SetController(0);
  this->ProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "StartPiece: " << this->StartPiece << "\n";
  os << indent << "EndPiece: " << this->EndPiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
  os << indent << "WriteSummaryFile: " << this->WriteSummaryFile << "\n";
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::ProcessRequest(vtkInformation* request,
                                 vtkInformationVector** inputVector,
                                 vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  int retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);
  if (request->Has(vtkDemandDrivenPipeline::REQUEST_DATA()))
  {
    if (retVal && this->ContinuingExecution)
    {
      request->Set(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
    }
    else
    {
      request->Remove(vtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
      this->ContinuingExecution = false;
    }
  }
  return retVal;
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::RequestUpdateExtent(vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector, vtkInformationVector *vtkNotUsed(outputVector))
{
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  int piece = 0;
  if (this->ContinuingExecution)
  {
    assert(this->CurrentPiece >= this->StartPiece &&
      this->CurrentPiece <= this->EndPiece &&
      this->CurrentPiece < this->NumberOfPieces);
    piece = this->CurrentPiece;
  }
  else
  {
    piece = this->StartPiece;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
    this->GetNumberOfPieces());
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
    this->GhostLevel);
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SetWriteSummaryFile(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " ("
                << this << "): setting WriteSummaryFile to " << flag);
  if (this->WriteSummaryFile != flag)
  {
    this->WriteSummaryFile = flag;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WriteInternal()
{
  bool the_beginning = (this->ContinuingExecution == false);
  bool the_end = true;

  this->ContinuingExecution = false;
  this->CurrentPiece = the_beginning? this->StartPiece : this->CurrentPiece;

  assert(this->CurrentPiece >= this->StartPiece && this->CurrentPiece <= this->EndPiece);
  the_end = (this->CurrentPiece == this->EndPiece);

  if (the_beginning)
  {
    // Prepare the file name.
    this->SplitFileName();
    delete [] this->PieceWrittenFlags;
    this->PieceWrittenFlags = new unsigned char[this->NumberOfPieces];
    memset(this->PieceWrittenFlags, 0, sizeof(unsigned char)*this->NumberOfPieces);

    // Prepare the extension.
    this->SetupPieceFileNameExtension();
  }

  // Write the current piece.

  // Split progress range by piece.  Just assume all pieces are the
  // same size.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  this->SetProgressRange(progressRange, this->CurrentPiece - this->StartPiece,
    this->EndPiece - this->StartPiece + 1);
  vtkDataSet* inputDS = this->GetInputAsDataSet();
  if (inputDS && (inputDS->GetNumberOfPoints() > 0 || inputDS->GetNumberOfCells() > 0))
  {
    if (!this->WritePiece(this->CurrentPiece))
    {
      vtkErrorMacro("Ran out of disk space; deleting file(s) already written");
      this->DeleteFiles();
      return 0;
    }
    this->PieceWrittenFlags[this->CurrentPiece] = static_cast<unsigned char>(0x1);
  }

  // Write the summary file if requested.
  if (the_end && this->WriteSummaryFile)
  {
    // Decide whether to write the summary file.
    bool writeSummaryLocally = (this->Controller == NULL || this->Controller->GetLocalProcessId() == 0);

    // Let subclasses collect information, if any to write the summary file.
    this->PrepareSummaryFile();

    if (writeSummaryLocally)
    {
      if (!this->Superclass::WriteInternal())
      {
        vtkErrorMacro("Ran out of disk space; deleting file(s) already written");
        this->DeleteFiles();
        return 0;
      }
    }
  }

  if (the_end == false)
  {
    this->CurrentPiece++;
    assert(this->CurrentPiece <= this->EndPiece);
    this->ContinuingExecution = true;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::DeleteFiles()
{
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    char* fileName = this->CreatePieceFileName(i, this->PathName);
    this->DeleteAFile(fileName);
    delete [] fileName;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::PrepareSummaryFile()
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    assert(this->PieceWrittenFlags != NULL);
    // Reduce information about which pieces were written out to rank 0.
    int myId = this->Controller->GetLocalProcessId();
    unsigned char* recvBuffer = (myId == 0)? new unsigned char[this->NumberOfPieces] : NULL;
    this->Controller->Reduce(
      this->PieceWrittenFlags, recvBuffer, this->NumberOfPieces,
      vtkCommunicator::MAX_OP, 0);
    if (myId == 0)
    {
      std::swap(this->PieceWrittenFlags, recvBuffer);
    }
    delete [] recvBuffer;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePrimaryElementAttributes(ostream &, vtkIndent)
{
  this->WriteScalarAttribute("GhostLevel", this->GhostLevel);
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WriteData()
{
  // Write the summary file.
  ostream& os = *(this->Stream);
  vtkIndent indent = vtkIndent().GetNextIndent();
  vtkIndent nextIndent = indent.GetNextIndent();

  this->StartFile();
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  os << indent << "<" << this->GetDataSetName();
  this->WritePrimaryElementAttributes(os, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }
  os << ">\n";

  // Write the information needed for a reader to produce the output's
  // information during UpdateInformation without reading a piece.
  this->WritePData(indent.GetNextIndent());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return 0;
  }

  // Write the elements referencing each piece and its file.
  for (int i = 0; i < this->NumberOfPieces; ++i)
  {
    if (this->PieceWrittenFlags[i] == 0)
    {
      continue;
    }
    os << nextIndent << "<Piece";
    this->WritePPieceAttributes(i);
    if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
    {
      return 0;
    }
    os << "/>\n";
  }

  os << indent << "</" << this->GetDataSetName() << ">\n";

  this->EndFile();
  return (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError) ? 0 : 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePData(vtkIndent indent)
{
  vtkDataSet* input = this->GetInputAsDataSet();
  this->WritePPointData(input->GetPointData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WritePCellData(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePPieceAttributes(int index)
{
  char* fileName = this->CreatePieceFileName(index);
  this->WriteStringAttribute("Source", fileName);
  delete [] fileName;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SplitFileName()
{
  // Split the FileName into its PathName, FileNameBase, and
  // FileNameExtension components.

  std::string pathname = vtksys::SystemTools::GetProgramPath(this->FileName);
  // Pathname may be empty if FileName is simply a filename without any leading
  // "/".
  if (!pathname.empty())
  {
    pathname += "/";
  }
  std::string filename_wo_ext = vtksys::SystemTools::GetFilenameWithoutExtension(this->FileName);
  std::string ext = vtksys::SystemTools::GetFilenameExtension(this->FileName);

  delete [] this->PathName;
  delete [] this->FileNameBase;
  delete [] this->FileNameExtension;

  this->PathName = vtksys::SystemTools::DuplicateString(pathname.c_str());
  this->FileNameBase = vtksys::SystemTools::DuplicateString(filename_wo_ext.c_str());
  this->FileNameExtension = vtksys::SystemTools::DuplicateString(ext.c_str());
}

//----------------------------------------------------------------------------
char* vtkXMLPDataWriter::CreatePieceFileName(int index, const char* path)
{
  std::ostringstream s;
  if (path)
  {
    s << path;
  }
  s << this->FileNameBase;
  if (this->UseSubdirectory)
  {
    s << "/" << this->FileNameBase;
  }
  s << "_" << index;
  if (this->PieceFileNameExtension)
  {
    s << this->PieceFileNameExtension;
  }

  size_t len = s.str().length();
  char *buffer = new char[len + 1];
  strncpy(buffer, s.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WritePiece(int index)
{
  // Create the writer for the piece.  Its configuration should match
  // our own writer.
  vtkXMLWriter* pWriter = this->CreatePieceWriter(index);
  pWriter->AddObserver(vtkCommand::ProgressEvent, this->ProgressObserver);

  char* fileName = this->CreatePieceFileName(index, this->PathName);
  std::string path = vtksys::SystemTools::GetParentDirectory(fileName);
  if (path.size() > 0 && !vtksys::SystemTools::PathExists(path))
  {
    vtksys::SystemTools::MakeDirectory(path);
  }
  pWriter->SetFileName(fileName);
  delete [] fileName;

  // Copy the writer settings.
  pWriter->SetDebug(this->Debug);
  pWriter->SetCompressor(this->Compressor);
  pWriter->SetDataMode(this->DataMode);
  pWriter->SetByteOrder(this->ByteOrder);
  pWriter->SetEncodeAppendedData(this->EncodeAppendedData);
  pWriter->SetHeaderType(this->HeaderType);
  pWriter->SetBlockSize(this->BlockSize);

  // Write the piece.
  int result = pWriter->Write();
  this->SetErrorCode(pWriter->GetErrorCode());

  // Cleanup.
  pWriter->RemoveObserver(this->ProgressObserver);
  pWriter->Delete();

  return result;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::ProgressCallbackFunction(vtkObject* caller,
                                                 unsigned long,
                                                 void* clientdata, void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if(w)
  {
    reinterpret_cast<vtkXMLPDataWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::ProgressCallback(vtkAlgorithm* w)
{
  float width = this->ProgressRange[1]-this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress*width;
  this->UpdateProgressDiscrete(progress);
  if(this->AbortExecute)
  {
    w->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SetupPieceFileNameExtension()
{
  delete [] this->PieceFileNameExtension;

  // Create a temporary piece writer and then initialize the extension.
  vtkXMLWriter* writer = this->CreatePieceWriter(0);
  const char* ext = writer->GetDefaultFileExtension();
  this->PieceFileNameExtension = new char[strlen(ext)+2];
  this->PieceFileNameExtension[0] = '.';
  strcpy(this->PieceFileNameExtension+1, ext);
  writer->Delete();
}
