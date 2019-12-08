/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataObjectWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPDataObjectWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMultiProcessController.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>

vtkCxxSetObjectMacro(vtkXMLPDataObjectWriter, Controller, vtkMultiProcessController);
//----------------------------------------------------------------------------
vtkXMLPDataObjectWriter::vtkXMLPDataObjectWriter()
{
  this->StartPiece = 0;
  this->EndPiece = 0;
  this->NumberOfPieces = 1;
  this->GhostLevel = 0;
  this->WriteSummaryFile = 1;

  this->UseSubdirectory = false;

  this->PathName = nullptr;
  this->FileNameBase = nullptr;
  this->FileNameExtension = nullptr;
  this->PieceFileNameExtension = nullptr;

  // Setup a callback for the internal writer to report progress.
  this->InternalProgressObserver = vtkCallbackCommand::New();
  this->InternalProgressObserver->SetCallback(&vtkXMLPDataObjectWriter::ProgressCallbackFunction);
  this->InternalProgressObserver->SetClientData(this);

  this->Controller = nullptr;
  this->SetController(vtkMultiProcessController::GetGlobalController());

  this->ContinuingExecution = false;
  this->CurrentPiece = -1;
  this->PieceWrittenFlags = nullptr;
}

//----------------------------------------------------------------------------
vtkXMLPDataObjectWriter::~vtkXMLPDataObjectWriter()
{
  delete[] this->PathName;
  delete[] this->FileNameBase;
  delete[] this->FileNameExtension;
  delete[] this->PieceFileNameExtension;
  delete[] this->PieceWrittenFlags;
  this->SetController(nullptr);
  this->InternalProgressObserver->Delete();
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "NumberOfPieces: " << this->NumberOfPieces << "\n";
  os << indent << "StartPiece: " << this->StartPiece << "\n";
  os << indent << "EndPiece: " << this->EndPiece << "\n";
  os << indent << "GhostLevel: " << this->GhostLevel << "\n";
  os << indent << "UseSubdirectory: " << this->UseSubdirectory << "\n";
  os << indent << "WriteSummaryFile: " << this->WriteSummaryFile << "\n";
}

