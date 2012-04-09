/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXYPlotWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkXYPlotWidget.h"
#include "vtkXYPlotActor.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkCoordinate.h"

vtkStandardNewMacro(vtkXYPlotWidget);
vtkCxxSetObjectMacro(vtkXYPlotWidget, XYPlotActor, vtkXYPlotActor);

//-------------------------------------------------------------------------
vtkXYPlotWidget::vtkXYPlotWidget()
{
  this->XYPlotActor = vtkXYPlotActor::New();
  this->EventCallbackCommand->SetCallback(vtkXYPlotWidget::ProcessEvents);
  this->State = vtkXYPlotWidget::Outside;
  this->Priority = 0.55;
}

//-------------------------------------------------------------------------
vtkXYPlotWidget::~vtkXYPlotWidget()
{
  if (this->XYPlotActor)
    {
    this->XYPlotActor->Delete();
    }
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::SetEnabled(int enabling)
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

    // Add the xy plot
    this->CurrentRenderer->AddViewProp(this->XYPlotActor);
    this->InvokeEvent(vtkCommand::EnableEvent,NULL);
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
    this->CurrentRenderer->RemoveActor(this->XYPlotActor);
    this->InvokeEvent(vtkCommand::DisableEvent,NULL);
    this->SetCurrentRenderer(NULL);
    }

  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::ProcessEvents(vtkObject* vtkNotUsed(object),
                                       unsigned long event,
                                       void* clientdata,
                                       void* vtkNotUsed(calldata))
{
  vtkXYPlotWidget* self = reinterpret_cast<vtkXYPlotWidget *>( clientdata );

  //okay, let's do the right thing
  switch(event)
    {
    case vtkCommand::LeftButtonPressEvent:
      self->OnLeftButtonDown();
      break;
    case vtkCommand::LeftButtonReleaseEvent:
      self->OnLeftButtonUp();
      break;
    case vtkCommand::MouseMoveEvent:
      self->OnMouseMove();
      break;
    }
}

