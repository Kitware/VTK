// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPPolyDataWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkXMLPolyDataWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPPolyDataWriter);

//------------------------------------------------------------------------------
vtkXMLPPolyDataWriter::vtkXMLPPolyDataWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPPolyDataWriter::~vtkXMLPPolyDataWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPPolyDataWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkPolyData* vtkXMLPPolyDataWriter::GetInput()
{
  return static_cast<vtkPolyData*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLPPolyDataWriter::GetDataSetName()
{
  return "PPolyData";
}

//------------------------------------------------------------------------------
const char* vtkXMLPPolyDataWriter::GetDefaultFileExtension()
{
  return "pvtp";
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter* vtkXMLPPolyDataWriter::CreateUnstructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLPolyDataWriter* pWriter = vtkXMLPolyDataWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//------------------------------------------------------------------------------
int vtkXMLPPolyDataWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}
VTK_ABI_NAMESPACE_END
