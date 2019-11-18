/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPHyperTreeGridWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXMLPHyperTreeGridWriter.h"

#include "vtkCallbackCommand.h"
#include "vtkErrorCode.h"
#include "vtkHyperTreeGrid.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkXMLHyperTreeGridWriter.h"

#include <vtksys/SystemTools.hxx>

#include <cassert>

vtkStandardNewMacro(vtkXMLPHyperTreeGridWriter);

//----------------------------------------------------------------------------
vtkXMLPHyperTreeGridWriter::vtkXMLPHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
vtkXMLPHyperTreeGridWriter::~vtkXMLPHyperTreeGridWriter() = default;

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
vtkHyperTreeGrid* vtkXMLPHyperTreeGridWriter::GetInput()
{
  return vtkHyperTreeGrid::SafeDownCast(this->Superclass::GetInput());
}

//----------------------------------------------------------------------------
const char* vtkXMLPHyperTreeGridWriter::GetDataSetName()
{
  return "PHyperTreeGrid";
}

//----------------------------------------------------------------------------
const char* vtkXMLPHyperTreeGridWriter::GetDefaultFileExtension()
{
  return "phtg";
}

//----------------------------------------------------------------------------
vtkXMLHyperTreeGridWriter* vtkXMLPHyperTreeGridWriter::CreateHyperTreeGridPieceWriter(
  int vtkNotUsed(index))
{
  // Create the writer for the piece.
  vtkXMLHyperTreeGridWriter* pWriter = vtkXMLHyperTreeGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//----------------------------------------------------------------------------
vtkXMLWriter* vtkXMLPHyperTreeGridWriter::CreatePieceWriter(int index)
{
  // Create the writer for the piece.
  vtkXMLHyperTreeGridWriter* pWriter = this->CreateHyperTreeGridPieceWriter(index);
  return pWriter;
}

//----------------------------------------------------------------------------
void vtkXMLPHyperTreeGridWriter::WritePData(vtkIndent vtkNotUsed(indent)) {}

//----------------------------------------------------------------------------
int vtkXMLPHyperTreeGridWriter::WritePieceInternal()
{
  int piece = this->GetCurrentPiece();
  vtkHyperTreeGrid* inputHyperTreeGrid = this->GetInput();

  if (inputHyperTreeGrid)
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
int vtkXMLPHyperTreeGridWriter::WritePiece(int index)
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
void vtkXMLPHyperTreeGridWriter::SetupPieceFileNameExtension()
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
int vtkXMLPHyperTreeGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkHyperTreeGrid");
  return 1;
}
