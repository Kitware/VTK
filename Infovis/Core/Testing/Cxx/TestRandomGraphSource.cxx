/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRandomGraphSource.cxx

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
#include "vtkAdjacentVertexIterator.h"
#include "vtkBitArray.h"
#include "vtkGraph.h"
#include "vtkIdTypeArray.h"
#include "vtkRandomGraphSource.h"
#include "vtkSmartPointer.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestRandomGraphSource(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  VTK_CREATE(vtkRandomGraphSource, source);

  int errors = 0;

  cerr << "Testing simple generator..." << endl;
  source->SetNumberOfVertices(100);
  source->SetNumberOfEdges(200);
  source->Update();
  vtkGraph* g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    cerr << "ERROR: Wrong number of vertices ("
         << g->GetNumberOfVertices() << " != " << 100 << ")" << endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 200)
  {
    cerr << "ERROR: Wrong number of edges ("
         << g->GetNumberOfEdges() << " != " << 200 << ")" << endl;
    errors++;
  }
  cerr << "...done." << endl;

  cerr << "Testing simple generator..." << endl;
  source->SetStartWithTree(true);
  source->Update();
  g = source->GetOutput();
  if (g->GetNumberOfVertices() != 100)
  {
    cerr << "ERROR: Wrong number of vertices ("
         << g->GetNumberOfVertices() << " != " << 100 << ")" << endl;
    errors++;
  }
  if (g->GetNumberOfEdges() != 299)
  {
    cerr << "ERROR: Wrong number of edges ("
         << g->GetNumberOfEdges() << " != " << 299 << ")" << endl;
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
    cerr << "ERROR: Starting with tree was not connected."
         << "Only " << numVisited << " of "
         << g->GetNumberOfVertices() << " were connected." << endl;
    errors++;
  }
  cerr << "...done." << endl;

  return errors;
}
