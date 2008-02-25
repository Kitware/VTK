/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAssignAttribute.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkAssignAttribute.

#include "vtkAssignAttribute.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkDoubleArray.h"
#include "vtkMutableUndirectedGraph.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestAssignAttribute(int, char *[])
{
  int errors = 0;
  
  VTK_CREATE(vtkMutableUndirectedGraph, graph);
  VTK_CREATE(vtkPolyData, poly);
  VTK_CREATE(vtkPoints, pts);
  VTK_CREATE(vtkCellArray, verts);
  VTK_CREATE(vtkDoubleArray, scalars);
  scalars->SetName("scalars");
  for (vtkIdType i = 0; i < 10; ++i)
    {
    pts->InsertNextPoint(i, 0, 0);
    verts->InsertNextCell(1, &i);
    graph->AddVertex();
    scalars->InsertNextValue(i);
    }
  for (vtkIdType i = 0; i < 10; ++i)
    {
    graph->AddEdge(i, (i+1)%10);
    }
  graph->GetVertexData()->AddArray(scalars);
  graph->GetEdgeData()->AddArray(scalars);
  poly->SetPoints(pts);
  poly->SetVerts(verts);
  poly->GetPointData()->AddArray(scalars);
  poly->GetCellData()->AddArray(scalars);
  VTK_CREATE(vtkAssignAttribute, assign);

  assign->SetInput(graph);
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::VERTEX_DATA);  
  assign->Update();  
  vtkGraph *output = vtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetVertexData()->GetScalars() != scalars.GetPointer())
    {
    cerr << "Vertex scalars not set properly" << endl;
    ++errors;
    }
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::EDGE_DATA);
  assign->Update();  
  output = vtkGraph::SafeDownCast(assign->GetOutput());
  if (output->GetEdgeData()->GetScalars() != scalars.GetPointer())
    {
    cerr << "Edge scalars not set properly" << endl;
    ++errors;
    }

  assign->SetInput(poly);
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::POINT_DATA);  
  assign->Update();  
  vtkPolyData *outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetPointData()->GetScalars() != scalars.GetPointer())
    {
    cerr << "Vertex scalars not set properly" << endl;
    ++errors;
    }
  assign->Assign("scalars", vtkDataSetAttributes::SCALARS, vtkAssignAttribute::CELL_DATA);
  assign->Update();  
  outputPoly = vtkPolyData::SafeDownCast(assign->GetOutput());
  if (outputPoly->GetCellData()->GetScalars() != scalars.GetPointer())
    {
    cerr << "Edge scalars not set properly" << endl;
    ++errors;
    }

  return 0;
}
