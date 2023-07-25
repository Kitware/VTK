// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkRectilinearGridWriter.h"

#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkRectilinearGrid.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkRectilinearGridWriter);

void vtkRectilinearGridWriter::WriteData()
{
  ostream* fp;
  vtkRectilinearGrid* input = vtkRectilinearGrid::SafeDownCast(this->GetInput());
  int dim[3];

  vtkDebugMacro(<< "Writing vtk rectilinear grid...");

  if (!(fp = this->OpenVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    return;
  }
  //
  // Write rectilinear grid specific stuff
  //
  *fp << "DATASET RECTILINEAR_GRID\n";

  // Write data owned by the dataset
  if (!this->WriteDataSetData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (this->WriteExtent)
  {
    int extent[6];
    input->GetExtent(extent);
    *fp << "EXTENT " << extent[0] << " " << extent[1] << " " << extent[2] << " " << extent[3] << " "
        << extent[4] << " " << extent[5] << "\n";
  }
  else
  {
    input->GetDimensions(dim);
    *fp << "DIMENSIONS " << dim[0] << " " << dim[1] << " " << dim[2] << "\n";
  }

  if (!this->WriteCoordinates(fp, input->GetXCoordinates(), 0))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WriteCoordinates(fp, input->GetYCoordinates(), 1))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WriteCoordinates(fp, input->GetZCoordinates(), 2))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  if (!this->WriteCellData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }
  if (!this->WritePointData(fp, input))
  {
    vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
    this->CloseVTKFile(fp);
    unlink(this->FileName);
    return;
  }

  this->CloseVTKFile(fp);
}

int vtkRectilinearGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

vtkRectilinearGrid* vtkRectilinearGridWriter::GetInput()
{
  return vtkRectilinearGrid::SafeDownCast(this->Superclass::GetInput());
}

vtkRectilinearGrid* vtkRectilinearGridWriter::GetInput(int port)
{
  return vtkRectilinearGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkRectilinearGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
