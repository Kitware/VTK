/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests picking a vtkFollower and vtkProp3DFollower
//
#include "vtkJPEGReader.h"
#include "vtkImageActor.h"
#include "vtkImageMapper3D.h"
#include "vtkPlaneSource.h"
#include "vtkPolyDataMapper.h"
#include "vtkFollower.h"
#include "vtkProp3DFollower.h"
#include "vtkPropPicker.h"
#include "vtkCellPicker.h"
#include "vtkStructuredPointsReader.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCommand.h"
#include "vtkInteractorEventRecorder.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkSphereSource.h"
#include "vtkRegressionTestImage.h"

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// -----------------------------------------------------------------------
// This does the actual work: updates the vtkPline implicit function.
// This in turn causes the pipeline to update and clip the object.
// Callback for the interaction
class vtkPickFollowerCallback : public vtkCommand
{
public:
  static vtkPickFollowerCallback *New()
    { return new vtkPickFollowerCallback; }
  virtual void Execute(vtkObject *caller, unsigned long, void*)
    {
//      vtkPropPicker *picker = reinterpret_cast<vtkPropPicker*>(caller);
      vtkCellPicker *picker = reinterpret_cast<vtkCellPicker*>(caller);
      if ( picker->GetViewProp() != NULL )
        {
        cout << "Picked\n";
        }
    }

  vtkPickFollowerCallback() {}
};

