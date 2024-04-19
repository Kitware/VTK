// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPImageDataWriter.h"

#include "vtkErrorCode.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkXMLImageDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPImageDataWriter);

//------------------------------------------------------------------------------
vtkXMLPImageDataWriter::vtkXMLPImageDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPImageDataWriter::~vtkXMLPImageDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPImageDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkImageData* vtkXMLPImageDataWriter::GetInput()
{
  return static_cast<vtkImageData*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLPImageDataWriter::GetDataSetName()
{
  return "PImageData";
}

//------------------------------------------------------------------------------
const char* vtkXMLPImageDataWriter::GetDefaultFileExtension()
{
  return "pvti";
}

//------------------------------------------------------------------------------
void vtkXMLPImageDataWriter::WritePrimaryElementAttributes(ostream& os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  vtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  if (this->ErrorCode == vtkErrorCode::OutOfDiskSpaceError)
  {
    return;
  }

  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
}

//------------------------------------------------------------------------------
vtkXMLStructuredDataWriter* vtkXMLPImageDataWriter::CreateStructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLImageDataWriter* pWriter = vtkXMLImageDataWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//------------------------------------------------------------------------------
int vtkXMLPImageDataWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
VTK_ABI_NAMESPACE_END
