/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarBarWidget.h"
#include "vtkScalarBarActor.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCoordinate.h"

vtkCxxRevisionMacro(vtkScalarBarWidget, "1.3");
vtkStandardNewMacro(vtkScalarBarWidget);
vtkCxxSetObjectMacro(vtkScalarBarWidget, ScalarBarActor, vtkScalarBarActor);

//-------------------------------------------------------------------------
vtkScalarBarWidget::vtkScalarBarWidget()
{
  this->ScalarBarActor = vtkScalarBarActor::New();
  this->EventCallbackCommand->SetCallback(vtkScalarBarWidget::ProcessEvents);
  this->State = vtkScalarBarWidget::Outside;
  this->LeftButtonDown = 0;
  this->RightButtonDown = 0;
  this->Priority = 0.55;
}

//-------------------------------------------------------------------------
vtkScalarBarWidget::~vtkScalarBarWidget()
{
  this->SetScalarBarActor(0);
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::SetEnabled(int enabling)
{
  if ( ! this->Interactor )
    {
    vtkErrorMacro(<<"The interactor must be set prior to enabling/disabling widget");
    return;
    }
  
  if ( enabling ) 
    {
    vtkDebugMacro(<<"Enabling line widget");
    if ( this->Enabled ) //already enabled, just return
      {
      return;
      }
    
    if ( ! this->CurrentRenderer )
      {
      this->SetCurrentRenderer(this->Interactor->FindPokedRenderer(
        this->Interactor->GetLastEventPosition()[0],
        this->Interactor->GetLastEventPosition()[1]));
      if (this->CurrentRenderer == NULL)
        {
        return;
        }
      }

    this->Enabled = 1;
    
    // listen for the following events
    vtkRenderWindowInteractor *i = this->Interactor;
    i->AddObserver(vtkCommand::MouseMoveEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::LeftButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonPressEvent, 
                   this->EventCallbackCommand, this->Priority);
    i->AddObserver(vtkCommand::RightButtonReleaseEvent, 
                   this->EventCallbackCommand, this->Priority);

    // Add the scalar bar
    this->CurrentRenderer->AddViewProp(this->ScalarBarActor);
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);

    // Get the cursor resource manager
    this->ObserverMediator = this->Interactor->GetObserverMediator();
    }
  else //disabling------------------------------------------
    {
    vtkDebugMacro(<<"Disabling line widget");
    if ( ! this->Enabled ) //already disabled, just return
      {
      return;
      }
    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);

    // turn off the line
    this->CurrentRenderer->RemoveActor(this->ScalarBarActor);
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
    }
  
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::ProcessEvents(vtkObject* vtkNotUsed(object), 
                                       unsigned long event,
                                       void* clientdata, 
                                       void* vtkNotUsed(calldata))
{
  vtkScalarBarWidget* self = reinterpret_cast<vtkScalarBarWidget *>( clientdata );
  
  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::RightButtonPressEvent:
      self->OnRightButtonDown();
      break;
    case vtkCommand::RightButtonReleaseEvent:
      self->OnRightButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    }
}

