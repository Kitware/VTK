/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitCylinderWidget.cxx

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
#include "vtkImplicitCylinderWidget.h"
#include "vtkImplicitCylinderRepresentation.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkLODActor.h"
#include "vtkCylinder.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

const char eventLog2[] =
  "# StreamVersion 1\n"
  "CharEvent 108 202 0 0 105 1 i\n"
  "KeyReleaseEvent 108 202 0 0 105 1 i\n"
  "MouseMoveEvent 113 194 0 0 0 0 i\n"
  "MouseMoveEvent 111 190 0 0 0 0 i\n"
  "MouseMoveEvent 109 185 0 0 0 0 i\n"
  "MouseMoveEvent 108 183 0 0 0 0 i\n"
  "RenderEvent 108 183 0 0 0 0 i\n"
  "MouseMoveEvent 105 175 0 0 0 0 i\n"
  "RenderEvent 105 175 0 0 0 0 i\n"
  "MouseMoveEvent 104 169 0 0 0 0 i\n"
  "RenderEvent 104 169 0 0 0 0 i\n"
  "MouseMoveEvent 103 169 0 0 0 0 i\n"
  "RenderEvent 103 169 0 0 0 0 i\n"
  "MouseMoveEvent 103 167 0 0 0 0 i\n"
  "RenderEvent 103 167 0 0 0 0 i\n"
  "MouseMoveEvent 103 167 0 0 0 0 i\n"
  "RenderEvent 103 167 0 0 0 0 i\n"
  "MouseMoveEvent 85 162 0 0 0 0 i\n"
  "RenderEvent 85 162 0 0 0 0 i\n"
  "MouseMoveEvent 68 158 0 0 0 0 i\n"
  "RenderEvent 68 158 0 0 0 0 i\n"
  "MouseMoveEvent 59 156 0 0 0 0 i\n"
  "RenderEvent 59 156 0 0 0 0 i\n"
  "MouseMoveEvent 41 149 0 0 0 0 i\n"
  "RenderEvent 41 149 0 0 0 0 i\n"
  "MouseMoveEvent 39 148 0 0 0 0 i\n"
  "RenderEvent 39 148 0 0 0 0 i\n"
  "LeftButtonPressEvent 39 148 0 0 0 0 i\n"
  "RenderEvent 39 148 0 0 0 0 i\n"
  "MouseMoveEvent 39 147 0 0 0 0 i\n"
  "RenderEvent 39 147 0 0 0 0 i\n"
  "MouseMoveEvent 39 146 0 0 0 0 i\n"
  "RenderEvent 39 146 0 0 0 0 i\n"
  "MouseMoveEvent 39 145 0 0 0 0 i\n"
  "RenderEvent 39 145 0 0 0 0 i\n"
  "MouseMoveEvent 39 143 0 0 0 0 i\n"
  "RenderEvent 39 143 0 0 0 0 i\n"
  "MouseMoveEvent 39 143 0 0 0 0 i\n"
  "RenderEvent 39 143 0 0 0 0 i\n"
  "MouseMoveEvent 39 142 0 0 0 0 i\n"
  "RenderEvent 39 142 0 0 0 0 i\n"
  "MouseMoveEvent 39 141 0 0 0 0 i\n"
  "RenderEvent 39 141 0 0 0 0 i\n"
  "MouseMoveEvent 39 140 0 0 0 0 i\n"
  "RenderEvent 39 140 0 0 0 0 i\n"
  "MouseMoveEvent 40 139 0 0 0 0 i\n"
  "RenderEvent 40 139 0 0 0 0 i\n"
  "MouseMoveEvent 43 134 0 0 0 0 i\n"
  "RenderEvent 43 134 0 0 0 0 i\n"
  "MouseMoveEvent 45 129 0 0 0 0 i\n"
  "RenderEvent 45 129 0 0 0 0 i\n"
  "MouseMoveEvent 45 123 0 0 0 0 i\n"
  "RenderEvent 45 123 0 0 0 0 i\n"
  "MouseMoveEvent 51 108 0 0 0 0 i\n"
  "RenderEvent 51 108 0 0 0 0 i\n"
  "MouseMoveEvent 53 101 0 0 0 0 i\n"
  "RenderEvent 53 101 0 0 0 0 i\n"
  "MouseMoveEvent 55 95 0 0 0 0 i\n"
  "RenderEvent 55 95 0 0 0 0 i\n"
  "MouseMoveEvent 58 89 0 0 0 0 i\n"
  "RenderEvent 58 89 0 0 0 0 i\n"
  "MouseMoveEvent 66 80 0 0 0 0 i\n"
  "RenderEvent 66 80 0 0 0 0 i\n"
  "MouseMoveEvent 89 73 0 0 0 0 i\n"
  "RenderEvent 89 73 0 0 0 0 i\n"
  "MouseMoveEvent 100 73 0 0 0 0 i\n"
  "RenderEvent 100 73 0 0 0 0 i\n"
  "MouseMoveEvent 120 72 0 0 0 0 i\n"
  "RenderEvent 120 72 0 0 0 0 i\n"
  "MouseMoveEvent 125 75 0 0 0 0 i\n"
  "RenderEvent 125 75 0 0 0 0 i\n"
  "MouseMoveEvent 139 81 0 0 0 0 i\n"
  "RenderEvent 139 81 0 0 0 0 i\n"
  "MouseMoveEvent 145 85 0 0 0 0 i\n"
  "RenderEvent 145 85 0 0 0 0 i\n"
  "MouseMoveEvent 149 87 0 0 0 0 i\n"
  "RenderEvent 149 87 0 0 0 0 i\n"
  "MouseMoveEvent 157 87 0 0 0 0 i\n"
  "RenderEvent 157 87 0 0 0 0 i\n"
  "MouseMoveEvent 163 87 0 0 0 0 i\n"
  "RenderEvent 163 87 0 0 0 0 i\n"
  "MouseMoveEvent 166 87 0 0 0 0 i\n"
  "RenderEvent 166 87 0 0 0 0 i\n"
  "MouseMoveEvent 170 85 0 0 0 0 i\n"
  "RenderEvent 170 85 0 0 0 0 i\n"
  "MouseMoveEvent 172 80 0 0 0 0 i\n"
  "RenderEvent 172 80 0 0 0 0 i\n"
  "MouseMoveEvent 173 76 0 0 0 0 i\n"
  "RenderEvent 173 76 0 0 0 0 i\n"
  "MouseMoveEvent 173 75 0 0 0 0 i\n"
  "RenderEvent 173 75 0 0 0 0 i\n"
  "MouseMoveEvent 174 71 0 0 0 0 i\n"
  "RenderEvent 174 71 0 0 0 0 i\n"
  "MouseMoveEvent 174 71 0 0 0 0 i\n"
  "RenderEvent 174 71 0 0 0 0 i\n"
  "MouseMoveEvent 174 70 0 0 0 0 i\n"
  "RenderEvent 174 70 0 0 0 0 i\n"
  "MouseMoveEvent 175 67 0 0 0 0 i\n"
  "RenderEvent 175 67 0 0 0 0 i\n"
  "MouseMoveEvent 175 66 0 0 0 0 i\n"
  "RenderEvent 175 66 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 175 66 0 0 0 0 i\n"
  "RenderEvent 175 66 0 0 0 0 i\n"
  "MouseMoveEvent 175 69 0 0 0 0 i\n"
  "RenderEvent 175 69 0 0 0 0 i\n"
  "MouseMoveEvent 182 95 0 0 0 0 i\n"
  "RenderEvent 182 95 0 0 0 0 i\n"
  "MouseMoveEvent 187 119 0 0 0 0 i\n"
  "RenderEvent 187 119 0 0 0 0 i\n"
  "MouseMoveEvent 192 132 0 0 0 0 i\n"
  "RenderEvent 192 132 0 0 0 0 i\n"
  "MouseMoveEvent 192 133 0 0 0 0 i\n"
  "RenderEvent 192 133 0 0 0 0 i\n"
  "MouseMoveEvent 194 137 0 0 0 0 i\n"
  "RenderEvent 194 137 0 0 0 0 i\n"
  "MouseMoveEvent 194 138 0 0 0 0 i\n"
  "RenderEvent 194 138 0 0 0 0 i\n"
  "MouseMoveEvent 192 141 0 0 0 0 i\n"
  "RenderEvent 192 141 0 0 0 0 i\n"
  "MouseMoveEvent 191 143 0 0 0 0 i\n"
  "RenderEvent 191 143 0 0 0 0 i\n"
  "LeftButtonPressEvent 191 143 0 0 0 0 i\n"
  "RenderEvent 191 143 0 0 0 0 i\n"
  "MouseMoveEvent 191 144 0 0 0 0 i\n"
  "RenderEvent 191 144 0 0 0 0 i\n"
  "MouseMoveEvent 190 153 0 0 0 0 i\n"
  "RenderEvent 190 153 0 0 0 0 i\n"
  "MouseMoveEvent 190 155 0 0 0 0 i\n"
  "RenderEvent 190 155 0 0 0 0 i\n"
  "MouseMoveEvent 189 161 0 0 0 0 i\n"
  "RenderEvent 189 161 0 0 0 0 i\n"
  "MouseMoveEvent 189 166 0 0 0 0 i\n"
  "RenderEvent 189 166 0 0 0 0 i\n"
  "MouseMoveEvent 187 172 0 0 0 0 i\n"
  "RenderEvent 187 172 0 0 0 0 i\n"
  "MouseMoveEvent 185 177 0 0 0 0 i\n"
  "RenderEvent 185 177 0 0 0 0 i\n"
  "MouseMoveEvent 181 185 0 0 0 0 i\n"
  "RenderEvent 181 185 0 0 0 0 i\n"
  "MouseMoveEvent 180 187 0 0 0 0 i\n"
  "RenderEvent 180 187 0 0 0 0 i\n"
  "MouseMoveEvent 179 191 0 0 0 0 i\n"
  "RenderEvent 179 191 0 0 0 0 i\n"
  "MouseMoveEvent 177 197 0 0 0 0 i\n"
  "RenderEvent 177 197 0 0 0 0 i\n"
  "MouseMoveEvent 177 201 0 0 0 0 i\n"
  "RenderEvent 177 201 0 0 0 0 i\n"
  "MouseMoveEvent 175 205 0 0 0 0 i\n"
  "RenderEvent 175 205 0 0 0 0 i\n"
  "MouseMoveEvent 175 207 0 0 0 0 i\n"
  "RenderEvent 175 207 0 0 0 0 i\n"
  "MouseMoveEvent 175 209 0 0 0 0 i\n"
  "RenderEvent 175 209 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 175 209 0 0 0 0 i\n"
  "RenderEvent 175 209 0 0 0 0 i\n"
  "MouseMoveEvent 175 209 0 0 0 0 i\n"
  "RenderEvent 175 209 0 0 0 0 i\n"
  "MouseMoveEvent 181 185 0 0 0 0 i\n"
  "RenderEvent 181 185 0 0 0 0 i\n"
  "MouseMoveEvent 198 155 0 0 0 0 i\n"
  "MouseMoveEvent 199 152 0 0 0 0 i\n"
  "MouseMoveEvent 200 147 0 0 0 0 i\n"
  "MouseMoveEvent 200 141 0 0 0 0 i\n"
  "MouseMoveEvent 200 135 0 0 0 0 i\n"
  "MouseMoveEvent 200 133 0 0 0 0 i\n"
  "MouseMoveEvent 201 131 0 0 0 0 i\n"
  "MouseMoveEvent 201 128 0 0 0 0 i\n"
  "MouseMoveEvent 201 125 0 0 0 0 i\n"
  "MouseMoveEvent 201 121 0 0 0 0 i\n"
  "MouseMoveEvent 201 117 0 0 0 0 i\n"
  "RenderEvent 201 117 0 0 0 0 i\n"
  "MouseMoveEvent 201 111 0 0 0 0 i\n"
  "RenderEvent 201 111 0 0 0 0 i\n"
  "MouseMoveEvent 201 110 0 0 0 0 i\n"
  "RenderEvent 201 110 0 0 0 0 i\n"
  "LeftButtonPressEvent 201 110 0 0 0 0 i\n"
  "RenderEvent 201 110 0 0 0 0 i\n"
  "MouseMoveEvent 199 109 0 0 0 0 i\n"
  "RenderEvent 199 109 0 0 0 0 i\n"
  "MouseMoveEvent 170 102 0 0 0 0 i\n"
  "RenderEvent 170 102 0 0 0 0 i\n"
  "MouseMoveEvent 153 98 0 0 0 0 i\n"
  "RenderEvent 153 98 0 0 0 0 i\n"
  "MouseMoveEvent 139 92 0 0 0 0 i\n"
  "RenderEvent 139 92 0 0 0 0 i\n"
  "MouseMoveEvent 113 81 0 0 0 0 i\n"
  "RenderEvent 113 81 0 0 0 0 i\n"
  "MouseMoveEvent 113 80 0 0 0 0 i\n"
  "RenderEvent 113 80 0 0 0 0 i\n"
  "MouseMoveEvent 113 73 0 0 0 0 i\n"
  "RenderEvent 113 73 0 0 0 0 i\n"
  "MouseMoveEvent 113 73 0 0 0 0 i\n"
  "RenderEvent 113 73 0 0 0 0 i\n"
  "MouseMoveEvent 114 73 0 0 0 0 i\n"
  "RenderEvent 114 73 0 0 0 0 i\n"
  "MouseMoveEvent 115 73 0 0 0 0 i\n"
  "RenderEvent 115 73 0 0 0 0 i\n"
  "MouseMoveEvent 121 75 0 0 0 0 i\n"
  "RenderEvent 121 75 0 0 0 0 i\n"
  "MouseMoveEvent 123 77 0 0 0 0 i\n"
  "RenderEvent 123 77 0 0 0 0 i\n"
  "MouseMoveEvent 124 77 0 0 0 0 i\n"
  "RenderEvent 124 77 0 0 0 0 i\n"
  "LeftButtonReleaseEvent 124 77 0 0 0 0 i\n"
  "RenderEvent 124 77 0 0 0 0 i\n"
  "MouseMoveEvent 124 77 0 0 0 0 i\n"
  "RenderEvent 124 77 0 0 0 0 i\n"
  "MouseMoveEvent 124 77 0 0 0 0 i\n"
  "RenderEvent 124 77 0 0 0 0 i\n"
  "MouseMoveEvent 124 79 0 0 0 0 i\n"
  "RenderEvent 124 79 0 0 0 0 i\n"
  ;

