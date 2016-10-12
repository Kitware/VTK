/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkRenderWindowInteractor.h"

#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkGraphicsFactory.h"
#include "vtkInteractorStyleSwitchBase.h"
#include "vtkMath.h"
#include "vtkPropPicker.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkRendererCollection.h"
#include "vtkDebugLeaks.h"
#include "vtkObserverMediator.h"
#include "vtkPickingManager.h"

#include <map>


// PIMPL'd class to keep track of timers. It maps the ids returned by CreateTimer()
// to the platform-specific representation for timer ids.
struct vtkTimerStruct
{
  int Id;
  int Type;
  unsigned long Duration;
  vtkTimerStruct() : Id(0),Type(vtkRenderWindowInteractor::OneShotTimer),Duration(10) {}
  vtkTimerStruct(int platformTimerId, int timerType, unsigned long duration)
  {
      this->Id = platformTimerId;
      this->Type = timerType;
      this->Duration = duration;
  }
};


class vtkTimerIdMap : public std::map<int,vtkTimerStruct> {};
typedef std::map<int,vtkTimerStruct>::iterator vtkTimerIdMapIterator;

// Initialize static variable that keeps track of timer ids for
// render window interactors.
static int vtkTimerId = 1;

//----------------------------------------------------------------------------
vtkCxxSetObjectMacro(vtkRenderWindowInteractor,Picker,vtkAbstractPicker);

//----------------------------------------------------------------------
// Construct object so that light follows camera motion.
vtkRenderWindowInteractor::vtkRenderWindowInteractor()
{
  this->RenderWindow    = NULL;
  // Here we are using base, and relying on the graphics factory or standard
  // object factory logic to create the correct instance, which should be the
  // vtkInteractorStyleSwitch when linked to the interactor styles, or
  // vtkInteractorStyleSwitchBase if the style module is not linked.
  this->InteractorStyle = NULL;
  this->SetInteractorStyle(vtkInteractorStyleSwitchBase::New());
  this->InteractorStyle->Delete();

  this->LightFollowCamera = 1;
  this->Initialized = 0;
  this->Enabled = 0;
  this->EnableRender = true;
  this->DesiredUpdateRate = 15;
  // default limit is 3 hours per frame
  this->StillUpdateRate = 0.0001;

  this->Picker = this->CreateDefaultPicker();
  this->Picker->Register(this);
  this->Picker->Delete();

  this->PickingManager = 0;
  vtkPickingManager* pm = this->CreateDefaultPickingManager();
  this->SetPickingManager(pm);
  pm->Delete();

  this->EventPosition[0] = this->LastEventPosition[0] = 0;
  this->EventPosition[1] = this->LastEventPosition[1] = 0;

  for (int i = 0; i < VTKI_MAX_POINTERS; ++i)
  {
    this->EventPositions[i][0] = this->LastEventPositions[i][0] = 0;
    this->EventPositions[i][1] = this->LastEventPositions[i][1] = 0;
  }
  this->PointerIndex = 0;

  this->EventSize[0] = 0;
  this->EventSize[1] = 0;

  this->Size[0] = 0;
  this->Size[1] = 0;

  this->NumberOfFlyFrames = 15;
  this->Dolly = 0.30;

  this->AltKey = 0;
  this->ControlKey = 0;
  this->ShiftKey = 0;
  this->KeyCode = 0;
  this->Rotation = 0;
  this->LastRotation = 0;
  this->Scale = 0;
  this->LastScale = 0;
  this->RepeatCount = 0;
  this->KeySym = 0;
  this->TimerEventId = 0;
  this->TimerEventType = 0;
  this->TimerEventDuration = 0;
  this->TimerEventPlatformId = 0;

  this->TimerMap = new vtkTimerIdMap;
  this->TimerDuration = 10;
  this->ObserverMediator = 0;
  this->HandleEventLoop = false;

  this->UseTDx=false; // 3DConnexion device.

  for (int i=0; i < VTKI_MAX_POINTERS; i++)
  {
    this->PointerIndexLookup[i] = 0;
    this->PointersDown[i] = 0;
  }

  this->RecognizeGestures = true;
  this->PointersDownCount = 0;
  this->CurrentGesture = vtkCommand::StartEvent;
}

