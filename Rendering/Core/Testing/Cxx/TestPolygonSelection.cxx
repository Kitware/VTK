/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonSelection.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkExtractSelectedPolyDataIds.h"
#include "vtkHardwareSelector.h"
#include "vtkIntArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleDrawPolygon.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

const char eventLog[] =
  "# StreamVersion 1\n"
  "RenderEvent 0 0 0 0 0 0 0\n"
  "EnterEvent 278 0 0 0 0 0 0\n"
  "MouseMoveEvent 278 0 0 0 0 0 0\n"
  "MouseMoveEvent 274 8 0 0 0 0 0\n"
  "MouseMoveEvent 144 44 0 0 0 0 0\n"
  "MouseMoveEvent 144 43 0 0 0 0 0\n"
  "LeftButtonPressEvent 144 43 0 0 0 0 0\n"
  "StartInteractionEvent 144 43 0 0 0 0 0\n"
  "MouseMoveEvent 143 43 0 0 0 0 0\n"
  "MouseMoveEvent 29 43 0 0 0 0 0\n"
  "MouseMoveEvent 29 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 278 0 0 0 0 0\n"
  "LeftButtonReleaseEvent 146 278 0 0 0 0 0\n"
  "EndInteractionEvent 146 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 278 0 0 0 0 0\n"
  "MouseMoveEvent 146 279 0 0 0 0 0\n"
  "MouseMoveEvent 146 280 0 0 0 0 0\n"
  "MouseMoveEvent 294 207 0 0 0 0 0\n"
  "LeaveEvent 294 207 0 0 0 0 0\n"
  ;

int TestPolygonSelection( int argc, char* argv[] )
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetRadius(0.5);

  vtkNew<vtkActor> sactor;
  sactor->PickableOn(); //lets the HardwareSelector select in it
  vtkNew<vtkPolyDataMapper> smapper;
  sactor->SetMapper(smapper.GetPointer());

  vtkNew<vtkRenderer> ren;
  ren->AddActor(sactor.GetPointer());
  // extracted part
  vtkNew<vtkPolyDataMapper> emapper;
  vtkNew<vtkActor> eactor;
  eactor->PickableOff();
  eactor->SetMapper(emapper.GetPointer());
  ren->AddActor(eactor.GetPointer());

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300,300);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren.GetPointer());

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin.GetPointer());

  //use the draw-polygon interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkNew<vtkInteractorStyleDrawPolygon> polyStyle;
  polyStyle->DrawPolygonPixelsOff();
  rwi->SetInteractorStyle(polyStyle.GetPointer());

  // record events
  vtkNew<vtkInteractorEventRecorder> recorder;
  recorder->SetInteractor(rwi);

#ifdef RECORD
  recorder->SetFileName("record.log");
  recorder->On();
  recorder->Record();
#else
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog);
#endif

  smapper->SetInputConnection(sphere->GetOutputPort());

  iren->Initialize();
  renWin->Render();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  renWin->Render();

  std::vector<vtkVector2i> points = polyStyle->GetPolygonPoints();
  if(points.size() >= 3)
  {
    vtkNew<vtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(points.size());
    for (unsigned int j = 0; j < points.size(); ++j)
    {
      const vtkVector2i &v = points[j];
      int pos[2] = {v[0], v[1]};
      polygonPointsArray->SetTypedTuple(j, pos);
    }

    vtkNew<vtkHardwareSelector> hardSel;
    hardSel->SetRenderer(ren.GetPointer());

    int* wsize = ren->GetSize();
    int* origin = ren->GetOrigin();
    hardSel->SetArea(origin[0], origin[1], origin[0]+wsize[0]-1, origin[1]+wsize[1]-1);
    hardSel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);

    if (hardSel->CaptureBuffers())
    {
      vtkSelection* psel = hardSel->GeneratePolygonSelection(
        polygonPointsArray->GetPointer(0),
        polygonPointsArray->GetNumberOfTuples()*2);
      hardSel->ClearBuffers();

      vtkSmartPointer<vtkSelection> sel;
      sel.TakeReference(psel);
      vtkNew<vtkExtractSelectedPolyDataIds> selFilter;
      selFilter->SetInputConnection(0,sphere->GetOutputPort());
      selFilter->SetInputData(1, sel);
      selFilter->Update();

      emapper->SetInputConnection(selFilter->GetOutputPort());
      emapper->Update();

      sactor->SetVisibility(false);
      renWin->Render();
    }
  }
  int retVal = vtkRegressionTestImage( renWin.GetPointer() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
