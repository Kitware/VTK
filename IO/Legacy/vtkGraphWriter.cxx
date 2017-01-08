/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGraphWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGraphWriter.h"

#include "vtkByteSwap.h"
#include "vtkCellArray.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkMolecule.h"
#include "vtkObjectFactory.h"

#if!defined(_WIN32) || defined(__CYGWIN__)
# include <unistd.h> /* unlink */
#else
# include <io.h> /* unlink */
#endif

vtkStandardNewMacro(vtkGraphWriter);

void vtkGraphWriter::WriteData()
{
  ostream *fp;
  vtkGraph* const input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk graph data...");

  if( !(fp=this->OpenVTKFile()) || !this->WriteHeader(fp) )
  {
    if(fp)
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

  vtkMolecule *mol = vtkMolecule::SafeDownCast(input);
  if (mol) // molecule is most derived, test first
  {
    *fp << "DATASET MOLECULE\n";
    this->WriteMoleculeData(fp, mol);
  }
  else if (vtkDirectedGraph::SafeDownCast(input))
  {
    *fp << "DATASET DIRECTED_GRAPH\n";
  }
  else
  {
    *fp << "DATASET UNDIRECTED_GRAPH\n";
  }

  int error_occurred = 0;

  if(!this->WriteFieldData(fp, input->GetFieldData()))
  {
    error_occurred = 1;
  }
  if(!error_occurred && !this->WritePoints(fp, input->GetPoints()))
  {
    error_occurred = 1;
  }
  if(!error_occurred)
  {
    const vtkIdType vertex_count = input->GetNumberOfVertices();
    *fp << "VERTICES " << vertex_count << "\n";
    const vtkIdType edge_count = input->GetNumberOfEdges();
    *fp << "EDGES " << edge_count << "\n";
    for (vtkIdType e = 0; e < edge_count; ++e)
    {
      *fp << input->GetSourceVertex(e) << " " << input->GetTargetVertex(e) << "\n";
    }
  }
  if(!error_occurred && !this->WriteEdgeData(fp, input))
  {
    error_occurred = 1;
  }
  if(!error_occurred && !this->WriteVertexData(fp, input))
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

void vtkGraphWriter::WriteMoleculeData(std::ostream *fp, vtkMolecule *m)
{
  if (m->HasLattice())
  {
    vtkVector3d a;
    vtkVector3d b;
    vtkVector3d c;
    vtkVector3d origin;
    m->GetLattice(a, b, c, origin);
    *fp << "LATTICE_A " << a[0] << " " << a[1] << " " << a[2] << "\n";
    *fp << "LATTICE_B " << b[0] << " " << b[1] << " " << b[2] << "\n";
    *fp << "LATTICE_C " << c[0] << " " << c[1] << " " << c[2] << "\n";
    *fp << "LATTICE_ORIGIN " << origin[0] << " " << origin[1] << " "
        << origin[2] << "\n";
  }
}

int vtkGraphWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

vtkGraph* vtkGraphWriter::GetInput()
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput());
}

vtkGraph* vtkGraphWriter::GetInput(int port)
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkGraphWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