//----------------------------------------------------------------------
vtkRenderWindowInteractor::~vtkRenderWindowInteractor()
{
  if (this->InteractorStyle != NULL)
  {
    this->InteractorStyle->UnRegister(this);
  }
  if ( this->Picker)
  {
    this->Picker->UnRegister(this);
  }
  delete [] this->KeySym;
  if ( this->ObserverMediator)
  {
    this->ObserverMediator->Delete();
  }
  delete this->TimerMap;

  this->SetPickingManager(0);
  this->SetRenderWindow(0);
}

//----------------------------------------------------------------------
vtkRenderWindowInteractor *vtkRenderWindowInteractor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret =
    vtkGraphicsFactory::CreateInstance("vtkRenderWindowInteractor");
  if ( ret )
  {
    return static_cast<vtkRenderWindowInteractor *>(ret);
  }

  vtkRenderWindowInteractor *o = new vtkRenderWindowInteractor;
  o->InitializeObjectBase();
  return o;
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::Render()
{
  if (this->RenderWindow && this->Enabled && this->EnableRender)
  {
    this->RenderWindow->Render();
  }
  // outside the above test so that third-party code can redirect
  // the render to the appropriate class
  this->InvokeEvent(vtkCommand::RenderEvent, NULL);
}

//----------------------------------------------------------------------
// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void vtkRenderWindowInteractor::UnRegister(vtkObjectBase *o)
{
  if (this->RenderWindow && this->RenderWindow->GetInteractor() == this &&
      this->RenderWindow != o)
  {
    if (this->GetReferenceCount()+this->RenderWindow->GetReferenceCount() == 3)
    {
      this->RenderWindow->SetInteractor(NULL);
      this->SetRenderWindow(NULL);
    }
  }

  this->vtkObject::UnRegister(o);
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::Start()
{
  // Let the compositing handle the event loop if it wants to.
  if (this->HasObserver(vtkCommand::StartEvent) && !this->HandleEventLoop)
  {
    this->InvokeEvent(vtkCommand::StartEvent,NULL);
    return;
  }

  // As a convenience, initialize if we aren't initialized yet.
  if (!this->Initialized)
  {
    this->Initialize();

    if (!this->Initialized)
    {
      return;
    }
  }

  // Pass execution to the subclass which will run the event loop,
  // this will not return until TerminateApp is called.
  this->StartEventLoop();
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::SetRenderWindow(vtkRenderWindow *aren)
{
  if (this->RenderWindow != aren)
  {
    // to avoid destructor recursion
    vtkRenderWindow *temp = this->RenderWindow;
    this->RenderWindow = aren;
    if (temp != NULL)
    {
      temp->UnRegister(this);
    }
    if (this->RenderWindow != NULL)
    {
      this->RenderWindow->Register(this);
      if (this->RenderWindow->GetInteractor() != this)
      {
        this->RenderWindow->SetInteractor(this);
      }
    }
  }
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::SetInteractorStyle(vtkInteractorObserver *style)
{
  if (this->InteractorStyle != style)
  {
    // to avoid destructor recursion
    vtkInteractorObserver *temp = this->InteractorStyle;
    this->InteractorStyle = style;
    if (temp != NULL)
    {
      temp->SetInteractor(0);
      temp->UnRegister(this);
    }
    if (this->InteractorStyle != NULL)
    {
      this->InteractorStyle->Register(this);
      if (this->InteractorStyle->GetInteractor() != this)
      {
        this->InteractorStyle->SetInteractor(this);
      }
    }
  }
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::UpdateSize(int x,int y)
{
  // if the size changed send this on to the RenderWindow
  if ((x != this->Size[0])||(y != this->Size[1]))
  {
    this->Size[0] = this->EventSize[0] = x;
    this->Size[1] = this->EventSize[1] = y;
    this->RenderWindow->SetSize(x,y);
  }
}

// This function is used to return an index given an ID
// and allocate one if needed
int vtkRenderWindowInteractor::GetPointerIndexForContact(size_t dwID)
{
  for (int i=0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID+1)
    {
      return i;
    }
  }

  for (int i=0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == 0)
    {
      this->PointerIndexLookup[i] = dwID+1;
      return i;
    }
  }

  // Out of contacts
  return -1;
}

// This function is used to return an index given an ID
int vtkRenderWindowInteractor::GetPointerIndexForExistingContact(size_t dwID)
{
  for (int i=0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID+1)
    {
      return i;
    }
  }

  // Not found
  return -1;
}

void vtkRenderWindowInteractor::ClearContact(size_t dwID)
{
  for (int i=0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointerIndexLookup[i] == dwID+1)
    {
      this->PointerIndexLookup[i] = 0;
      return;
    }
  }
}

