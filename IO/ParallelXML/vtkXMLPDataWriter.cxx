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
#include "vtkDoubleArray.h"
#include "vtkErrorCode.h"
#include "vtkFieldData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vtksys/SystemTools.hxx>

//----------------------------------------------------------------------------
vtkXMLPDataWriter::vtkXMLPDataWriter() = default;

//----------------------------------------------------------------------------
vtkXMLPDataWriter::~vtkXMLPDataWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePData(vtkIndent indent)
{
  vtkDataSet* input = this->GetInputAsDataSet();

  // We want to avoid using appended data mode as it
  // is not supported in meta formats.
  int dataMode = this->DataMode;
  if (dataMode == vtkXMLWriter::Appended)
  {
    this->DataMode = vtkXMLWriter::Binary;
  }

  vtkFieldData* fieldData = input->GetFieldData();

  vtkInformation* meta = input->GetInformation();
  bool hasTime = meta->Has(vtkDataObject::DATA_TIME_STEP()) ? true : false;
  if ((fieldData && fieldData->GetNumberOfArrays()) || hasTime)
  {
    vtkNew<vtkFieldData> fieldDataCopy;
    fieldDataCopy->ShallowCopy(fieldData);
    if (hasTime)
    {
      vtkNew<vtkDoubleArray> time;
      time->SetNumberOfTuples(1);
      time->SetTypedComponent(0, 0, meta->Get(vtkDataObject::DATA_TIME_STEP()));
      time->SetName("TimeValue");
      fieldDataCopy->AddArray(time);
    }
    this->WriteFieldDataInline(fieldDataCopy, indent);
  }

  this->DataMode = dataMode;

  this->WritePPointData(input->GetPointData(), indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }
  this->WritePCellData(input->GetCellData(), indent);
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();

  vtkDataSet* inputDS = this->GetInputAsDataSet();
  if (inputDS && (inputDS->GetNumberOfPoints() > 0 || inputDS->GetNumberOfCells() > 0))
  {
    if (!this->WritePiece(piece))
    {
      vtkErrorMacro("Ran out of disk space; deleting file(s) already written");
      this->DeleteFiles();
      return 0;
    }
    this->PieceWrittenFlags[piece] = static_cast<unsigned char>(0x1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPDataWriter::WritePiece(int index)
{
  // Create the writer for the piece.  Its configuration should match
  // our own writer.
  vtkXMLWriter* pWriter = this->CreatePieceWriter(index);
  pWriter->AddObserver(vtkCommand::ProgressEvent, this->InternalProgressObserver);

  char* fileName = this->CreatePieceFileName(index, this->PathName);
  std::string path = vtksys::SystemTools::GetParentDirectory(fileName);
  if (!path.empty() && !vtksys::SystemTools::PathExists(path))
  {
    vtksys::SystemTools::MakeDirectory(path);
  }
  pWriter->SetFileName(fileName);
  delete[] fileName;

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
  pWriter->RemoveObserver(this->InternalProgressObserver);
  pWriter->Delete();

  return result;
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::WritePrimaryElementAttributes(
  std::ostream& vtkNotUsed(os), vtkIndent vtkNotUsed(indent))
{
  this->WriteScalarAttribute("GhostLevel", this->GhostLevel);
}

//----------------------------------------------------------------------------
void vtkXMLPDataWriter::SetupPieceFileNameExtension()
{
  this->Superclass::SetupPieceFileNameExtension();

  // Create a temporary piece writer and then initialize the extension.
  vtkXMLWriter* writer = this->CreatePieceWriter(0);
  const char* ext = writer->GetDefaultFileExtension();
  this->PieceFileNameExtension = new char[strlen(ext) + 2];
  this->PieceFileNameExtension[0] = '.';
  strcpy(this->PieceFileNameExtension + 1, ext);
  writer->Delete();
}
