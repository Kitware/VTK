// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include <vtkMutableUndirectedGraph.h>
#include <vtkRemoveIsolatedVertices.h>
#include <vtkSmartPointer.h>

int TestRemoveIsolatedVertices(int, char*[])
{
  vtkSmartPointer<vtkMutableUndirectedGraph> g = vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  // Create 3 vertices
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();
  g->AddVertex();

  g->AddEdge(v1, v2);

  vtkSmartPointer<vtkRemoveIsolatedVertices> filter =
    vtkSmartPointer<vtkRemoveIsolatedVertices>::New();
  filter->SetInputData(g);
  filter->Update();

  if (filter->GetOutput()->GetNumberOfVertices() != 2)
  {
    std::cerr << "There are " << filter->GetOutput()->GetNumberOfVertices()
              << " vertices but there should be 2." << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
