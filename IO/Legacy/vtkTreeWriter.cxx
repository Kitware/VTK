// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkTreeWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkTree.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
#include <unistd.h> /* unlink */
#else
#include <io.h> /* unlink */
#endif

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkTreeWriter);

void vtkTreeWriter::WriteEdges(ostream& Stream, vtkTree* Tree)
{
  for (vtkIdType e = 0; e < Tree->GetNumberOfEdges(); ++e)
  {
    vtkIdType parent = Tree->GetSourceVertex(e);
    vtkIdType child = Tree->GetTargetVertex(e);
    Stream << child << " " << parent << "\n";
  }
}

void vtkTreeWriter::WriteData()
{
  ostream* fp;
  vtkTree* const input = this->GetInput();

  vtkDebugMacro(<< "Writing vtk tree data...");

  if (!(fp = this->OpenVTKFile()) || !this->WriteHeader(fp))
  {
    if (fp)
    {
      if (this->FileName)
      {
        vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
        this->CloseVTKFile(fp);
        unlink(this->FileName);
      }
      else
      {
        this->CloseVTKFile(fp);
        vtkErrorMacro("Could not read memory header. ");
      }
    }
    return;
  }

  *fp << "DATASET TREE\n";

  bool error_occurred = false;

  if (!this->WriteFieldData(fp, input->GetFieldData()))
  {
    error_occurred = true;
  }
  if (!error_occurred && !this->WritePoints(fp, input->GetPoints()))
  {
    error_occurred = true;
  }
  if (!error_occurred)
  {
    const vtkIdType edge_count = input->GetNumberOfEdges();
    *fp << "EDGES " << edge_count << "\n";
    this->WriteEdges(*fp, input);
  }
  if (!error_occurred && !this->WriteEdgeData(fp, input))
  {
    error_occurred = true;
  }
  if (!error_occurred && !this->WriteVertexData(fp, input))
  {
    error_occurred = true;
  }

  if (error_occurred)
  {
    if (this->FileName)
    {
      vtkErrorMacro("Ran out of disk space; deleting file: " << this->FileName);
      this->CloseVTKFile(fp);
      unlink(this->FileName);
    }
    else
    {
      vtkErrorMacro("Error writing data set to memory");
      this->CloseVTKFile(fp);
    }
    return;
  }
  this->CloseVTKFile(fp);
}

int vtkTreeWriter::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkTree");
  return 1;
}

vtkTree* vtkTreeWriter::GetInput()
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput());
}

vtkTree* vtkTreeWriter::GetInput(int port)
{
  return vtkTree::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkTreeWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
VTK_ABI_NAMESPACE_END
