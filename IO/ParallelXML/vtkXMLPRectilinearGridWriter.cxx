// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPRectilinearGridWriter.h"

#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"
#include "vtkXMLRectilinearGridWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPRectilinearGridWriter);

//------------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::vtkXMLPRectilinearGridWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPRectilinearGridWriter::~vtkXMLPRectilinearGridWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkRectilinearGrid* vtkXMLPRectilinearGridWriter::GetInput()
{
  return static_cast<vtkRectilinearGrid*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDataSetName()
{
  return "PRectilinearGrid";
}

//------------------------------------------------------------------------------
const char* vtkXMLPRectilinearGridWriter::GetDefaultFileExtension()
{
  return "pvtr";
}

//------------------------------------------------------------------------------
vtkXMLStructuredDataWriter* vtkXMLPRectilinearGridWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLRectilinearGridWriter* pWriter = vtkXMLRectilinearGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//------------------------------------------------------------------------------
void vtkXMLPRectilinearGridWriter::WritePData(vtkIndent indent)
{
  this->Superclass::WritePData(indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  vtkRectilinearGrid* input = this->GetInput();
  this->WritePCoordinates(
    input->GetXCoordinates(), input->GetYCoordinates(), input->GetZCoordinates(), indent);
}

int vtkXMLPRectilinearGridWriter::FillInputPortInformation(
  int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}
VTK_ABI_NAMESPACE_END