//-------------------------------------------------------------------------
int vtkXYPlotWidget::ComputeStateBasedOnPosition(int X, int Y,
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
  Result = vtkXYPlotWidget::Moving;
  // unless we are on a corner or edges
  if (e2)
    {
    Result = vtkXYPlotWidget::AdjustingE2;
    }
  if (e4)
    {
    Result = vtkXYPlotWidget::AdjustingE4;
    }
  if (e1)
    {
    Result = vtkXYPlotWidget::AdjustingE1;
    if (e2)
      {
      Result = vtkXYPlotWidget::AdjustingP1;
      }
    if (e4)
      {
      Result = vtkXYPlotWidget::AdjustingP4;
      }
    }
  if (e3)
    {
    Result = vtkXYPlotWidget::AdjustingE3;
    if (e2)
      {
      Result = vtkXYPlotWidget::AdjustingP2;
      }
    if (e4)
      {
      Result = vtkXYPlotWidget::AdjustingP3;
      }
    }

  return Result;
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::SetCursor(int cState)
{
  switch (cState)
    {
    case vtkXYPlotWidget::AdjustingP1:
      this->RequestCursorShape(VTK_CURSOR_SIZESW);
      break;
    case vtkXYPlotWidget::AdjustingP3:
      this->RequestCursorShape(VTK_CURSOR_SIZENE);
      break;
    case vtkXYPlotWidget::AdjustingP2:
      this->RequestCursorShape(VTK_CURSOR_SIZESE);
      break;
    case vtkXYPlotWidget::AdjustingP4:
      this->RequestCursorShape(VTK_CURSOR_SIZENW);
      break;
    case vtkXYPlotWidget::AdjustingE1:
    case vtkXYPlotWidget::AdjustingE3:
      this->RequestCursorShape(VTK_CURSOR_SIZEWE);
      break;
    case vtkXYPlotWidget::AdjustingE2:
    case vtkXYPlotWidget::AdjustingE4:
      this->RequestCursorShape(VTK_CURSOR_SIZENS);
      break;
    case vtkXYPlotWidget::Moving:
      this->RequestCursorShape(VTK_CURSOR_SIZEALL);
      break;
    }
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::OnLeftButtonDown()
{
  // We're only here is we are enabled
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // are we over the widget?
  //this->Interactor->FindPokedRenderer(X,Y);
  int *pos1 = this->XYPlotActor->GetPositionCoordinate()
    ->GetComputedDisplayValue(this->CurrentRenderer);
  int *pos2 = this->XYPlotActor->GetPosition2Coordinate()
    ->GetComputedDisplayValue(this->CurrentRenderer);

  // are we not over the xy plot, ignore
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
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::OnMouseMove()
{
  // compute some info we need for all cases
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];


  // compute the display bounds of the xy plot if we are inside or outside
  int *pos1, *pos2;
  if (this->State == vtkXYPlotWidget::Outside ||
      this->State == vtkXYPlotWidget::Inside)
    {
    pos1 = this->XYPlotActor->GetPositionCoordinate()
      ->GetComputedDisplayValue(this->CurrentRenderer);
    pos2 = this->XYPlotActor->GetPosition2Coordinate()
      ->GetComputedDisplayValue(this->CurrentRenderer);

    if (this->State == vtkXYPlotWidget::Outside)
      {
      // if we are not over the xy plot, ignore
      if (X < pos1[0] || X > pos2[0] ||
          Y < pos1[1] || Y > pos2[1])
        {
        return;
        }
      // otherwise change our state to inside
      this->State = vtkXYPlotWidget::Inside;
      }

    // if inside, set the cursor to the correct shape
    if (this->State == vtkXYPlotWidget::Inside)
      {
      // if we have left then change cursor back to default
      if (X < pos1[0] || X > pos2[0] ||
          Y < pos1[1] || Y > pos2[1])
        {
        this->State = vtkXYPlotWidget::Outside;
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
  double *fpos1 = this->XYPlotActor->GetPositionCoordinate()->GetValue();
  double *fpos2 = this->XYPlotActor->GetPosition2Coordinate()->GetValue();
  float par1[2];
  float par2[2];
  par1[0] = fpos1[0];
  par1[1] = fpos1[1];
  par2[0] = fpos1[0] + fpos2[0];
  par2[1] = fpos1[1] + fpos2[1];

  // based on the state, adjust the xy plot parameters
  switch (this->State)
    {
    case vtkXYPlotWidget::AdjustingP1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::AdjustingP2:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::AdjustingP3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::AdjustingP4:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::AdjustingE1:
      par1[0] = par1[0] + XF - this->StartPosition[0];
      break;
    case vtkXYPlotWidget::AdjustingE2:
      par1[1] = par1[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::AdjustingE3:
      par2[0] = par2[0] + XF - this->StartPosition[0];
      break;
    case vtkXYPlotWidget::AdjustingE4:
      par2[1] = par2[1] + YF - this->StartPosition[1];
      break;
    case vtkXYPlotWidget::Moving:
      // first apply the move
      par1[0] = par1[0] + XF - this->StartPosition[0];
      par1[1] = par1[1] + YF - this->StartPosition[1];
      par2[0] = par2[0] + XF - this->StartPosition[0];
      par2[1] = par2[1] + YF - this->StartPosition[1];
      // then check for an orientation change if the xy plot moves so that
      // its center is closer to a different edge that its current edge by
      // 0.2 then swap orientation
      float centerX = (par1[0] + par2[0])/2.0;
      float centerY = (par1[1] + par2[1])/2.0;
      // what edge is it closest to
      if (fabs(centerX - 0.5) > fabs(centerY - 0.5))
        {
        // is it far enough in to consider a change in orientation?
        if (fabs(centerX - 0.5) > 0.2+fabs(centerY - 0.5))
          {
          // do we need to change orientation
          if (!this->XYPlotActor->GetExchangeAxes())
            {
            this->XYPlotActor->SetExchangeAxes(1);
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
          if (this->XYPlotActor->GetExchangeAxes())
            {
            this->XYPlotActor->SetExchangeAxes(0);
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

  // push the change out to the xy plot
  // make sure the xy plot doesn't shrink to nothing
  if (par2[0] > par1[0] && par2[1] > par1[1])
    {
    this->XYPlotActor->GetPositionCoordinate()->SetValue(par1[0],par1[1]);
    this->XYPlotActor->GetPosition2Coordinate()->
      SetValue(par2[0] - par1[0], par2[1] - par1[1]);
    this->StartPosition[0] = XF;
    this->StartPosition[1] = YF;
    }

  // start a drag
  this->EventCallbackCommand->SetAbortFlag(1);
  this->InvokeEvent(vtkCommand::InteractionEvent, NULL);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::OnLeftButtonUp()
{
  if (this->State == vtkXYPlotWidget::Outside)
    {
    return;
    }

  // stop adjusting
  this->State = vtkXYPlotWidget::Outside;
  this->EventCallbackCommand->SetAbortFlag(1);
  this->RequestCursorShape(VTK_CURSOR_DEFAULT);
  this->EndInteraction();
  this->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);
  this->Interactor->Render();
}

//-------------------------------------------------------------------------
void vtkXYPlotWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "XYPlotActor: " << this->XYPlotActor << "\n";
}
