/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDIMACSGraphWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkDIMACSGraphWriter.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDirectedGraph.h"
#include "vtkEdgeListIterator.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkObjectFactory.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

vtkStandardNewMacro(vtkDIMACSGraphWriter);

void vtkDIMACSGraphWriter::WriteData()
{
  vtkGraph* const input = this->GetInput();

  vtkDebugMacro(<<"Writing vtk graph data...");

  ostream *fp = this->OpenVTKFile();
  if(!fp)
  {
    vtkErrorMacro("Falied to open output stream");
    return;
  }

  *fp << "c vtkGraph as DIMACS format\n";

  if(vtkDirectedGraph::SafeDownCast(input))
  {
    *fp << "c Graph stored as DIRECTED\n";
  }
  else
  {
    *fp << "c Graph stored as UNDIRECTED\n";
  }

  const vtkIdType vertex_count = input->GetNumberOfVertices();
  const vtkIdType edge_count = input->GetNumberOfEdges();

  // Output this 'special' line with the 'problem type' and then
  // vertex and edge counts
  *fp << "p graph "<< vertex_count << " " << edge_count << "\n";

  // See if the input has a "weight" array
  vtkDataArray* weight = 0;
  weight = input->GetEdgeData()->GetArray("weight");

  // Output either the weight array or just 1 if
  // we have no weight array
  VTK_CREATE(vtkEdgeListIterator, edges);
  input->GetEdges(edges);
  if (weight)
  {
    while(edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      float value = weight->GetTuple1(e.Id);
      *fp << "e " << e.Source+1 << " " << e.Target+1 << " " << value << "\n";
    }
  }
  else
  {
    while(edges->HasNext())
    {
      vtkEdgeType e = edges->Next();
      *fp << "e " << e.Source+1 << " " << e.Target+1 << " 1\n";
    }
  }

  // NOTE: Vertices are incremented by 1 since DIMACS files number vertices
  //       from 1..n.

  this->CloseVTKFile(fp);
}

int vtkDIMACSGraphWriter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkGraph");
  return 1;
}

vtkGraph* vtkDIMACSGraphWriter::GetInput()
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput());
}

vtkGraph* vtkDIMACSGraphWriter::GetInput(int port)
{
  return vtkGraph::SafeDownCast(this->Superclass::GetInput(port));
}

void vtkDIMACSGraphWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
