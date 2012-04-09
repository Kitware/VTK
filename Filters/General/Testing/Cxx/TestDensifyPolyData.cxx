/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDensifyPolyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

#include "vtkDensifyPolyData.h"
#include "vtkXMLPolyDataWriter.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkRegressionTestImage.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTestUtilities.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkCellArray.h"
#include "vtkSmartPointer.h"
#include "vtkProperty.h"
#include "vtkPolyDataMapper.h"
#include "vtkPoints.h"
#include "vtkSphereSource.h"

#define VTK_CREATE(type, var) \
  vtkSmartPointer<type> var = vtkSmartPointer<type>::New()

int TestDensifyPolyData(int argc, char *argv[])
{

  vtkPoints *boxPoints = vtkPoints::New();
  boxPoints->InsertNextPoint(-0.5,-0.5,-0.5);
  boxPoints->InsertNextPoint(-0.5,-0.5,0.5);
  boxPoints->InsertNextPoint(-0.5,0.5,0.5);
  boxPoints->InsertNextPoint(-0.5,0.5,-0.5);
  boxPoints->InsertNextPoint(0.5,-0.5,-0.5);
  boxPoints->InsertNextPoint(0.5,0.5,-0.5);
  boxPoints->InsertNextPoint(0.5,-0.5,0.5);
  boxPoints->InsertNextPoint(0.5,0.5,0.023809850216);
  boxPoints->InsertNextPoint(0.5,0.072707727551,0.5);
  boxPoints->InsertNextPoint(-0.014212930575,0.5,0.5);

  vtkPolyData *boxPolydata = vtkPolyData::New();
  vtkCellArray *polys = vtkCellArray::New();
  boxPolydata->SetPolys(polys);
  boxPolydata->SetPoints(boxPoints);
  { vtkIdType ids[] = {0,1,2,3};
    boxPolydata->InsertNextCell(VTK_POLYGON, 4, ids ); }
  { vtkIdType ids[] = {4,5,7,8,6};
    boxPolydata->InsertNextCell(VTK_POLYGON, 5, ids ); }
  { vtkIdType ids[] = {0,4,6,1};
    boxPolydata->InsertNextCell(VTK_POLYGON, 4, ids ); }
  { vtkIdType ids[] = {3,2,9,7,5};
    boxPolydata->InsertNextCell(VTK_POLYGON, 5, ids ); }
  { vtkIdType ids[] = {0,3,5,4};
    boxPolydata->InsertNextCell(VTK_POLYGON, 4, ids ); }
  { vtkIdType ids[] = {1,6,8,9,2};
    boxPolydata->InsertNextCell(VTK_POLYGON, 5, ids ); }
  { vtkIdType ids[] = {7,9,8};
    boxPolydata->InsertNextCell(VTK_POLYGON, 3, ids ); }

  VTK_CREATE( vtkDensifyPolyData, densifyFilter );
  densifyFilter->SetInputData(boxPolydata);
  densifyFilter->SetNumberOfSubdivisions(2);

  vtkXMLPolyDataWriter *writer = vtkXMLPolyDataWriter::New();
  writer->SetInputConnection(densifyFilter->GetOutputPort());
  writer->SetFileName("tessellatedBox.vtp");
  writer->SetDataModeToAscii();
  writer->Update();

  VTK_CREATE( vtkSphereSource, sphere );
  VTK_CREATE( vtkDensifyPolyData, densifyFilter2 );
  densifyFilter2->SetInputConnection(sphere->GetOutputPort());
  densifyFilter2->SetNumberOfSubdivisions(1);


  // Throw the stuff on the screen.
  VTK_CREATE(vtkRenderWindow, renwin);
  renwin->SetMultiSamples(0);
  renwin->SetSize(800, 640);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renwin);

  VTK_CREATE(vtkPolyDataMapper, mapper1);
  mapper1->SetInputData(boxPolydata);

  VTK_CREATE(vtkActor, actor1);
  actor1->SetMapper(mapper1);
  actor1->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer1);
  renderer1->AddActor(actor1);
  renderer1->SetBackground(0.0, 0.5, 0.5);
  renderer1->SetViewport(0, 0, 0.5, 0.5);
  renwin->AddRenderer(renderer1);
  actor1->GetProperty()->SetRepresentationToWireframe();

  VTK_CREATE(vtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(densifyFilter->GetOutputPort());

  VTK_CREATE(vtkActor, actor2);
  actor2->SetMapper(mapper2);
  actor2->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer2);
  renderer2->AddActor(actor2);
  renderer2->SetBackground(0.0, 0.5, 0.5);
  renderer2->SetViewport(0.5, 0.0, 1, 0.5);
  renwin->AddRenderer(renderer2);
  actor2->GetProperty()->SetRepresentationToWireframe();

  VTK_CREATE(vtkPolyDataMapper, mapper3);
  mapper3->SetInputConnection(sphere->GetOutputPort());

  VTK_CREATE(vtkActor, actor3);
  actor3->SetMapper(mapper3);
  actor3->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer3);
  renderer3->AddActor(actor3);
  renderer3->SetBackground(0.0, 0.5, 0.5);
  renderer3->SetViewport(0, 0.5, 0.5, 1);
  renwin->AddRenderer(renderer3);
  actor3->GetProperty()->SetRepresentationToWireframe();

  VTK_CREATE(vtkPolyDataMapper, mapper4);
  mapper4->SetInputConnection(densifyFilter2->GetOutputPort());

  VTK_CREATE(vtkActor, actor4);
  actor4->SetMapper(mapper4);
  actor4->GetProperty()->SetPointSize(3.0f);

  VTK_CREATE(vtkRenderer, renderer4);
  renderer4->AddActor(actor4);
  renderer4->SetBackground(0.0, 0.5, 0.5);
  renderer4->SetViewport(0.5, 0.5, 1, 1);
  renwin->AddRenderer(renderer4);
  actor4->GetProperty()->SetRepresentationToWireframe();


  renwin->Render();

  int retVal = vtkRegressionTestImage(renwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  writer->Delete();
  boxPolydata->Delete();
  boxPoints->Delete();
  polys->Delete();

  return !retVal;
}

