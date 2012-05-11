/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRemoveIsolatedVertices.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include <vtkSmartPointer.h>
 #include <vtkMutableUndirectedGraph.h>
#include <vtkRemoveIsolatedVertices.h>

int TestRemoveIsolatedVertices(int, char *[])
{
  vtkSmartPointer<vtkMutableUndirectedGraph> g =
      vtkSmartPointer<vtkMutableUndirectedGraph>::New();

  // Create 3 vertices
  vtkIdType v1 = g->AddVertex();
  vtkIdType v2 = g->AddVertex();
  g->AddVertex();

  g->AddEdge ( v1, v2 );

  vtkSmartPointer<vtkRemoveIsolatedVertices> filter =
    vtkSmartPointer<vtkRemoveIsolatedVertices>::New();
  filter->SetInputData(g);
  filter->Update();

  if(filter->GetOutput()->GetNumberOfVertices() != 2)
    {
    std::cerr << "There are " << filter->GetOutput()->GetNumberOfVertices()
              << " vertices but there should be 2." << std::endl;
    return EXIT_FAILURE;
    }

  return EXIT_SUCCESS;
}
