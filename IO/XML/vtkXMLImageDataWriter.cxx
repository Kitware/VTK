// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLImageDataWriter.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkMatrix3x3.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLImageDataWriter);

//------------------------------------------------------------------------------
vtkXMLImageDataWriter::vtkXMLImageDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLImageDataWriter::~vtkXMLImageDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLImageDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkImageData* vtkXMLImageDataWriter::GetInput()
{
  return static_cast<vtkImageData*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
void vtkXMLImageDataWriter::GetInputExtent(int* extent)
{
  this->GetInput()->GetExtent(extent);
}

//------------------------------------------------------------------------------
const char* vtkXMLImageDataWriter::GetDataSetName()
{
  return "ImageData";
}

//------------------------------------------------------------------------------
const char* vtkXMLImageDataWriter::GetDefaultFileExtension()
{
  return "vti";
}

//------------------------------------------------------------------------------
void vtkXMLImageDataWriter::WritePrimaryElementAttributes(ostream& os, vtkIndent indent)
{
  this->Superclass::WritePrimaryElementAttributes(os, indent);
  vtkImageData* input = this->GetInput();
  this->WriteVectorAttribute("Origin", 3, input->GetOrigin());
  this->WriteVectorAttribute("Spacing", 3, input->GetSpacing());
  this->WriteVectorAttribute("Direction", 9, input->GetDirectionMatrix()->GetData());
}

//------------------------------------------------------------------------------
int vtkXMLImageDataWriter::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}
VTK_ABI_NAMESPACE_END