void vtkRenderWindowInteractor::ClearPointerIndex(int i)
{
  if (i < VTKI_MAX_POINTERS)
  {
    this->PointerIndexLookup[i] = 0;
  }
}

// This function is used to return an index given an ID
bool vtkRenderWindowInteractor::IsPointerIndexSet(int i)
{
  if (i < VTKI_MAX_POINTERS)
  {
    return (this->PointerIndexLookup[i] != 0);
  }
  return false;
}

//----------------------------------------------------------------------
// Creates an instance of vtkPropPicker by default
vtkAbstractPropPicker *vtkRenderWindowInteractor::CreateDefaultPicker()
{
  return vtkPropPicker::New();
}

//----------------------------------------------------------------------
// Creates an instance of vtkPickingManager by default
vtkPickingManager* vtkRenderWindowInteractor::CreateDefaultPickingManager()
{
  return vtkPickingManager::New();
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::SetPickingManager(vtkPickingManager* pm)
{
  if(this->PickingManager == pm)
  {
    return;
  }

  vtkPickingManager* tempPickingManager = this->PickingManager;
  this->PickingManager = pm;
  if (this->PickingManager)
  {
    this->PickingManager->Register(this);
    this->PickingManager->SetInteractor(this);
  }

  if(tempPickingManager)
  {
    tempPickingManager->SetInteractor(0);
    tempPickingManager->UnRegister(this);
  }

  this->Modified();
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::ExitCallback()
{
  if (this->HasObserver(vtkCommand::ExitEvent))
  {
    this->InvokeEvent(vtkCommand::ExitEvent,NULL);
  }
  else
  {
    this->TerminateApp();
  }
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::UserCallback()
{
  this->InvokeEvent(vtkCommand::UserEvent,NULL);
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::StartPickCallback()
{
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::EndPickCallback()
{
  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::FlyTo(vtkRenderer *ren, double x, double y, double z)
{
  double flyFrom[3], flyTo[3];
  double d[3], focalPt[3];
  int i, j;

  flyTo[0]=x; flyTo[1]=y; flyTo[2]=z;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);
  for (i=0; i<3; i++)
  {
    d[i] = flyTo[i] - flyFrom[i];
  }
  double distance = vtkMath::Normalize(d);
  double delta = distance/this->NumberOfFlyFrames;

  for (i=1; i<=NumberOfFlyFrames; i++)
  {
    for (j=0; j<3; j++)
    {
      focalPt[j] = flyFrom[j] + d[j]*i*delta;
    }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->Dolly(this->Dolly/this->NumberOfFlyFrames + 1.0);
    ren->GetActiveCamera()->OrthogonalizeViewUp();
    ren->ResetCameraClippingRange();
    this->Render();
  }
}

//----------------------------------------------------------------------
void vtkRenderWindowInteractor::FlyToImage(vtkRenderer *ren, double x, double y)
{
  double flyFrom[3], flyTo[3];
  double d[3], focalPt[3], position[3], positionFrom[3];
  int i, j;

  flyTo[0]=x; flyTo[1]=y;
  ren->GetActiveCamera()->GetFocalPoint(flyFrom);  flyTo[2] = flyFrom[2];
  ren->GetActiveCamera()->GetPosition(positionFrom);
  for (i=0; i<2; i++)
  {
    d[i] = flyTo[i] - flyFrom[i];
  }
  d[2] = 0.0;
  double distance = vtkMath::Normalize(d);
  double delta = distance/this->NumberOfFlyFrames;

  for (i=1; i<=NumberOfFlyFrames; i++)
  {
    for (j=0; j<3; j++)
    {
      focalPt[j] = flyFrom[j] + d[j]*i*delta;
      position[j] = positionFrom[j] + d[j]*i*delta;
    }
    ren->GetActiveCamera()->SetFocalPoint(focalPt);
    ren->GetActiveCamera()->SetPosition(position);
    ren->GetActiveCamera()->Dolly(this->Dolly/this->NumberOfFlyFrames + 1.0);
    ren->ResetCameraClippingRange();
    this->Render();
  }
}

//----------------------------------------------------------------------------
vtkRenderer* vtkRenderWindowInteractor::FindPokedRenderer(int x,int y)
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;
  vtkRenderer *currentRenderer=NULL, *interactiveren=NULL, *viewportren=NULL;
  int numRens, i;

  rc = this->RenderWindow->GetRenderers();
  numRens = rc->GetNumberOfItems();

  for (i = numRens -1; (i >= 0) && !currentRenderer; i--)
  {
    aren = static_cast<vtkRenderer *>(rc->GetItemAsObject(i));
    if (aren->IsInViewport(x,y) && aren->GetInteractive())
    {
      currentRenderer = aren;
    }

    if (interactiveren == NULL && aren->GetInteractive())
    {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      interactiveren = aren;
    }
    if (viewportren == NULL && aren->IsInViewport(x, y))
    {
      // Save this renderer in case we can't find one in the viewport that
      // is interactive.
      viewportren = aren;
    }
  }//for all renderers

  // We must have a value.  If we found an interactive renderer before, that's
  // better than a non-interactive renderer.
  if ( currentRenderer == NULL )
  {
    currentRenderer = interactiveren;
  }

  // We must have a value.  If we found a renderer that is in the viewport,
  // that is better than any old viewport (but not as good as an interactive
  // one).
  if ( currentRenderer == NULL )
  {
    currentRenderer = viewportren;
  }

  // We must have a value - take anything.
  if ( currentRenderer == NULL)
  {
    aren = rc->GetFirstRenderer();
    currentRenderer = aren;
  }

  return currentRenderer;
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetScale(double scale)
{
  this->LastScale = this->Scale;
  if (this->Scale != scale)
  {
    this->Scale = scale;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetRotation(double rot)
{
  this->LastRotation = this->Rotation;
  if (this->Rotation != rot)
  {
    this->Rotation = rot;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::SetTranslation(double val[2])
{
  this->LastTranslation[0] = this->Translation[0];
  this->LastTranslation[1] = this->Translation[1];
  if (this->Translation[0] != val[0] ||
      this->Translation[1] != val[1])
  {
    this->Translation[0] = val[0];
    this->Translation[1] = val[1];
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::RecognizeGesture(vtkCommand::EventIds event)
{
  // we know we are in multitouch now, so start recognizing

  // more than two pointers we ignore
  if (this->PointersDownCount > 2)
  {
    return;
  }

  // store the initial positions
  if (event == vtkCommand::LeftButtonPressEvent)
  {
    for (int i = 0; i < VTKI_MAX_POINTERS; i++)
    {
      if (this->PointersDown[i])
      {
        this->StartingEventPositions[i][0] =
          this->EventPositions[i][0];
        this->StartingEventPositions[i][1] =
          this->EventPositions[i][1];
      }
    }
    // we do not know what the gesture is yet
    this->CurrentGesture = vtkCommand::StartEvent;
    return;
  }

  // end the gesture if needed
  if (event == vtkCommand::LeftButtonReleaseEvent)
  {
    if (this->CurrentGesture == vtkCommand::PinchEvent)
    {
      this->EndPinchEvent();
    }
    if (this->CurrentGesture == vtkCommand::RotateEvent)
    {
      this->EndRotateEvent();
    }
    if (this->CurrentGesture == vtkCommand::PanEvent)
    {
      this->EndPanEvent();
    }
    this->CurrentGesture = vtkCommand::StartEvent;
    return;
  }

  // what are the two pointers we are working with
  int count = 0;
  int *posVals[2];
  int *startVals[2];
  for (int i = 0; i < VTKI_MAX_POINTERS; i++)
  {
    if (this->PointersDown[i])
    {
      posVals[count] = this->EventPositions[i];
      startVals[count] = this->StartingEventPositions[i];
      count++;
    }
  }

  // The meat of the algorithm
  // on move events we analyze them to determine what type
  // of movement it is and then deal with it.
  if (event == vtkCommand::MouseMoveEvent)
  {
    // calculate the distances
    double originalDistance = sqrt(
        static_cast<double>(
        (startVals[0][0] - startVals[1][0])*(startVals[0][0] - startVals[1][0])
        + (startVals[0][1] - startVals[1][1])*(startVals[0][1] - startVals[1][1])));
    double newDistance = sqrt(
        static_cast<double>(
        (posVals[0][0] - posVals[1][0])*(posVals[0][0] - posVals[1][0])
        + (posVals[0][1] - posVals[1][1])*(posVals[0][1] - posVals[1][1])));

    // calculate rotations
    double originalAngle =
      vtkMath::DegreesFromRadians( atan2((double)startVals[1][1] - startVals[0][1],
                                         (double)startVals[1][0] - startVals[0][0]));
    double newAngle =
      vtkMath::DegreesFromRadians( atan2( (double)posVals[1][1] - posVals[0][1],
                                          (double)posVals[1][0] - posVals[0][0]));

    // angles are cyclic so watch for that, 1 and 359 are only 2 apart :)
    double angleDeviation = newAngle - originalAngle;
    newAngle = (newAngle+180.0 >= 360.0 ? newAngle - 180.0 : newAngle + 180.0);
    originalAngle = (originalAngle+180.0 >= 360.0 ? originalAngle - 180.0 : originalAngle + 180.0);
    if (fabs(newAngle - originalAngle) < fabs(angleDeviation))
    {
      angleDeviation = newAngle - originalAngle;
    }

    // calculate the translations
    double trans[2];
      trans[0] = (posVals[0][0] - startVals[0][0] + posVals[1][0] - startVals[1][0])/2.0;
      trans[1] = (posVals[0][1] - startVals[0][1] + posVals[1][1] - startVals[1][1])/2.0;

    // OK we want to
    // - immediately respond to the user
    // - allow the user to zoom without panning (saves focal point)
    // - allow the user to rotate without panning (saves focal point)

    // do we know what gesture we are doing yet? If not
    // see if we can figure it out
    if (this->CurrentGesture == vtkCommand::StartEvent)
    {
      // pinch is a move to/from the center point
      // rotate is a move along the circumference
      // pan is a move of the center point
      // compute the distance along each of these axes in pixels
      // the first to break thresh wins
      double thresh = 0.01*sqrt(
        static_cast<double>(this->Size[0]*this->Size[0] + this->Size[1]*this->Size[1]));
      if (thresh < 15.0)
      {
        thresh = 15.0;
      }
      double pinchDistance = fabs(newDistance - originalDistance);
      double rotateDistance = newDistance*3.1415926*fabs(angleDeviation)/360.0;
      double panDistance = sqrt(trans[0]*trans[0] + trans[1]*trans[1]);
      if (pinchDistance > thresh
          && pinchDistance > rotateDistance
          && pinchDistance > panDistance)
      {
        this->CurrentGesture = vtkCommand::PinchEvent;
        this->Scale = 1.0;
        this->StartPinchEvent();
      }
      else if (rotateDistance > thresh
          && rotateDistance > panDistance)
      {
        this->CurrentGesture = vtkCommand::RotateEvent;
        this->Rotation = 0.0;
        this->StartRotateEvent();
      }
      else if (panDistance > thresh)
      {
        this->CurrentGesture = vtkCommand::PanEvent;
        this->Translation[0] = 0.0;
        this->Translation[1] = 0.0;
        this->StartPanEvent();
      }
    }

    // if we have found a specific type of movement then
    // handle it
    if (this->CurrentGesture == vtkCommand::RotateEvent)
    {
      this->SetRotation(angleDeviation);
      this->RotateEvent();
    }

    if (this->CurrentGesture == vtkCommand::PinchEvent)
    {
        vtkErrorMacro("See pinch");
      this->SetScale(newDistance/originalDistance);
      this->PinchEvent();
    }

    if (this->CurrentGesture == vtkCommand::PanEvent)
    {
      this->SetTranslation(trans);
      this->PanEvent();
    }

  }

}


// Timer methods. There are two basic groups of methods, those for backward
// compatibility (group #1) and those that operate on specific timers (i.e.,
// use timer ids). The first group of methods implicitly assume that there is
// only one timer at a time running. This was okay in the old days of VTK when
// only the interactors used timers. However with the introduction of new 3D
// widgets into VTK multiple timers often run simultaneously.
//
//old-style group #1
int vtkRenderWindowInteractor::CreateTimer(int timerType)
{
  int platformTimerId, timerId;
  if ( timerType == VTKI_TIMER_FIRST )
  {
    unsigned long duration = this->TimerDuration;
    timerId = vtkTimerId; //just use current id, assume we don't have mutliple timers
    platformTimerId = this->InternalCreateTimer(timerId,RepeatingTimer,duration);
    if ( 0 == platformTimerId )
    {
      return 0;
    }
    (*this->TimerMap)[timerId] = vtkTimerStruct(platformTimerId,RepeatingTimer,duration);
    return timerId;
  }

  else //VTKI_TIMER_UPDATE is just updating last created timer
  {
    return 1; //do nothing because repeating timer has been created
  }
}

//old-style group #1
//just destroy last one created
int vtkRenderWindowInteractor::DestroyTimer()
{
  int timerId = vtkTimerId;
  vtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if ( iter != this->TimerMap->end() )
  {
    this->InternalDestroyTimer((*iter).second.Id);
    this->TimerMap->erase(iter);
    return 1;
  }

  return 0;
}

//new-style group #2 returns timer id
int vtkRenderWindowInteractor::CreateRepeatingTimer(unsigned long duration)
{
  int timerId = ++vtkTimerId;
  int platformTimerId = this->InternalCreateTimer(timerId,RepeatingTimer,duration);
  if ( 0 == platformTimerId )
  {
    return 0;
  }
  (*this->TimerMap)[timerId] = vtkTimerStruct(platformTimerId,RepeatingTimer,duration);
  return timerId;
}

//new-style group #2 returns timer id
int vtkRenderWindowInteractor::CreateOneShotTimer(unsigned long duration)
{
  int timerId = ++vtkTimerId;
  int platformTimerId = this->InternalCreateTimer(timerId,OneShotTimer,duration);
  if ( 0 == platformTimerId )
  {
    return 0;
  }
  (*this->TimerMap)[timerId] = vtkTimerStruct(platformTimerId,OneShotTimer,duration);
  return timerId;
}

//new-style group #2 returns type (non-zero unless bad timerId)
int vtkRenderWindowInteractor::IsOneShotTimer(int timerId)
{
  vtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if ( iter != this->TimerMap->end() )
  {
    return ((*iter).second.Type == OneShotTimer);
  }
  return 0;
}

//new-style group #2 returns duration (non-zero unless bad timerId)
unsigned long vtkRenderWindowInteractor::GetTimerDuration(int timerId)
{
  vtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if ( iter != this->TimerMap->end() )
  {
    return (*iter).second.Duration;
  }
  return 0;
}

//new-style group #2 returns non-zero if timer reset
int vtkRenderWindowInteractor::ResetTimer(int timerId)
{
  vtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if ( iter != this->TimerMap->end() )
  {
    this->InternalDestroyTimer((*iter).second.Id);
    int platformTimerId = this->InternalCreateTimer(timerId, (*iter).second.Type,
                                                    (*iter).second.Duration);
    if ( platformTimerId != 0 )
    {
      (*iter).second.Id = platformTimerId;
      return 1;
    }
    else
    {
      this->TimerMap->erase(iter);
    }
  }
  return 0;
}

//new-style group #2 returns non-zero if timer destroyed
int vtkRenderWindowInteractor::DestroyTimer(int timerId)
{
  vtkTimerIdMapIterator iter = this->TimerMap->find(timerId);
  if ( iter != this->TimerMap->end() )
  {
    this->InternalDestroyTimer((*iter).second.Id);
    this->TimerMap->erase(iter);
    return 1;
  }
  return 0;
}

// Stubbed out dummys
int vtkRenderWindowInteractor::InternalCreateTimer(int vtkNotUsed(timerId), int vtkNotUsed(timerType),
                                                   unsigned long vtkNotUsed(duration))
{
  return 0;
}

int vtkRenderWindowInteractor::InternalDestroyTimer(int vtkNotUsed(platformTimerId))
{
  return 0;
}

// Translate from platformTimerId to the corresponding (VTK) timerId.
// Returns 0 (invalid VTK timerId) if platformTimerId is not found in the map.
// This first stab at an implementation just iterates the map until it finds
// the sought platformTimerId. If performance becomes an issue (lots of timers,
// all firing frequently...) we could speed this up by making a reverse map so
// this method is a quick map lookup.
int vtkRenderWindowInteractor::GetVTKTimerId(int platformTimerId)
{
  int timerId = 0;
  vtkTimerIdMapIterator iter = this->TimerMap->begin();
  for ( ; iter != this->TimerMap->end(); ++iter )
  {
    if ((*iter).second.Id == platformTimerId)
    {
      timerId = (*iter).first;
      break;
    }
  }

  return timerId;
}

// Access to the static variable
int vtkRenderWindowInteractor::GetCurrentTimerId()
{
  return vtkTimerId;
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InteractorStyle:    " << this->InteractorStyle << "\n";
  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  if ( this->Picker )
  {
    os << indent << "Picker: " << this->Picker << "\n";
  }
  else
  {
    os << indent << "Picker: (none)\n";
  }
  if ( this->ObserverMediator )
  {
    os << indent << "Observer Mediator: " << this->ObserverMediator << "\n";
  }
  else
  {
    os << indent << "Observer Mediator: (none)\n";
  }
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
  os << indent << "DesiredUpdateRate: " << this->DesiredUpdateRate << "\n";
  os << indent << "StillUpdateRate: " << this->StillUpdateRate << "\n";
  os << indent << "Initialized: " << this->Initialized << "\n";
  os << indent << "Enabled: " << this->Enabled << "\n";
  os << indent << "EnableRender: " << this->EnableRender << "\n";
  os << indent << "EventPosition: " << "( " << this->EventPosition[0] <<
    ", " << this->EventPosition[1] << " )\n";
  os << indent << "LastEventPosition: " << "( " << this->LastEventPosition[0]
     << ", " << this->LastEventPosition[1] << " )\n";
  os << indent << "EventSize: " << "( " << this->EventSize[0] <<
    ", " << this->EventSize[1] << " )\n";
  os << indent << "Viewport Size: " << "( " << this->Size[0] <<
    ", " << this->Size[1] << " )\n";
  os << indent << "Number of Fly Frames: " << this->NumberOfFlyFrames <<"\n";
  os << indent << "Dolly: " << this->Dolly <<"\n";
  os << indent << "ControlKey: " << this->ControlKey << "\n";
  os << indent << "AltKey: " << this->AltKey << "\n";
  os << indent << "ShiftKey: " << this->ShiftKey << "\n";
  os << indent << "KeyCode: " << this->KeyCode << "\n";
  os << indent << "KeySym: " << (this->KeySym ? this->KeySym : "(null)")
     << "\n";
  os << indent << "RepeatCount: " << this->RepeatCount << "\n";
  os << indent << "Timer Duration: " << this->TimerDuration << "\n";
  os << indent << "TimerEventId: " << this->TimerEventId << "\n";
  os << indent << "TimerEventType: " << this->TimerEventType << "\n";
  os << indent << "TimerEventDuration: " << this->TimerEventDuration << "\n";
  os << indent << "TimerEventPlatformId: " << this->TimerEventPlatformId << "\n";
  os << indent << "UseTDx: " << this->UseTDx << endl;
  os << indent << "Recognize Gestures: " << this->RecognizeGestures << endl;
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::Initialize()
{
  this->Initialized=1;
  this->Enable();
  this->Render();
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::HideCursor()
{
  this->RenderWindow->HideCursor();
}

//----------------------------------------------------------------------------
void vtkRenderWindowInteractor::ShowCursor()
{
  this->RenderWindow->ShowCursor();
}

//----------------------------------------------------------------------------
vtkObserverMediator *vtkRenderWindowInteractor::GetObserverMediator()
{
  if ( !this->ObserverMediator )
  {
    this->ObserverMediator = vtkObserverMediator::New();
    this->ObserverMediator->SetInteractor(this);
  }

  return this->ObserverMediator;
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::MouseMoveEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // handle gestures or not?
  if (this->RecognizeGestures && this->PointersDownCount > 1)
  {
    // handle the gesture
    this->RecognizeGesture(vtkCommand::MouseMoveEvent);
  }
  else
  {
    this->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
  }
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::RightButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::RightButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::RightButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::LeftButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  // are we translating multitouch into gestures?
  if (this->RecognizeGestures)
  {
    if (!this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 1;
      this->PointersDownCount++;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // did we just transition to multitouch?
      if (this->PointersDownCount == 2)
      {
        this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
      }
      // handle the gesture
      this->RecognizeGesture(vtkCommand::LeftButtonPressEvent);
      return;
    }
  }

  this->InvokeEvent(vtkCommand::LeftButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::LeftButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }

  if (this->RecognizeGestures)
  {
    if (this->PointersDown[this->PointerIndex])
    {
      this->PointersDown[this->PointerIndex] = 0;
      this->PointersDownCount--;
    }
    // do we have multitouch
    if (this->PointersDownCount > 1)
    {
      // handle the gesture
      this->RecognizeGesture(vtkCommand::LeftButtonReleaseEvent);
      return;
    }
  }
  this->InvokeEvent(vtkCommand::LeftButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::MiddleButtonPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::MiddleButtonPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::MiddleButtonReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::MiddleButtonReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::MouseWheelForwardEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::MouseWheelBackwardEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::ExposeEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::ExposeEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::ConfigureEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::ConfigureEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::EnterEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::EnterEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::LeaveEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::LeaveEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::KeyPressEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::KeyPressEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::KeyReleaseEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::KeyReleaseEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::CharEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::CharEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::ExitEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::ExitEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::StartPinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::StartPinchEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::PinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::PinchEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::EndPinchEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::EndPinchEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::StartRotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::StartRotateEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::RotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::RotateEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::EndRotateEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::EndRotateEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::StartPanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::StartPanEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::PanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::PanEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::EndPanEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::EndPanEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::TapEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::TapEvent, NULL);
}
//------------------------------------------------------------------
void vtkRenderWindowInteractor::LongTapEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::LongTapEvent, NULL);
}

//------------------------------------------------------------------
void vtkRenderWindowInteractor::SwipeEvent()
{
  if (!this->Enabled)
  {
    return;
  }
  this->InvokeEvent(vtkCommand::SwipeEvent, NULL);
}
