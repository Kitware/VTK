/*=========================================================================

  Program:   Visualization Toolkit
  Module:    SGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example shows how to manually create a structured grid.
// The basic idea is to instantiate vtkStructuredGrid, set its dimensions,
// and then assign points defining the grid coordinate. The number of
// points must equal the number of points implicit in the dimensions
// (i.e., dimX*dimY*dimZ). Also, data attributes (either point or cell)
// can be added to the dataset.
//
//
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkFloatArray.h"
#include "vtkHedgeHog.h"
#include "vtkMath.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkStructuredGrid.h"

int main()
{
  int i, j, k, kOffset, jOffset, offset;
  float x[3], v[3], rMin=0.5, rMax=1.0, deltaRad, deltaZ;
  float radius, theta;
  static int dims[3]={13,11,11};

  // Create the structured grid.
  vtkStructuredGrid *sgrid = vtkStructuredGrid::New();
      sgrid->SetDimensions(dims);

  // We also create the points and vectors. The points
  // form a hemi-cylinder of data.
  vtkFloatArray *vectors = vtkFloatArray::New();
    vectors->SetNumberOfComponents(3);
    vectors->SetNumberOfTuples(dims[0]*dims[1]*dims[2]);
  vtkPoints *points = vtkPoints::New();
    points->Allocate(dims[0]*dims[1]*dims[2]);

  deltaZ = 2.0 / (dims[2]-1);
  deltaRad = (rMax-rMin) / (dims[1]-1);
  v[2]=0.0;
  for ( k=0; k<dims[2]; k++)
  {
    x[2] = -1.0 + k*deltaZ;
    kOffset = k * dims[0] * dims[1];
    for (j=0; j<dims[1]; j++)
    {
      radius = rMin + j*deltaRad;
      jOffset = j * dims[0];
      for (i=0; i<dims[0]; i++)
      {
        theta = i * vtkMath::RadiansFromDegrees(15.0);
        x[0] = radius * cos(theta);
        x[1] = radius * sin(theta);
        v[0] = -x[1];
        v[1] = x[0];
        offset = i + jOffset + kOffset;
        points->InsertPoint(offset,x);
        vectors->InsertTuple(offset,v);
      }
    }
  }
  sgrid->SetPoints(points);
  points->Delete();
  sgrid->GetPointData()->SetVectors(vectors);
  vectors->Delete();

  // We create a simple pipeline to display the data.
  vtkHedgeHog *hedgehog = vtkHedgeHog::New();
      hedgehog->SetInputData(sgrid);
      hedgehog->SetScaleFactor(0.1);

  vtkPolyDataMapper *sgridMapper = vtkPolyDataMapper::New();
      sgridMapper->SetInputConnection(hedgehog->GetOutputPort());
  vtkActor *sgridActor = vtkActor::New();
      sgridActor->SetMapper(sgridMapper);
      sgridActor->GetProperty()->SetColor(0,0,0);

  // Create the usual rendering stuff
  vtkRenderer *renderer = vtkRenderer::New();
  vtkRenderWindow *renWin = vtkRenderWindow::New();
    renWin->AddRenderer(renderer);

  vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::New();
    iren->SetRenderWindow(renWin);

  renderer->AddActor(sgridActor);
  renderer->SetBackground(1,1,1);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Elevation(60.0);
  renderer->GetActiveCamera()->Azimuth(30.0);
  renderer->GetActiveCamera()->Zoom(1.25);
  renWin->SetSize(300,300);

  // interact with data
  renWin->Render();
  iren->Start();

  renderer->Delete();
  renWin->Delete();
  iren->Delete();
  sgrid->Delete();
  hedgehog->Delete();
  sgridMapper->Delete();
  sgridActor->Delete();

  return 0;
}