// -----------------------------------------------------------------------
char PickFollowerLog[] =
  "# StreamVersion 1\n"
  "RenderEvent 0 0 0 0 0 0 0\n"
  "EnterEvent 285 289 0 0 0 0 0\n"
  "MouseMoveEvent 285 289 0 0 0 0 0\n"
  "MouseMoveEvent 271 294 0 0 0 0 0\n"
  "LeaveEvent 271 294 0 0 0 0 0\n"
  "EnterEvent 136 299 0 0 0 0 0\n"
  "MouseMoveEvent 136 299 0 0 0 0 0\n"
  "MouseMoveEvent 136 294 0 0 0 0 0\n"
  "MouseMoveEvent 136 291 0 0 0 0 0\n"
  "MouseMoveEvent 136 286 0 0 0 0 0\n"
  "MouseMoveEvent 136 280 0 0 0 0 0\n"
  "MouseMoveEvent 136 277 0 0 0 0 0\n"
  "MouseMoveEvent 136 274 0 0 0 0 0\n"
  "MouseMoveEvent 136 271 0 0 0 0 0\n"
  "MouseMoveEvent 136 268 0 0 0 0 0\n"
  "MouseMoveEvent 136 262 0 0 0 0 0\n"
  "MouseMoveEvent 136 260 0 0 0 0 0\n"
  "MouseMoveEvent 136 256 0 0 0 0 0\n"
  "MouseMoveEvent 136 253 0 0 0 0 0\n"
  "MouseMoveEvent 136 251 0 0 0 0 0\n"
  "MouseMoveEvent 136 249 0 0 0 0 0\n"
  "MouseMoveEvent 136 247 0 0 0 0 0\n"
  "MouseMoveEvent 136 246 0 0 0 0 0\n"
  "MouseMoveEvent 136 245 0 0 0 0 0\n"
  "MouseMoveEvent 136 243 0 0 0 0 0\n"
  "MouseMoveEvent 137 241 0 0 0 0 0\n"
  "MouseMoveEvent 137 240 0 0 0 0 0\n"
  "MouseMoveEvent 137 238 0 0 0 0 0\n"
  "MouseMoveEvent 137 237 0 0 0 0 0\n"
  "MouseMoveEvent 137 236 0 0 0 0 0\n"
  "MouseMoveEvent 137 234 0 0 0 0 0\n"
  "MouseMoveEvent 138 231 0 0 0 0 0\n"
  "MouseMoveEvent 138 230 0 0 0 0 0\n"
  "MouseMoveEvent 138 229 0 0 0 0 0\n"
  "MouseMoveEvent 138 227 0 0 0 0 0\n"
  "MouseMoveEvent 139 227 0 0 0 0 0\n"
  "MouseMoveEvent 139 226 0 0 0 0 0\n"
  "MouseMoveEvent 139 225 0 0 0 0 0\n"
  "MouseMoveEvent 139 224 0 0 0 0 0\n"
  "KeyPressEvent 139 224 0 0 114 1 r\n"
  "CharEvent 139 224 0 0 114 1 r\n"
  "RenderEvent 139 224 0 0 114 1 r\n"
  "MouseMoveEvent 140 223 0 0 0 0 r\n"
  "MouseMoveEvent 141 223 0 0 0 0 r\n"
  "KeyReleaseEvent 141 223 0 0 114 1 r\n"
  "MouseMoveEvent 142 223 0 0 0 0 r\n"
  "MouseMoveEvent 143 223 0 0 0 0 r\n"
  "MouseMoveEvent 144 223 0 0 0 0 r\n"
  "MouseMoveEvent 145 223 0 0 0 0 r\n"
  "MouseMoveEvent 146 223 0 0 0 0 r\n"
  "MouseMoveEvent 146 222 0 0 0 0 r\n"
  "MouseMoveEvent 146 221 0 0 0 0 r\n"
  "MouseMoveEvent 146 220 0 0 0 0 r\n"
  "MouseMoveEvent 146 219 0 0 0 0 r\n"
  "MouseMoveEvent 146 218 0 0 0 0 r\n"
  "MouseMoveEvent 146 217 0 0 0 0 r\n"
  "MouseMoveEvent 146 216 0 0 0 0 r\n"
  "MouseMoveEvent 146 215 0 0 0 0 r\n"
  "MouseMoveEvent 146 213 0 0 0 0 r\n"
  "MouseMoveEvent 146 212 0 0 0 0 r\n"
  "MouseWheelBackwardEvent 146 212 0 0 0 0 r\n"
  "StartInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "EndInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "MouseWheelBackwardEvent 146 212 0 0 0 0 r\n"
  "StartInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "EndInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "MouseWheelBackwardEvent 146 212 0 0 0 0 r\n"
  "StartInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "EndInteractionEvent 146 212 0 0 0 0 r\n"
  "RenderEvent 146 212 0 0 0 0 r\n"
  "MouseMoveEvent 146 209 0 0 0 0 r\n"
  "MouseMoveEvent 146 205 0 0 0 0 r\n"
  "MouseMoveEvent 146 202 0 0 0 0 r\n"
  "MouseMoveEvent 146 199 0 0 0 0 r\n"
  "MouseMoveEvent 146 197 0 0 0 0 r\n"
  "MouseMoveEvent 146 196 0 0 0 0 r\n"
  "MouseMoveEvent 146 194 0 0 0 0 r\n"
  "MouseMoveEvent 147 193 0 0 0 0 r\n"
  "MouseMoveEvent 147 191 0 0 0 0 r\n"
  "MouseMoveEvent 148 190 0 0 0 0 r\n"
  "MouseMoveEvent 149 189 0 0 0 0 r\n"
  "MouseMoveEvent 149 186 0 0 0 0 r\n"
  "MouseMoveEvent 149 185 0 0 0 0 r\n"
  "MouseMoveEvent 149 184 0 0 0 0 r\n"
  "MouseMoveEvent 149 183 0 0 0 0 r\n"
  "MouseMoveEvent 149 182 0 0 0 0 r\n"
  "MouseMoveEvent 149 181 0 0 0 0 r\n"
  "MouseMoveEvent 149 180 0 0 0 0 r\n"
  "MouseMoveEvent 149 179 0 0 0 0 r\n"
  "MouseMoveEvent 149 178 0 0 0 0 r\n"
  "MouseMoveEvent 149 177 0 0 0 0 r\n"
  "MouseMoveEvent 149 176 0 0 0 0 r\n"
  "MouseMoveEvent 149 175 0 0 0 0 r\n"
  "MouseMoveEvent 149 174 0 0 0 0 r\n"
  "MouseMoveEvent 149 173 0 0 0 0 r\n"
  "MouseMoveEvent 149 172 0 0 0 0 r\n"
  "MouseMoveEvent 149 171 0 0 0 0 r\n"
  "KeyPressEvent 149 171 0 0 112 1 p\n"
  "CharEvent 149 171 0 0 112 1 p\n"
  "StartPickEvent 149 171 0 0 112 1 p\n"
  "RenderEvent 149 171 0 0 112 1 p\n"
  "EndPickEvent 149 171 0 0 112 1 p\n"
  "KeyReleaseEvent 149 171 0 0 112 1 p\n"
  "MouseMoveEvent 151 169 0 0 0 0 p\n"
  "MouseMoveEvent 154 169 0 0 0 0 p\n"
  "MouseMoveEvent 164 166 0 0 0 0 p\n"
  "MouseMoveEvent 174 166 0 0 0 0 p\n"
  "MouseMoveEvent 182 165 0 0 0 0 p\n"
  "MouseMoveEvent 190 165 0 0 0 0 p\n"
  "MouseMoveEvent 194 165 0 0 0 0 p\n"
  "MouseMoveEvent 200 165 0 0 0 0 p\n"
  "MouseMoveEvent 204 164 0 0 0 0 p\n"
  "MouseMoveEvent 208 164 0 0 0 0 p\n"
  "MouseMoveEvent 215 163 0 0 0 0 p\n"
  "MouseMoveEvent 218 163 0 0 0 0 p\n"
  "MouseMoveEvent 221 163 0 0 0 0 p\n"
  "MouseMoveEvent 226 163 0 0 0 0 p\n"
  "MouseMoveEvent 230 163 0 0 0 0 p\n"
  "MouseMoveEvent 233 163 0 0 0 0 p\n"
  "MouseMoveEvent 234 163 0 0 0 0 p\n"
  "MouseMoveEvent 236 163 0 0 0 0 p\n"
  "MouseMoveEvent 237 163 0 0 0 0 p\n"
  "MouseMoveEvent 238 163 0 0 0 0 p\n"
  "MouseMoveEvent 239 163 0 0 0 0 p\n"
  "MouseMoveEvent 240 163 0 0 0 0 p\n"
  "MouseMoveEvent 242 163 0 0 0 0 p\n"
  "MouseMoveEvent 244 163 0 0 0 0 p\n"
  "MouseMoveEvent 245 163 0 0 0 0 p\n"
  "MouseMoveEvent 246 163 0 0 0 0 p\n"
  "MouseMoveEvent 247 163 0 0 0 0 p\n"
  "MouseMoveEvent 248 163 0 0 0 0 p\n"
  "KeyPressEvent 248 163 0 0 112 1 p\n"
  "CharEvent 248 163 0 0 112 1 p\n"
  "StartPickEvent 248 163 0 0 112 1 p\n"
  "RenderEvent 248 163 0 0 112 1 p\n"
  "EndPickEvent 248 163 0 0 112 1 p\n"
  "KeyReleaseEvent 248 163 0 0 112 1 p\n"
  "MouseMoveEvent 246 163 0 0 0 0 p\n"
  "MouseMoveEvent 243 163 0 0 0 0 p\n"
  "MouseMoveEvent 241 163 0 0 0 0 p\n"
  "MouseMoveEvent 238 163 0 0 0 0 p\n"
  "MouseMoveEvent 234 164 0 0 0 0 p\n"
  "MouseMoveEvent 229 164 0 0 0 0 p\n"
  "MouseMoveEvent 222 165 0 0 0 0 p\n"
  "MouseMoveEvent 215 165 0 0 0 0 p\n"
  "MouseMoveEvent 206 165 0 0 0 0 p\n"
  "MouseMoveEvent 200 165 0 0 0 0 p\n"
  "MouseMoveEvent 194 164 0 0 0 0 p\n"
  "MouseMoveEvent 192 163 0 0 0 0 p\n"
  "MouseMoveEvent 192 162 0 0 0 0 p\n"
  "MouseMoveEvent 191 162 0 0 0 0 p\n"
  "MouseMoveEvent 190 162 0 0 0 0 p\n"
  "MouseMoveEvent 189 162 0 0 0 0 p\n"
  "MouseMoveEvent 188 162 0 0 0 0 p\n"
  "MouseMoveEvent 186 162 0 0 0 0 p\n"
  "MouseMoveEvent 185 162 0 0 0 0 p\n"
  "MouseMoveEvent 183 162 0 0 0 0 p\n"
  "MouseMoveEvent 182 162 0 0 0 0 p\n"
  "MouseMoveEvent 180 162 0 0 0 0 p\n"
  "MouseMoveEvent 179 162 0 0 0 0 p\n"
  "MouseMoveEvent 178 162 0 0 0 0 p\n"
  "MouseMoveEvent 176 162 0 0 0 0 p\n"
  "MouseMoveEvent 175 162 0 0 0 0 p\n"
  "MouseMoveEvent 173 162 0 0 0 0 p\n"
  "KeyPressEvent 173 162 0 0 112 1 p\n"
  "CharEvent 173 162 0 0 112 1 p\n"
  "StartPickEvent 173 162 0 0 112 1 p\n"
  "RenderEvent 173 162 0 0 112 1 p\n"
  "EndPickEvent 173 162 0 0 112 1 p\n"
  "KeyReleaseEvent 173 162 0 0 112 1 p\n"
