/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTreeWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTreeWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkTree.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"

#if !defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkTreeWriter);

void vtkTreeWriter::WriteEdges(ostream& Stream, vtkTree* Tree, vtkIdType Vertex)
{
  if (Vertex != Tree->GetRoot())
    {
    Stream << Vertex << " " << Tree->GetParent(Vertex) << "\n";
    }
  
  vtkIdType count = Tree->GetNumberOfChildren(Vertex);
  for(vtkIdType child = 0; child != count; ++child)
    {
    WriteEdges(Stream, Tree, Tree->GetChild(Vertex, child));
    }
}

void vtkTreeWriter::WriteData()
{
  ostream *fp;
  vtkTree* const input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk tree data...");

  if ( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
    {
    if (fp)
      {
      if(this->FileName)
        {
        vtkErrorMacro("Ran out of disk space; deleting file: "
                      << this->FileName);
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
  
  int error_occurred = 0;

  if(!error_occurred && !this->WriteFieldData(fp, input->GetFieldData()))
    {
    error_occurred = 1;
    }
  if (!error_occurred && !this->WritePoints(fp, input->GetPoints()))
    {
    error_occurred = 1;
    }
  if(!error_occurred)
    {
    const vtkIdType edge_count = input->GetNumberOfEdges();
    *fp << "EDGES " << edge_count << "\n";
    this->WriteEdges(*fp, input, input->GetRoot());
    }
  if (!error_occurred && !this->WriteEdgeData(fp, input))
    {
    error_occurred = 1;
    }
  if (!error_occurred && !this->WriteVertexData(fp, input))
    {
    error_occurred = 1;
    }

  if(error_occurred)
    {
    if(this->FileName)
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

int vtkTreeWriter::FillInputPortInformation(int, vtkInformation *info)
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
  this->Superclass::PrintSelf(os,indent);
}
