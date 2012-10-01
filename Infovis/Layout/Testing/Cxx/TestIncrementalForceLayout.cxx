/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDiagram.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIncrementalForceLayout.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkPoints.h"
#include "vtkRandomGraphSource.h"

//----------------------------------------------------------------------------
int TestIncrementalForceLayout(int, char*[])
{
  vtkNew<vtkRandomGraphSource> source;
  source->SetNumberOfVertices(10);
  source->SetStartWithTree(false);
  source->SetNumberOfEdges(10);
  source->Update();

  vtkGraph *randomGraph = source->GetOutput();
  for (vtkIdType i = 0; i < randomGraph->GetNumberOfVertices(); ++i)
    {
    randomGraph->GetPoints()->SetPoint(i, vtkMath::Random(), vtkMath::Random(), vtkMath::Random());
    }

  vtkNew<vtkIncrementalForceLayout> layout;
  layout->SetGraph(randomGraph);
  layout->SetDistance(20.0f);
  for (vtkIdType i = 0; i < 1000; ++i)
    {
    layout->UpdatePositions();
    }
  for (vtkIdType e = 0; e < randomGraph->GetNumberOfEdges(); ++e)
    {
    if (randomGraph->GetSourceVertex(e) == randomGraph->GetTargetVertex(e))
      {
      continue;
      }
    double p1[3];
    randomGraph->GetPoint(randomGraph->GetSourceVertex(e), p1);
    double p2[3];
    randomGraph->GetPoint(randomGraph->GetTargetVertex(e), p2);
    double dx = p2[0] - p1[0];
    double dy = p2[1] - p1[1];
    double dist = sqrt(dx*dx + dy*dy);
    std::cerr << "Edge distance: " << dist << std::endl;
    if (fabs(dist - 20.0) > 5.0)
      {
      return 1;
      }
    }

  return 0;
}
