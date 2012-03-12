/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPlaneWidget2b.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkSmartPointer.h"

#include "vtkActor.h"
#include "vtkAppendPolyData.h"
#include "vtkClipPolyData.h"
#include "vtkCommand.h"
#include "vtkConeSource.h"
#include "vtkGlyph3D.h"
#include "vtkImplicitPlaneWidget2.h"
#include "vtkImplicitPlaneRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkPlane.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRendererCollection.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

double TestImplicitPlaneWidget2bPlaneOrigins[3][3] =
{ { 0, 10, 0 }, { 10, 0 , 0 }, { 0, 0, 0 } };

class vtkTimerCallback : public vtkCommand
{
public:
  static vtkTimerCallback *New()
    {
    vtkTimerCallback *cb = new vtkTimerCallback;
    cb->TimerId = 0;
    cb->Count = 0;
    return cb;
    }

  virtual void Execute(vtkObject *caller, unsigned long eventId,
    void *callData)
    {
    if (vtkCommand::TimerEvent == eventId)
      {
      int tid = * static_cast<int *>(callData);

      if (tid == this->TimerId)
        {
        vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren && iren->GetRenderWindow() && iren->GetRenderWindow()->GetRenderers())
          {
          ++this->Count;

          vtkRenderer *renderer = iren->GetRenderWindow()->GetRenderers()->GetFirstRenderer();

          vtkImplicitPlaneRepresentation * rep =
            vtkImplicitPlaneRepresentation::SafeDownCast( this->Widget->GetRepresentation() );
          rep->SetOrigin( TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3] );

          double b[6];
          for (unsigned int i = 0; i < 3; i++)
            {
            b[2*i] = TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][i] - .625;
            b[2*i+1] = TestImplicitPlaneWidget2bPlaneOrigins[this->Count % 3][i] + .625;
            }
          rep->PlaceWidget( b );
          renderer->ResetCamera();
          this->Widget->Render();


          std::cout << "Origin of the widget = ("
            << TestImplicitPlaneWidget2bPlaneOrigins[this->Count %3][0] << " "
            << TestImplicitPlaneWidget2bPlaneOrigins[this->Count %3][1] << " "
            << TestImplicitPlaneWidget2bPlaneOrigins[this->Count %3][2] << ")" << std::endl;
          std::cout << "Bounds of the widget = ("
            << b[0] << " "
            << b[1] << " "
            << b[2] << " "
            << b[3] << " "
            << b[4] << " "
            << b[5] << ")" << std::endl;
          }
        }
      else if (tid == this->QuitTimerId)
        {
        vtkRenderWindowInteractor *iren = vtkRenderWindowInteractor::SafeDownCast(caller);
        if (iren)
          {
          std::cout << "Calling iren->ExitCallback()..." << std::endl;
          iren->ExitCallback();
          }
        }

      }
    }

  int Count;
  int TimerId;
  int QuitTimerId;
  vtkImplicitPlaneWidget2 *Widget;
};

int TestImplicitPlaneWidget2b( int, char *[] )
{
  // Create a mace out of filters.
  //
  vtkSmartPointer<vtkSphereSource> sphere =
    vtkSmartPointer<vtkSphereSource>::New();
  vtkSmartPointer<vtkConeSource> cone =
    vtkSmartPointer<vtkConeSource>::New();
  vtkSmartPointer<vtkGlyph3D> glyph =
    vtkSmartPointer<vtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);
  glyph->Update();

  // The sphere and spikes are appended into a single polydata.
  // This just makes things simpler to manage.
  vtkSmartPointer<vtkAppendPolyData> apd =
    vtkSmartPointer<vtkAppendPolyData>::New();
  apd->AddInputConnection(glyph->GetOutputPort());
  apd->AddInputConnection(sphere->GetOutputPort());

  vtkSmartPointer<vtkPolyDataMapper> maceMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(apd->GetOutputPort());

  vtkSmartPointer<vtkLODActor> maceActor =
    vtkSmartPointer<vtkLODActor>::New();
  maceActor->SetMapper(maceMapper);
  maceActor->VisibilityOn();

  // This portion of the code clips the mace with the vtkPlanes
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkPlane> plane =
    vtkSmartPointer<vtkPlane>::New();
  vtkSmartPointer<vtkClipPolyData> clipper =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(plane);
  clipper->InsideOutOn();

  vtkSmartPointer<vtkPolyDataMapper> selectMapper =
    vtkSmartPointer<vtkPolyDataMapper>::New();
  selectMapper->SetInputConnection(clipper->GetOutputPort());

  vtkSmartPointer<vtkLODActor> selectActor =
    vtkSmartPointer<vtkLODActor>::New();
  selectActor->SetMapper(selectMapper);
  selectActor->GetProperty()->SetColor(0,1,0);
  selectActor->VisibilityOff();
  selectActor->SetScale(1.01, 1.01, 1.01);

  // Create the RenderWindow, Renderer and both Actors
  //
  vtkSmartPointer<vtkRenderer> ren1 =
    vtkSmartPointer<vtkRenderer>::New();
  vtkSmartPointer<vtkRenderWindow> renWin =
    vtkSmartPointer<vtkRenderWindow>::New();
  renWin->AddRenderer(ren1);

  vtkSmartPointer<vtkRenderWindowInteractor> iren =
    vtkSmartPointer<vtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  vtkSmartPointer<vtkImplicitPlaneRepresentation> rep =
    vtkSmartPointer<vtkImplicitPlaneRepresentation>::New();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());

  vtkSmartPointer<vtkImplicitPlaneWidget2> planeWidget =
    vtkSmartPointer<vtkImplicitPlaneWidget2>::New();
  planeWidget->SetInteractor(iren);
  planeWidget->SetRepresentation(rep);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  planeWidget->SetEnabled(1);
  renWin->Render();

  vtkSmartPointer<vtkTimerCallback> cb =
    vtkSmartPointer<vtkTimerCallback>::New();
  iren->AddObserver(vtkCommand::TimerEvent, cb);
  cb->TimerId = iren->CreateRepeatingTimer(2000); // 3 seconds
  cb->Widget = planeWidget;

  // And create a one shot timer to quit after 10 seconds.
  cb->QuitTimerId = iren->CreateOneShotTimer(10000);

  iren->Start();
  return EXIT_SUCCESS;
}
