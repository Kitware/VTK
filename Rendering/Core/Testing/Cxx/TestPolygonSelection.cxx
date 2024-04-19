// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkDataSetMapper.h"
#include "vtkExtractSelection.h"
#include "vtkHardwareSelector.h"
#include "vtkInformation.h"
#include "vtkIntArray.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkInteractorStyleDrawPolygon.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"

const char eventLog[] = "# StreamVersion 1\n"
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
                        "LeaveEvent 294 207 0 0 0 0 0\n";

int TestPolygonSelection(int argc, char* argv[])
{
  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(16);
  sphere->SetPhiResolution(16);
  sphere->SetRadius(0.5);

  vtkNew<vtkPolyDataMapper> sMapper;
  sMapper->SetInputConnection(sphere->GetOutputPort());

  vtkNew<vtkActor> sActor;
  sActor->PickableOn(); // let the HardwareSelector select in it
  sActor->SetMapper(sMapper);

  vtkNew<vtkRenderer> ren;
  ren->AddActor(sActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // use the draw-polygon interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkNew<vtkInteractorStyleDrawPolygon> polyStyle;
  polyStyle->DrawPolygonPixelsOff();
  rwi->SetInteractorStyle(polyStyle);

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

  iren->Initialize();
  renWin->Render();

#ifndef RECORD
  recorder->Play();
  recorder->Off();
#endif

  renWin->Render();

  std::vector<vtkVector2i> points = polyStyle->GetPolygonPoints();
  if (points.size() >= 3)
  {
    vtkNew<vtkIntArray> polygonPointsArray;
    polygonPointsArray->SetNumberOfComponents(2);
    polygonPointsArray->SetNumberOfTuples(static_cast<vtkIdType>(points.size()));
    for (unsigned int j = 0; j < points.size(); ++j)
    {
      const vtkVector2i& v = points[j];
      int pos[2] = { v[0], v[1] };
      polygonPointsArray->SetTypedTuple(j, pos);
    }

    vtkNew<vtkHardwareSelector> hardSel;
    hardSel->SetRenderer(ren);

    const int* wsize = ren->GetSize();
    const int* origin = ren->GetOrigin();
    hardSel->SetArea(origin[0], origin[1], origin[0] + wsize[0] - 1, origin[1] + wsize[1] - 1);
    hardSel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);

    if (hardSel->CaptureBuffers())
    {
      vtkSmartPointer<vtkSelection> sel;
      sel.TakeReference(hardSel->GeneratePolygonSelection(
        polygonPointsArray->GetPointer(0), polygonPointsArray->GetNumberOfTuples() * 2));
      hardSel->ClearBuffers();

      vtkNew<vtkExtractSelection> selFilter;
      selFilter->SetInputConnection(0, sphere->GetOutputPort());
      selFilter->SetInputData(1, sel);

      vtkNew<vtkDataSetMapper> eMapper;
      eMapper->SetInputConnection(selFilter->GetOutputPort());

      vtkNew<vtkActor> eActor;
      eActor->PickableOff();
      eActor->SetMapper(eMapper);

      ren->RemoveActor(sActor);
      ren->AddActor(eActor);

      renWin->Render();
    }
  }
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