//-------------------------------------------------------------------------
int vtkScalarBarWidget::ComputeStateBasedOnPosition(int X, int Y, 
                                                    int *pos1, int *pos2)
{
  int Result;
  
  // what are we modifying? The position, or size?
  // if size what piece?
  // if we are within 7 pixels of an edge...
  int e1 = 0;
  int e2 = 0;
  int e3 = 0;
  int e4 = 0;
  if (X - pos1[0] < 7)
    {
    e1 = 1;
    }
  if (pos2[0] - X < 7)
    {
    e3 = 1;
    }
  if (Y - pos1[1] < 7)
    {
    e2 = 1;
    }
  if (pos2[1] - Y < 7)
    {
    e4 = 1;
    }

  // assume we are moving
  Result = vtkScalarBarWidget::Moving;
  // unless we are on a corner or edges
  if (e2)
    {
    Result = vtkScalarBarWidget::AdjustingE2;
    }
  if (e4)
    {
    Result = vtkScalarBarWidget::AdjustingE4;
    }
  if (e1)
    {
    Result = vtkScalarBarWidget::AdjustingE1;
    if (e2)
      {
      Result = vtkScalarBarWidget::AdjustingP1;
      }
    if (e4)
      {
      Result = vtkScalarBarWidget::AdjustingP4;
      }
    }
  if (e3)
    {
    Result = vtkScalarBarWidget::AdjustingE3;
    if (e2)
      {
      Result = vtkScalarBarWidget::AdjustingP2;
      }
    if (e4)
      {
      Result = vtkScalarBarWidget::AdjustingP3;
      }
    }

  return Result;
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::SetCursor(int cState)
{
  switch (cState)
    {
    case vtkScalarBarWidget::AdjustingP1:
      this->RequestCursorShape(VTK_CURSOR_SIZESW);
      break;
    case vtkScalarBarWidget::AdjustingP3:
      this->RequestCursorShape(VTK_CURSOR_SIZENE);
      break;
    case vtkScalarBarWidget::AdjustingP2:
      this->RequestCursorShape(VTK_CURSOR_SIZESE);
      break;
    case vtkScalarBarWidget::AdjustingP4:
      this->RequestCursorShape(VTK_CURSOR_SIZENW);
      break;
    case vtkScalarBarWidget::AdjustingE1:
    case vtkScalarBarWidget::AdjustingE3:
      this->RequestCursorShape(VTK_CURSOR_SIZEWE);
      break;
    case vtkScalarBarWidget::AdjustingE2:
    case vtkScalarBarWidget::AdjustingE4:
      this->RequestCursorShape(VTK_CURSOR_SIZENS);
      break;
    case vtkScalarBarWidget::Moving:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;        
    }
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::OnLeftButtonDown()
{
  // We're only here is we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  //this->Interactor->FindPokedRenderer(X,Y);
  int *pos1 = this->ScalarBarActor->GetPositionCoordinate()
    ->GetComputedDisplayValue(this->CurrentRenderer);
  int *pos2 = this->ScalarBarActor->GetPosition2Coordinate()
    ->GetComputedDisplayValue(this->CurrentRenderer);

  // are we not over the scalar bar, ignore
  if (X < pos1[0] || X > pos2[0] || Y < pos1[1] || Y > pos2[1])
    {
    return;
    }
  
  // start a drag, store the normalized view coords
  double X2 = X;
  double Y2 = Y;
  // convert to normalized viewport coordinates
  this->CurrentRenderer->DisplayToNormalizedDisplay(X2,Y2);
  this->CurrentRenderer->NormalizedDisplayToViewport(X2,Y2);
  this->CurrentRenderer->ViewportToNormalizedViewport(X2,Y2);
  this->StartPosition[0] = X2;
  this->StartPosition[1] = Y2;

  this->State = this->ComputeStateBasedOnPosition(X, Y, pos1, pos2);
  this->SetCursor(this->State);
  
  this->EventCallbackCommand->SetAbortFlag(1);
  this->StartInteraction();
  this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
  this->LeftButtonDown = 1;
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // compute the display bounds of the scalar bar if we are inside or outside
  int *pos1, *pos2;
  if (this->State == vtkScalarBarWidget::Outside ||
      this->State == vtkScalarBarWidget::Inside)
    {
    pos1 = this->ScalarBarActor->GetPositionCoordinate()
      ->GetComputedDisplayValue(this->CurrentRenderer);
    pos2 = this->ScalarBarActor->GetPosition2Coordinate()
      ->GetComputedDisplayValue(this->CurrentRenderer);
  
    if (this->State == vtkScalarBarWidget::Outside)
      {
      // if we are not over the scalar bar, ignore
      if (X < pos1[0] || X > pos2[0] ||
          Y < pos1[1] || Y > pos2[1])
        {
        this->RequestCursorShape(VTK_CURSOR_DEFAULT);
        return;
        }
      // otherwise change our state to inside
      this->State = vtkScalarBarWidget::Inside;
      }
  
    // if inside, set the cursor to the correct shape
    if (this->State == vtkScalarBarWidget::Inside)
      {
      // if we have left then change cursor back to default
      if (X < pos1[0] || X > pos2[0] ||
          Y < pos1[1] || Y > pos2[1])
        {
        this->State = vtkScalarBarWidget::Outside;
        this->RequestCursorShape(VTK_CURSOR_DEFAULT);
        return;
        }
      // adjust the cursor based on our position
      this->SetCursor(this->ComputeStateBasedOnPosition(X,Y,pos1,pos2));
      return;
      }
    }
  
  double XF = X;
  double YF = Y;
  // convert to normalized viewport coordinates
  this->CurrentRenderer->DisplayToNormalizedDisplay(XF,YF);
  this->CurrentRenderer->NormalizedDisplayToViewport(XF,YF);
  this->CurrentRenderer->ViewportToNormalizedViewport(XF,YF);
  
  // there are four parameters that can be adjusted
  double *fpos1 = this->ScalarBarActor->GetPositionCoordinate()->GetValue();
  double *fpos2 = this->ScalarBarActor->GetPosition2Coordinate()->GetValue();
  double par1[2];
  double par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];  
  par2[1] = fpos1[1] + fpos2[1];  
    
  // based on the state, adjust the ScalarBar parameters
  switch (this->State)
    {
    case vtkScalarBarWidget::AdjustingP1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::AdjustingP2:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::AdjustingP3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::AdjustingP4:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::AdjustingE1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      break;
    case vtkScalarBarWidget::AdjustingE2:
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::AdjustingE3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      break;
    case vtkScalarBarWidget::AdjustingE4:
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkScalarBarWidget::Moving:
      // first apply the move
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      // then check for an orientation change if the scalar bar moves so that
      // its center is closer to a different edge that its current edge by
      // 0.2 then swap orientation
      double centerX = (par1[0] + par2[0])/2.0;
      double centerY = (par1[1] + par2[1])/2.0;
      // what edge is it closest to
      if (fabs(centerX - 0.5) > fabs(centerY - 0.5))
        {
        // is it far enough in to consider a change in orientation?
        if (fabs(centerX - 0.5) > 0.2+fabs(centerY - 0.5))
          {
          // do we need to change orientation
          if (this->ScalarBarActor->GetOrientation() == VTK_ORIENT_HORIZONTAL)
            {
            this->ScalarBarActor->SetOrientation(VTK_ORIENT_VERTICAL);
            // also change the corners
            par2[0] = centerX + centerY - par1[1];
            par2[1] = centerY + centerX - par1[0];
            par1[0] = 2*centerX - par2[0];
            par1[1] = 2*centerY - par2[1];
            }
          }
        }
      else
        {
        // is it far enough in to consider a change in orientation?
        if (fabs(centerY - 0.5) > 0.2+fabs(centerX - 0.5))
          {
          // do we need to change orientation
          if (this->ScalarBarActor->GetOrientation() != VTK_ORIENT_HORIZONTAL)
            {
            this->ScalarBarActor->SetOrientation(VTK_ORIENT_HORIZONTAL);
            // also change the corners
            par2[0] = centerX + centerY - par1[1];
            par2[1] = centerY + centerX - par1[0];
            par1[0] = 2*centerX - par2[0];
            par1[1] = 2*centerY - par2[1];
            }
          }
        }
      break;
    }
  
  // push the change out to the scalar bar
  // make sure the scalar bar doesn't shrink to nothing
  if (par2[0] > par1[0] && par2[1] > par1[1])
    {
    this->ScalarBarActor->GetPositionCoordinate()->SetValue(par1[0],par1[1]);
    this->ScalarBarActor->GetPosition2Coordinate()->
      SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartPosition[0] = XF;
    this->StartPosition[1] = YF;      
    }
  
  // start a drag
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent,NULL);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::OnLeftButtonUp()
{
  if (this->State == vtkScalarBarWidget::Outside || this->LeftButtonDown == 0)
    {
    return;
    }

  // stop adjusting
  this->State = vtkScalarBarWidget::Outside;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->RequestCursorShape(VTK_CURSOR_DEFAULT);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->LeftButtonDown = 0;
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::OnRightButtonDown()
{

  // are we not over the scalar bar, ignore
  if (this->State == vtkScalarBarWidget::Outside)
    {
    return;
    }

  if (this->HasObserver(vtkCommand::RightButtonPressEvent) ) 
    {
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::RightButtonPressEvent,NULL);
    }
  RightButtonDown = 1;
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::OnRightButtonUp()
{
  if ( this->RightButtonDown == 0 ) 
    {
    return;
    }

  if (this->HasObserver(vtkCommand::RightButtonReleaseEvent)) 
    {
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent(vtkCommand::RightButtonReleaseEvent,NULL);
    }
  this->RightButtonDown = 0;
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  
  os << indent << "ScalarBarActor: " << this->ScalarBarActor << "\n";
}