;

// -----------------------------------------------------------------------
int TestFollowerPicking( int argc, char* argv[] )
{
  // Create some simple actors
  //
  VTK_CREATE(vtkPlaneSource, plane);

  VTK_CREATE(vtkPolyDataMapper, mapper);
  mapper->SetInputConnection(plane->GetOutputPort());

  VTK_CREATE(vtkFollower, follower);
  follower->SetMapper( mapper);
  follower->SetPosition(1,2,3);

  // Mark the origin
  VTK_CREATE(vtkSphereSource,ss);
  VTK_CREATE(vtkPolyDataMapper,m);
  m->SetInputConnection(ss->GetOutputPort());
  VTK_CREATE(vtkActor,a);
  a->SetMapper(m);

  // Create a more complex follower
  char *fname =
    vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.jpg");
  VTK_CREATE(vtkJPEGReader, pnmReader);
  pnmReader->SetFileName(fname);
  delete[] fname;

  VTK_CREATE(vtkImageActor,ia);
  ia->GetMapper()->SetInputConnection(pnmReader->GetOutputPort());
  ia->SetScale(0.01,0.01,0.01);

  VTK_CREATE(vtkProp3DFollower,p3dFollower);
  p3dFollower->SetProp3D(ia);

  // Debugging code
  VTK_CREATE(vtkPlaneSource, plane2);

  VTK_CREATE(vtkPolyDataMapper, mapper2);
  mapper2->SetInputConnection(plane2->GetOutputPort());

  VTK_CREATE(vtkActor, actor);
  actor->SetMapper( mapper2);
  actor->AddPosition(1,2,3);
  actor->GetProperty()->SetRepresentationToWireframe();
  actor->GetProperty()->SetColor(0,1,0);

  // Picking callback
  VTK_CREATE(vtkPickFollowerCallback, myCallback);

//  VTK_CREATE(vtkPropPicker,picker);
  VTK_CREATE(vtkCellPicker,picker);
  picker->AddObserver(vtkCommand::EndPickEvent,myCallback);

  // Create the rendering machinary
  //
  VTK_CREATE(vtkRenderer, ren1);
  follower->SetCamera(ren1->GetActiveCamera());
  p3dFollower->SetCamera(ren1->GetActiveCamera());

  VTK_CREATE(vtkRenderWindow, renWin);
  renWin->AddRenderer(ren1);
  // Turn off antialiasing so all GPUs produce the same image
  renWin->SetMultiSamples(0);

  VTK_CREATE(vtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);
  iren->SetPicker(picker);

//  ren1->AddActor(follower);
  ren1->AddActor(p3dFollower);
//  ren1->AddActor(a);
//  ren1->AddActor(actor);

  // record events
  VTK_CREATE(vtkInteractorEventRecorder, recorder);
  recorder->SetInteractor(iren);
  recorder->SetFileName("record.log");
//  recorder->Record();
  recorder->ReadFromInputStringOn();
  recorder->SetInputString(PickFollowerLog);
  recorder->EnabledOn();

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);
  ren1->ResetCamera();
  iren->Initialize();
  renWin->Render();

  recorder->Play();
  recorder->Off();

  int retVal = vtkRegressionTestImageThreshold( renWin, 10 );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    iren->Start();
    }

  return !retVal;
}