// This does the actual work: updates the vtkCylinder implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkTICWCallback : public vtkCommand
{
public:
  static vtkTICWCallback *New()
  { return new vtkTICWCallback; }
  void Execute(vtkObject *caller, unsigned long, void*) VTK_OVERRIDE
  {
    vtkImplicitCylinderWidget *cylWidget =
      reinterpret_cast<vtkImplicitCylinderWidget*>(caller);
    vtkImplicitCylinderRepresentation *rep =
      reinterpret_cast<vtkImplicitCylinderRepresentation*>(cylWidget->GetRepresentation());
    rep->GetCylinder(this->Cylinder);
    this->Actor->VisibilityOn();
  }
  vtkTICWCallback():Cylinder(0),Actor(0) {}
  vtkCylinder *Cylinder;
  vtkActor *Actor;

};

int TestImplicitCylinderWidget(int vtkNotUsed(argc), char *vtkNotUsed(argv)[])
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

  // This portion of the code clips the mace with the vtkCylinder
  // implicit function. The clipped region is colored green.
  vtkSmartPointer<vtkCylinder> cylinder =
    vtkSmartPointer<vtkCylinder>::New();
  vtkSmartPointer<vtkClipPolyData> clipper =
    vtkSmartPointer<vtkClipPolyData>::New();
  clipper->SetInputConnection(apd->GetOutputPort());
  clipper->SetClipFunction(cylinder);
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

  // The SetInteractor method is how 3D widgets are associated with the render
  // window interactor. Internally, SetInteractor sets up a bunch of callbacks
  // using the Command/Observer mechanism (AddObserver()).
  vtkSmartPointer<vtkTICWCallback> myCallback =
    vtkSmartPointer<vtkTICWCallback>::New();
  myCallback->Cylinder = cylinder;
  myCallback->Actor = selectActor;

  vtkSmartPointer<vtkImplicitCylinderRepresentation> rep =
    vtkSmartPointer<vtkImplicitCylinderRepresentation>::New();
  rep->SetPlaceFactor(1.25);
  rep->PlaceWidget(glyph->GetOutput()->GetBounds());
  rep->SetRadius(0.25);
  rep->GetCylinderProperty()->SetOpacity(0.1);

  vtkSmartPointer<vtkImplicitCylinderWidget> cylWidget =
    vtkSmartPointer<vtkImplicitCylinderWidget>::New();
  cylWidget->SetInteractor(iren);
  cylWidget->SetRepresentation(rep);
  cylWidget->AddObserver(vtkCommand::InteractionEvent,myCallback);

  ren1->AddActor(maceActor);
  ren1->AddActor(selectActor);

  // Add the actors to the renderer, set the background and size
  //
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  vtkSmartPointer<vtkInteractorEventRecorder> recorder =
    vtkSmartPointer<vtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  //recorder->SetFileName("record.log");
  //recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(eventLog2);

  // render the image
  //
  renWin->SetMultiSamples(0);
  iren->Initialize();
  renWin->Render();
  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
