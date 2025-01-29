// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkLegacyCellGridWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellGrid.h"
#include "vtkErrorCode.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#include <algorithm>
#include <iterator>
#include <vector>

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkLegacyCellGridWriter);

void vtkLegacyCellGridWriter::WriteData()
{
  ostream* fp;
  vtkCellGrid* input = vtkCellGrid::SafeDownCast(this->GetInput());

  vtkDebugMacro(<< "Writing vtk cell-grid data...");

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
  // Write cell-grid specific stuff
  //
  // We use "DATASET" here to prevent vtkDataObjectReader from attempting
  // to read cell-grids, even though vtkCellGrid does not inherit vtkDataSet.
  std::vector<char> contents;
  {
    nlohmann::json data;
    if (!Subwriter->ToJSON(data, input))
    {
      vtkErrorMacro("Could not write \"" << this->FileName << "\".");
      this->CloseVTKFile(fp);
      unlink(this->FileName);
      return;
    }
    nlohmann::json::to_msgpack(data, contents);
  }

  *fp << "DATASET CELL_GRID " << contents.size() << "\n";
  fp->write(contents.data(), contents.size());
  *fp << "\n";

  this->CloseVTKFile(fp);
}

int vtkLegacyCellGridWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkCellGrid");
  return 1;
}

vtkCellGrid* vtkLegacyCellGridWriter::GetInput()
{
  return vtkCellGrid::SafeDownCast(this->Superclass::GetInput());
}

vtkCellGrid* vtkLegacyCellGridWriter::GetInput(int port)
{
  return vtkCellGrid::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkLegacyCellGridWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
