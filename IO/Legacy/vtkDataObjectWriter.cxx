// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkDataObjectWriter.h"

#include "vtkDataObject.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkDataObjectWriter);

vtkDataObjectWriter::vtkDataObjectWriter()
{
  this->Writer = vtkDataWriter::New();
}

vtkDataObjectWriter::~vtkDataObjectWriter()
{
  this->Writer->Delete();
}

// Write FieldData data to file
void vtkDataObjectWriter::WriteData()
{
  ostream* fp;
  vtkFieldData* f = this->GetInput()->GetFieldData();

  vtkDebugMacro(<< "Writing vtk FieldData data...");

  this->Writer->SetInputData(this->GetInput());

  if (!(fp = this->Writer->OpenVTKFile()) || !this->Writer->WriteHeader(fp))
  {
    return;
  }
  //
  // Write FieldData data specific stuff
  //
  this->Writer->WriteFieldData(fp, f);

  this->Writer->CloseVTKFile(fp);

  this->Writer->SetInputData(nullptr);
}

void vtkDataObjectWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent
     << "File Name: " << (this->Writer->GetFileName() ? this->Writer->GetFileName() : "(none)")
     << "\n";

  if (this->Writer->GetFileType() == VTK_BINARY)
  {
    os << indent << "File Type: BINARY\n";
  }
  else
  {
    os << indent << "File Type: ASCII\n";
  }

  if (this->Writer->GetHeader())
  {
    os << indent << "Header: " << this->Writer->GetHeader() << "\n";
  }
  else
  {
    os << indent << "Header: (None)\n";
  }

  if (this->Writer->GetFieldDataName())
  {
    os << indent << "Field Data Name: " << this->Writer->GetFieldDataName() << "\n";
  }
  else
  {
    os << indent << "Field Data Name: (None)\n";
  }
}

int vtkDataObjectWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkDataObject");
  return 1;
}
VTK_ABI_NAMESPACE_END
