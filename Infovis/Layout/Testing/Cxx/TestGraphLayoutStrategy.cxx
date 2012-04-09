/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGraphLayoutStrategy.cxx

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
#include "vtkCircularLayoutStrategy.h"
#include "vtkEdgeListIterator.h"
#include "vtkFast2DLayoutStrategy.h"
#include "vtkForceDirectedLayoutStrategy.h"
#include "vtkGraphLayout.h"
#include "vtkMath.h"
#include "vtkPassThroughLayoutStrategy.h"
#include "vtkRandomGraphSource.h"
#include "vtkRandomLayoutStrategy.h"
#include "vtkSimple2DLayoutStrategy.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTreeLayoutStrategy.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

int TestGraphLayoutStrategy(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{
  int errors = 0;
  
  // Create input graph
  vtkIdType numVert = 100;
  vtkIdType numEdges = 150;
  VTK_CREATE(vtkRandomGraphSource, source);
  source->SetNumberOfVertices(numVert);
  source->SetNumberOfEdges(numEdges);

  VTK_CREATE(vtkGraphLayout, layout);
  layout->SetInputConnection(source->GetOutputPort());
  vtkGraph *output = NULL;
  double pt[3] = {0.0, 0.0, 0.0};
  double pt2[3] = {0.0, 0.0, 0.0};
  double eps = 1.0e-6;
  double length = 0.0;
  double tol = 50.0;
  
  cerr << "Testing vtkCircularLayoutStrategy..." << endl;
  VTK_CREATE(vtkCircularLayoutStrategy, circular);
  layout->SetLayoutStrategy(circular);
  layout->Update();
  output = layout->GetOutput();
  for (vtkIdType i = 0; i < numVert; i++)
    {
    output->GetPoint(i, pt);
    double dist = pt[0]*pt[0] + pt[1]*pt[1] - 1.0;
    dist = dist > 0 ? dist : -dist;
    if (dist > eps || pt[2] != 0.0)
      {
      cerr << "ERROR: Point " << i << " is not on the unit circle." << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
  
  cerr << "Testing vtkFast2DLayoutStrategy..." << endl;
  VTK_CREATE(vtkFast2DLayoutStrategy, fast);
  fast->SetRestDistance(1.0f);
  length = fast->GetRestDistance();
  layout->SetLayoutStrategy(fast);
  layout->Update();
  output = layout->GetOutput();
  VTK_CREATE(vtkEdgeListIterator, edges);
  output->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    vtkIdType u = e.Source;
    vtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(vtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length/tol || dist > length*tol)
      {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
      }
    if (pt[2] != 0.0)
      {
      cerr << "ERROR: Point " << u << " not on the xy plane" << endl;
      errors++;
      }
    if (pt2[2] != 0.0)
      {
      cerr << "ERROR: Point " << v << " not on the xy plane" << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
  
  cerr << "Testing vtkForceDirectedLayoutStrategy..." << endl;
  VTK_CREATE(vtkForceDirectedLayoutStrategy, force);
  length = pow(1.0/numVert, 1.0/3.0);
  layout->SetLayoutStrategy(force);
  layout->Update();
  output = layout->GetOutput();
  output->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    vtkIdType u = e.Source;
    vtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(vtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length/tol || dist > length*tol)
      {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
  
  cerr << "Testing vtkPassThroughLayoutStrategy..." << endl;
  VTK_CREATE(vtkPassThroughLayoutStrategy, pass);
  layout->SetLayoutStrategy(pass);
  layout->Update();
  output = layout->GetOutput();
  for (vtkIdType i = 0; i < numVert; i++)
    {
    output->GetPoint(i, pt);
    if (pt[0] != 0.0 || pt[1] != 0.0 || pt[2] != 0.0)
      {
      cerr << "ERROR: Point " << i << " is not 0,0,0." << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
  
  cerr << "Testing vtkRandomLayoutStrategy..." << endl;
  VTK_CREATE(vtkRandomLayoutStrategy, random);
  double bounds[6] = {0, 0, 0, 0, 0, 0};
  random->GetGraphBounds(bounds);
  layout->SetLayoutStrategy(random);
  layout->Update();
  output = layout->GetOutput();
  for (vtkIdType i = 0; i < numVert; i++)
    {
    output->GetPoint(i, pt);
    if (pt[0] < bounds[0] || pt[0] > bounds[1]
     || pt[1] < bounds[2] || pt[1] > bounds[3]
     || pt[2] < bounds[4] || pt[2] > bounds[5])
      {
      cerr << "ERROR: Point " << i << " is not within the bounds." << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
  
  cerr << "Testing vtkSimple2DLayoutStrategy..." << endl;
  VTK_CREATE(vtkSimple2DLayoutStrategy, simple);
  simple->SetRestDistance(1.0f);
  length = simple->GetRestDistance();
  layout->SetLayoutStrategy(simple);
  layout->Update();
  output = layout->GetOutput();
  output->GetEdges(edges);
  while (edges->HasNext())
    {
    vtkEdgeType e = edges->Next();
    vtkIdType u = e.Source;
    vtkIdType v = e.Target;
    output->GetPoint(u, pt);
    output->GetPoint(v, pt2);
    double dist = sqrt(vtkMath::Distance2BetweenPoints(pt, pt2));
    if (dist < length/tol || dist > length*tol)
      {
      cerr << "ERROR: Edge " << u << "," << v << " distance is " << dist
           << " but resting distance is " << length << endl;
      errors++;
      }
    if (pt[2] != 0.0)
      {
      cerr << "ERROR: Point " << u << " not on the xy plane" << endl;
      errors++;
      }
    if (pt2[2] != 0.0)
      {
      cerr << "ERROR: Point " << v << " not on the xy plane" << endl;
      errors++;
      }
    }
  cerr << "...done." << endl;
    
  return errors;
}
