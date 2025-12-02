// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
#include "vtkAdjacentVertexIterator.h"
#include "vtkBitArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkRandomGraphSource.h"
#include "vtkSmartPointer.h"

#include <iostream>

#define VTK_CREATE(type, name) vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestRandomGraphSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkRandomGraphSource, source);

  int errors = 0;

  std::cerr << "Testing simple generator..." << std::endl;
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(200);
  source->Update();
  vtkGraph* g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    std::cerr << "ERROR: Wrong number of vertices (" << g->GetNumberOfVertices() << " != " << 100
              << ")" << std::endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 200)
  {
    std::cerr << "ERROR: Wrong number of edges (" << g->GetNumberOfEdges() << " != " << 200 << ")"
              << std::endl;
    errors++;
  }
  std::cerr << "...done." << std::endl;

  std::cerr << "Testing simple generator..." << std::endl;
  source->SetStartWithTree(true);
  source->Update();
  g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    std::cerr << "ERROR: Wrong number of vertices (" << g->GetNumberOfVertices() << " != " << 100
              << ")" << std::endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 299)
  {
    std::cerr << "ERROR: Wrong number of edges (" << g->GetNumberOfEdges() << " != " << 299 << ")"
              << std::endl;
    errors++;
  }
  VTK_CREATE(vtkBitArray, visited);
  visited->SetNumberOfTuples(g->GetNumberOfVertices());
  for (vtkIdType i = 0; i < g->GetNumberOfVertices(); i++)
  {
    visited->SetValue(i, 0);
  }
  VTK_CREATE(vtkIdTypeArray, stack);
  stack->SetNumberOfTuples(g->GetNumberOfVertices());
  vtkIdType top = 0;
  stack->SetValue(top, 0);
  VTK_CREATE(vtkAdjacentVertexIterator, adj);
  while (top >= 0)
  {
    vtkIdType u = stack->GetValue(top);
    top--;
    g->GetAdjacentVertices(u, adj);
    while (adj->HasNext())
    {
      vtkIdType v = adj->Next();
      if (!visited->GetValue(v))
      {
        visited->SetValue(v, 1);
        top++;
        stack->SetValue(top, v);
      }
    }
  }
  vtkIdType numVisited = 0;
  for (vtkIdType i = 0; i < g->GetNumberOfVertices(); i++)
  {
    numVisited += visited->GetValue(i);
  }
  if (numVisited != g->GetNumberOfVertices())
  {
    std::cerr << "ERROR: Starting with tree was not connected."
              << "Only " << numVisited << " of " << g->GetNumberOfVertices() << " were connected."
              << std::endl;
    errors++;
  }
  std::cerr << "...done." << std::endl;

  return errors;
}
