/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPTableWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPTableWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkDataSetAttributes.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkTable.h"
#include "vtkXMLTableWriter.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>

vtkStandardNewMacro(vtkXMLPTableWriter);

//----------------------------------------------------------------------------
vtkXMLPTableWriter::vtkXMLPTableWriter() = default;

//----------------------------------------------------------------------------
vtkXMLPTableWriter::~vtkXMLPTableWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPTableWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkTable* vtkXMLPTableWriter::GetInput()
{
  return vtkTable::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPTableWriter::GetDataSetName()
{
  return "PTable";
}

//----------------------------------------------------------------------------
const char* vtkXMLPTableWriter::GetDefaultFileExtension()
{
  return "pvtt";
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPTableWriter::CreatePieceWriter(int index)
{
  // Create the writer for the piece.
  vtkXMLTableWriter* pWriter = vtkXMLTableWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  pWriter->SetNumberOfPieces(this->NumberOfPieces);
  pWriter->SetWritePiece(index);

  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPTableWriter::WritePData(vtkIndent indent)
{
  vtkTable* input = this->GetInput();
  this->WritePRowData(input->GetRowData(), indent);
}

//----------------------------------------------------------------------------
int vtkXMLPTableWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();
  vtkTable* inputTable = this->GetInput();
  if (inputTable && inputTable->GetNumberOfRows() > 0)
  {
    if (!this->WritePiece(piece))
    {
      vtkErrorMacro("Could not write the current piece.");
      this->DeleteFiles();
      return 0;
    }
    this->PieceWrittenFlags[piece] = static_cast<unsigned char>(0x1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkXMLPTableWriter::WritePiece(int index)
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
void vtkXMLPTableWriter::WritePRowData(vtkDataSetAttributes* ds, vtkIndent indent)
{
  if (ds->GetNumberOfArrays() == 0)
  {
    return;
  }
  ostream& os = *this->Stream;
  char** names = this->CreateStringArray(ds->GetNumberOfArrays());

  os << indent << "<PRowData";
  this->WriteAttributeIndices(ds, names);
  if (this->ErrorCode != vtkErrorCode::NoError)
  {
    this->DestroyStringArray(ds->GetNumberOfArrays(), names);
    return;
  }
  os << ">\n";

  for (int i = 0; i < ds->GetNumberOfArrays(); ++i)
  {
    this->WritePArray(ds->GetAbstractArray(i), indent.GetNextIndent(), names[i]);
    if (this->ErrorCode != vtkErrorCode::NoError)
    {
      this->DestroyStringArray(ds->GetNumberOfArrays(), names);
      return;
    }
  }

  os << indent << "</PRowData>\n";
  os.flush();
  if (os.fail())
  {
    this->SetErrorCode(vtkErrorCode::GetLastSystemError());
  }

  this->DestroyStringArray(ds->GetNumberOfArrays(), names);
}

//----------------------------------------------------------------------------
void vtkXMLPTableWriter::SetupPieceFileNameExtension()
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

//----------------------------------------------------------------------------
int vtkXMLPTableWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTable");
  return 1;
}