//----------------------------------------------------------------------------
vtkTypeBool vtkXMLPDataObjectWriter::ProcessRequest(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  if (request->Has(vtkStreamingDemandDrivenPipeline::REQUEST_UPDATE_EXTENT()))
  {
    return this->RequestUpdateExtent(request, inputVector, outputVector);
  }

  vtkTypeBool retVal = this->Superclass::ProcessRequest(request, inputVector, outputVector);
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
void vtkXMLPDataObjectWriter::SetWriteSummaryFile(int flag)
{
  vtkDebugMacro(<< this->GetClassName() << " (" << this << "): setting WriteSummaryFile to "
                << flag);
  if (this->WriteSummaryFile != flag)
  {
    this->WriteSummaryFile = flag;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
int vtkXMLPDataObjectWriter::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* vtkNotUsed(outputVector))
{
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  int piece = 0;
  if (this->ContinuingExecution)
  {
    assert(this->CurrentPiece >= this->StartPiece && this->CurrentPiece <= this->EndPiece &&
      this->CurrentPiece < this->NumberOfPieces);
    piece = this->CurrentPiece;
  }
  else
  {
    piece = this->StartPiece;
  }

  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(
    vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), this->GetNumberOfPieces());
  inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), this->GhostLevel);
  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataObjectWriter::WriteInternal()
{
  bool beginning = this->ContinuingExecution == false;
  bool end = true;

  this->ContinuingExecution = false;
  this->CurrentPiece = beginning ? this->StartPiece : this->CurrentPiece;

  assert(this->CurrentPiece >= this->StartPiece && this->CurrentPiece <= this->EndPiece);
  end = this->CurrentPiece == this->EndPiece;

  if (beginning)
  {
    // Prepare the file name.
    this->SplitFileName();
    delete[] this->PieceWrittenFlags;
    this->PieceWrittenFlags = new unsigned char[this->NumberOfPieces];
    memset(this->PieceWrittenFlags, 0, sizeof(unsigned char) * this->NumberOfPieces);

    // Prepare the extension.
    this->SetupPieceFileNameExtension();
  }

  // Write the current piece.

  // Split progress range by piece. Just assume all pieces are the
  // same size.
  float progressRange[2] = { 0.f, 0.f };
  this->GetProgressRange(progressRange);

  this->SetProgressRange(
    progressRange, this->CurrentPiece - this->StartPiece, this->EndPiece - this->StartPiece + 1);

  if (!this->WritePieceInternal())
  {
    return 0;
  }

  // Write the summary file if requested.
  if (end && this->WriteSummaryFile)
  {
    // Decide whether to write the summary file.
    bool writeSummaryLocally =
      (this->Controller == nullptr || this->Controller->GetLocalProcessId() == 0);

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

  if (end == false)
  {
    this->CurrentPiece++;
    assert(this->CurrentPiece <= this->EndPiece);
    this->ContinuingExecution = true;
  }
  return 1;
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::PrepareSummaryFile()
{
  if (this->Controller && this->Controller->GetNumberOfProcesses() > 1)
  {
    assert(this->PieceWrittenFlags != nullptr);
    // Reduce information about which pieces were written out to rank 0.
    int myId = this->Controller->GetLocalProcessId();
    unsigned char* recvBuffer = (myId == 0) ? new unsigned char[this->NumberOfPieces] : nullptr;
    this->Controller->Reduce(
      this->PieceWrittenFlags, recvBuffer, this->NumberOfPieces, vtkCommunicator::MAX_OP, 0);
    if (myId == 0)
    {
      std::swap(this->PieceWrittenFlags, recvBuffer);
    }
    delete[] recvBuffer;
  }
}

//----------------------------------------------------------------------------
int vtkXMLPDataObjectWriter::WriteData()
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
void vtkXMLPDataObjectWriter::WritePPieceAttributes(int index)
{
  char* fileName = this->CreatePieceFileName(index);
  this->WriteStringAttribute("Source", fileName);
  delete[] fileName;
}

//----------------------------------------------------------------------------
char* vtkXMLPDataObjectWriter::CreatePieceFileName(int index, const char* path)
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
  char* buffer = new char[len + 1];
  strncpy(buffer, s.str().c_str(), len);
  buffer[len] = '\0';

  return buffer;
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::SplitFileName()
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

  delete[] this->PathName;
  delete[] this->FileNameBase;
  delete[] this->FileNameExtension;

  this->PathName = vtksys::SystemTools::DuplicateString(pathname.c_str());
  this->FileNameBase = vtksys::SystemTools::DuplicateString(filename_wo_ext.c_str());
  this->FileNameExtension = vtksys::SystemTools::DuplicateString(ext.c_str());
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::ProgressCallbackFunction(
  vtkObject* caller, unsigned long, void* clientdata, void*)
{
  vtkAlgorithm* w = vtkAlgorithm::SafeDownCast(caller);
  if (w)
  {
    reinterpret_cast<vtkXMLPDataObjectWriter*>(clientdata)->ProgressCallback(w);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::ProgressCallback(vtkAlgorithm* w)
{
  float width = this->ProgressRange[1] - this->ProgressRange[0];
  float internalProgress = w->GetProgress();
  float progress = this->ProgressRange[0] + internalProgress * width;
  this->UpdateProgressDiscrete(progress);
  if (this->AbortExecute)
  {
    w->SetAbortExecute(1);
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::DeleteFiles()
{
  for (int i = this->StartPiece; i < this->EndPiece; ++i)
  {
    char* fileName = this->CreatePieceFileName(i, this->PathName);
    this->DeleteAFile(fileName);
    delete[] fileName;
  }
}

//----------------------------------------------------------------------------
void vtkXMLPDataObjectWriter::SetupPieceFileNameExtension()
{
  delete[] this->PieceFileNameExtension;
}
