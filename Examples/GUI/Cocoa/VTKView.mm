//
//  VTKView.mm
//  Animoltion
//
//  Created by Michael A Jackson on 7/19/05.
//  Copyright 2005 __MyCompanyName__. All rights reserved.
//

#define id Id
#include "vtkProperty.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkInteractorStyleTrackballCamera.h"
#include "vtkCocoaRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkCocoaRenderWindow.h"
#include "vtkCommand.h"
#include "vtkCamera.h"
#undef id

#import "vtkCocoaWindow.h"
#import "VTKView.h"

@implementation VTKView

-(id)initWithFrame:(NSRect)frame {
  if ( self = [super initWithFrame:frame] ) {
    
      _renderer = vtkRenderer::New();
      _cocoaRenderWindow = vtkRenderWindow::New();
      _interactor = vtkRenderWindowInteractor::New();
      
      _cocoaRenderWindow->SetWindowId([self window]);
      _cocoaRenderWindow->SetDisplayId(self);
      
      _cocoaRenderWindow->AddRenderer(_renderer);
      _interactor->SetRenderWindow(_cocoaRenderWindow);
      _interactor->Disable();

      [self setVTKRenderWindow:(vtkCocoaRenderWindow*)_cocoaRenderWindow];
  }
  return self;    
}

- (void)drawRect:(NSRect)theRect
{
  if ( _interactor->GetEnabled() == NO )
    {
      _interactor->Initialize();
      vtkInteractorStyleTrackballCamera *trackball =  vtkInteractorStyleTrackballCamera::New();
      [self getInteractor]->SetInteractorStyle(trackball);
      trackball->Delete(); 
    }
  [super drawRect:theRect];
}


-(void)dealloc {
  _interactor->Delete();
  _renderer->Delete();
  _cocoaRenderWindow->Delete();

  [super dealloc];
}

-(vtkRenderer *)renderer {
  return _renderer;
}

-(void)removeAllActors {
  vtkRenderer *renderer = [self renderer];
  if ( ! renderer ) return;
  vtkActorCollection *coll = renderer->GetActors();
  coll->RemoveAllItems();
}


//------------------ Over Ride these from the Parent Class to get better mouse
// ----------------- interaction.                  ---------------------------
- (void)rightMouseDown:(NSEvent *)theEvent
{
  BOOL keepOn = YES;
  NSPoint mouseLoc;
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  vtkCocoaRenderWindowInteractor *myVTKRenderWindowInteractor = [self getInteractor];
  myVTKRenderWindowInteractor->SetEventInformationFlipY(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);

  do
    {
    theEvent = [[self window] nextEventMatchingMask: NSRightMouseUpMask | NSRightMouseDraggedMask | NSPeriodicMask];
    mouseLoc = [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
    myVTKRenderWindowInteractor->SetEventInformationFlipY((int)mouseLoc.x, (int)mouseLoc.y, 
    controlDown, shiftDown);
    switch ([theEvent type])
      {
      case NSRightMouseDragged:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseMoveEvent, NULL);
        break;
      case NSRightMouseUp:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::RightButtonReleaseEvent, NULL);
        keepOn = NO;
        return;
      case NSPeriodic:
        myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::TimerEvent, NULL);
        break;
      default:
        break;
      }
    }
  while (keepOn);

}

//----------------------------------------------------------------------------
- (void)scrollWheel:(NSEvent *)theEvent
{
 vtkCocoaRenderWindowInteractor *myVTKRenderWindowInteractor = [self getInteractor];
  NSPoint mouseLoc = 
    [self convertPoint:[[self window] convertScreenToBase:[theEvent locationInWindow]] fromView:nil];
  int shiftDown = ([theEvent modifierFlags] & NSShiftKeyMask);
  int controlDown = ([theEvent modifierFlags] & NSControlKeyMask);

  myVTKRenderWindowInteractor->SetEventInformation(
    (int)mouseLoc.x, (int)mouseLoc.y, controlDown, shiftDown);
  if( [theEvent deltaY] < 0)
    {
    myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseWheelForwardEvent, NULL);
    }
  else
    {
    myVTKRenderWindowInteractor->InvokeEvent(vtkCommand::MouseWheelBackwardEvent, NULL);
    }
}


@end
