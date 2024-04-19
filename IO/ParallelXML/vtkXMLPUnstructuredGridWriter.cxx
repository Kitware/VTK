// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkXMLPUnstructuredGridWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridWriter.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkXMLPUnstructuredGridWriter);

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::vtkXMLPUnstructuredGridWriter() = default;

//------------------------------------------------------------------------------
vtkXMLPUnstructuredGridWriter::~vtkXMLPUnstructuredGridWriter() = default;

//------------------------------------------------------------------------------
void vtkXMLPUnstructuredGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkUnstructuredGridBase* vtkXMLPUnstructuredGridWriter::GetInput()
{
  return static_cast<vtkUnstructuredGridBase*>(this->Superclass::GetInput());
}

//------------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDataSetName()
{
  return "PUnstructuredGrid";
}

//------------------------------------------------------------------------------
const char* vtkXMLPUnstructuredGridWriter::GetDefaultFileExtension()
{
  return "pvtu";
}

//------------------------------------------------------------------------------
vtkXMLUnstructuredDataWriter* vtkXMLPUnstructuredGridWriter::CreateUnstructuredPieceWriter()
{
  // Create the writer for the piece.
  vtkXMLUnstructuredGridWriter* pWriter = vtkXMLUnstructuredGridWriter::New();
  pWriter->SetInputConnection(this->GetInputConnection(0, 0));
  return pWriter;
}

//------------------------------------------------------------------------------
int vtkXMLPUnstructuredGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGridBase");
  return 1;
}
VTK_ABI_NAMESPACE_END
