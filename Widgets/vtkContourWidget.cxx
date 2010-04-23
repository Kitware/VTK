/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContourWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkContourWidget.h"
#include "vtkOrientedGlyphContourRepresentation.h"
#include "vtkCommand.h"
#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkSphereSource.h"
#include "vtkProperty.h"
#include "vtkProperty2D.h"
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkPolyData.h"

vtkStandardNewMacro(vtkContourWidget);

//----------------------------------------------------------------------
vtkContourWidget::vtkContourWidget()
{
  this->ManagesCursor    = 0;
  this->WidgetState      = vtkContourWidget::Start;
  this->CurrentHandle    = 0;
  this->AllowNodePicking = 0;
  this->FollowCursor     = 0;
  this->ContinuousDraw   = 0;
  this->ContinuousActive = 0;

  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonPressEvent,
                                          vtkWidgetEvent::Select,
                                          this, vtkContourWidget::SelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::AddFinalPoint,
                                          this, vtkContourWidget::AddFinalPointAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkContourWidget::MoveAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::LeftButtonReleaseEvent,
                                          vtkWidgetEvent::EndSelect,
                                          this, vtkContourWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::NoModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Delete,
                                          this, vtkContourWidget::DeleteAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::KeyPressEvent,
                                          vtkEvent::ShiftModifier, 127, 1, "Delete",
                                          vtkWidgetEvent::Reset,
                                          this, vtkContourWidget::ResetAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonPressEvent,
                                          vtkWidgetEvent::Translate,
                                          this, vtkContourWidget::TranslateContourAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MiddleButtonReleaseEvent,
                                          vtkWidgetEvent::EndTranslate,
                                          this, vtkContourWidget::EndSelectAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonPressEvent,
                                          vtkWidgetEvent::Scale,
                                          this, vtkContourWidget::ScaleContourAction);
  this->CallbackMapper->SetCallbackMethod(vtkCommand::RightButtonReleaseEvent,
                                          vtkWidgetEvent::EndScale,
                                          this, vtkContourWidget::EndSelectAction);

  this->CreateDefaultRepresentation();
}

//----------------------------------------------------------------------
vtkContourWidget::~vtkContourWidget()
{
}

//----------------------------------------------------------------------
void vtkContourWidget::CreateDefaultRepresentation()
{
  if ( !this->WidgetRep )
    {
    vtkOrientedGlyphContourRepresentation *rep =
      vtkOrientedGlyphContourRepresentation::New();

    this->WidgetRep = rep;

    vtkSphereSource *ss = vtkSphereSource::New();
    ss->SetRadius( 0.5 );
    rep->SetActiveCursorShape( ss->GetOutput() );
    ss->Delete();

    rep->GetProperty()->SetColor( 0.25, 1.0, 0.25 );

    vtkProperty *property =
        vtkProperty::SafeDownCast( rep->GetActiveProperty() );
    if ( property )
      {
      property->SetRepresentationToSurface();
      property->SetAmbient( 0.1 );
      property->SetDiffuse( 0.9 );
      property->SetSpecular( 0.0 );
      }
    }
}

//----------------------------------------------------------------------
void vtkContourWidget::CloseLoop()
{
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep);
  if ( !rep->GetClosedLoop() && rep->GetNumberOfNodes() > 1 )
    {
    this->WidgetState = vtkContourWidget::Manipulate;
    rep->ClosedLoopOn();
    this->Render();
    }
}

//----------------------------------------------------------------------
void vtkContourWidget::SetEnabled( int enabling )
{
  // The handle widgets are not actually enabled until they are placed.
  // The handle widgets take their representation from the vtkContourRepresentation.
  if ( enabling )
    {
    if ( this->WidgetState == vtkContourWidget::Start )
      {
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->VisibilityOff();
      }
    else
      {
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep)->VisibilityOn();
      }
    }

  this->Superclass::SetEnabled( enabling );
}

// The following methods are the callbacks that the contour widget responds to.
//-------------------------------------------------------------------------
void vtkContourWidget::SelectAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep =
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if( self->ContinuousDraw )
   {
   self->ContinuousActive = 0;
   }

  switch ( self->WidgetState )
    {
    case vtkContourWidget::Start:
    case vtkContourWidget::Define:
      {
      // If we are following the cursor, let's add 2 nodes rightaway, on the
      // first click. The second node is the one that follows the cursor
      // around.
      if ( (self->FollowCursor || self->ContinuousDraw) && (rep->GetNumberOfNodes() == 0) )
        {
        self->AddNode();
        }
      self->AddNode();
      if( self->ContinuousDraw )
        {
        self->ContinuousActive = 1;
        }
      break;
      }

    case vtkContourWidget::Manipulate:
      {
      if ( rep->ActivateNode( X, Y ) )
        {
        self->Superclass::StartInteraction();
        self->InvokeEvent( vtkCommand::StartInteractionEvent, NULL );
        self->StartInteraction();
        rep->SetCurrentOperationToTranslate();
        rep->StartWidgetInteraction( pos );
        self->EventCallbackCommand->SetAbortFlag( 1 );
        }
      else if ( rep->AddNodeOnContour( X, Y ) )
        {
        if ( rep->ActivateNode( X, Y ) )
          {
          rep->SetCurrentOperationToTranslate();
          rep->StartWidgetInteraction( pos );
          }
        self->EventCallbackCommand->SetAbortFlag( 1 );
        }
      break;
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::AddFinalPointAction(vtkAbstractWidget *w)
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep =
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  if ( self->WidgetState !=  vtkContourWidget::Manipulate &&
       rep->GetNumberOfNodes() >= 1 )
    {
    // In follow cursor and continuous draw mode, the "extra" node 
    // has already been added for us.
    if ( !self->FollowCursor && !self->ContinuousDraw )
      {
      self->AddNode();
      }

    if( self->ContinuousDraw )
      {
      self->ContinuousActive = 0;
      }

    self->WidgetState = vtkContourWidget::Manipulate;
    self->EventCallbackCommand->SetAbortFlag( 1 );
    self->InvokeEvent( vtkCommand::EndInteractionEvent, NULL );
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//------------------------------------------------------------------------
void vtkContourWidget::AddNode()
{
  int X = this->Interactor->GetEventPosition()[0];
  int Y = this->Interactor->GetEventPosition()[1];

  // If the rep already has at least 2 nodes, check how close we are to
  // the first
  vtkContourRepresentation* rep =
    reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep);

  int numNodes = rep->GetNumberOfNodes();
  if ( numNodes > 1 )
    {
    int pixelTolerance = rep->GetPixelTolerance();
    int pixelTolerance2 = pixelTolerance * pixelTolerance;

    double displayPos[2];
    if ( !rep->GetNthNodeDisplayPosition( 0, displayPos ) )
      {
      vtkErrorMacro("Can't get first node display position!");
      return;
      }

    // if in continous draw mode, we dont want to cose the loop until we are at least
    // numNodes > pixelTolerance away

    int distance2 = static_cast<int>((X - displayPos[0]) * (X - displayPos[0]) +
                                     (Y - displayPos[1]) * (Y - displayPos[1]));

    if ( (distance2 < pixelTolerance2 && numNodes > 2) ||
         ( this->ContinuousDraw && numNodes > pixelTolerance && distance2 < pixelTolerance2 ) )
      {
      // yes - we have made a loop. Stop defining and switch to
      // manipulate mode
      this->WidgetState = vtkContourWidget::Manipulate;
      rep->ClosedLoopOn();
      this->Render();
      this->EventCallbackCommand->SetAbortFlag( 1 );
      this->InvokeEvent( vtkCommand::EndInteractionEvent, NULL );
      return;
      }
    }

  if ( rep->AddNodeAtDisplayPosition( X, Y ) )
    {
    if ( this->WidgetState == vtkContourWidget::Start )
      {
      this->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
      }

    this->WidgetState = vtkContourWidget::Define;
    rep->VisibilityOn();
    this->EventCallbackCommand->SetAbortFlag(1);
    this->InvokeEvent( vtkCommand::InteractionEvent, NULL );
    }
}

//-------------------------------------------------------------------------
// Note that if you select the contour at a location that is not moused over
// a control point, the translate action makes the closest contour node
// jump to the current mouse location. Perhaps we should either
// (a) Disable translations when not moused over a control point
// (b) Fix the jumping behaviour by calculating motion vectors from the start
//     of the interaction.
void vtkContourWidget::TranslateContourAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState != vtkContourWidget::Manipulate )
    {
    return;
    }

  vtkContourRepresentation *rep =
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if ( rep->ActivateNode( X, Y ) )
    {
    self->Superclass::StartInteraction();
    self->InvokeEvent( vtkCommand::StartInteractionEvent, NULL );
    self->StartInteraction();
    rep->SetCurrentOperationToShift(); // Here
    rep->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }
  else
    {
    double p[3];
    int idx;
    if( rep->FindClosestPointOnContour( X, Y, p, &idx ) )
      {
      rep->GetNthNodeDisplayPosition( idx, pos );
      rep->ActivateNode( pos );
      self->Superclass::StartInteraction();
      self->InvokeEvent( vtkCommand::StartInteractionEvent, NULL );
      self->StartInteraction();
      rep->SetCurrentOperationToShift(); // Here
      rep->StartWidgetInteraction( pos );
      self->EventCallbackCommand->SetAbortFlag( 1 );
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}
//-------------------------------------------------------------------------
// Note that if you select the contour at a location that is not moused over
// a control point, the scale action makes the closest contour node
// jump to the current mouse location. Perhaps we should either
// (a) Disable scaling when not moused over a control point
// (b) Fix the jumping behaviour by calculating motion vectors from the start
//     of the interaction.
void vtkContourWidget::ScaleContourAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState != vtkContourWidget::Manipulate )
    return;

  vtkContourRepresentation *rep =
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  double pos[2];
  pos[0] = X;
  pos[1] = Y;

  if ( rep->ActivateNode( X, Y ) )
    {
    self->Superclass::StartInteraction();
    self->InvokeEvent( vtkCommand::StartInteractionEvent, NULL );
    self->StartInteraction();
    rep->SetCurrentOperationToScale(); // Here
    rep->StartWidgetInteraction( pos );
    self->EventCallbackCommand->SetAbortFlag( 1 );
    }
  else
    {
    double p[3];
    int idx;
    if( rep->FindClosestPointOnContour( X, Y, p, &idx ) )
      {
      rep->GetNthNodeDisplayPosition( idx, pos );
      rep->ActivateNode( pos );
      self->Superclass::StartInteraction();
      self->InvokeEvent(vtkCommand::StartInteractionEvent,NULL);
      self->StartInteraction();
      rep->SetCurrentOperationToScale(); // Here
      rep->StartWidgetInteraction(pos);
      self->EventCallbackCommand->SetAbortFlag(1);
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::DeleteAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState == vtkContourWidget::Start )
    {
    return;
    }

  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkContourWidget::Define )
    {
    if ( rep->DeleteLastNode() )
      {
      self->InvokeEvent( vtkCommand::InteractionEvent, NULL );
      }
    }
  else
    {
    int X = self->Interactor->GetEventPosition()[0];
    int Y = self->Interactor->GetEventPosition()[1];
    rep->ActivateNode( X, Y );
    if ( rep->DeleteActiveNode() )
      {
      self->InvokeEvent( vtkCommand::InteractionEvent, NULL );
      }
    rep->ActivateNode( X, Y );
    int numNodes = rep->GetNumberOfNodes();
    if ( numNodes < 3 )
      {
      rep->ClosedLoopOff();
      if( numNodes < 2 )
        {
        self->WidgetState = vtkContourWidget::Define;
        }
      }
    }

  if ( rep->GetNeedToRender() )
    {
    self->Render();
    rep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::MoveAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);

  if ( self->WidgetState == vtkContourWidget::Start )
    {
    return;
    }

  int X = self->Interactor->GetEventPosition()[0];
  int Y = self->Interactor->GetEventPosition()[1];
  vtkContourRepresentation *rep = 
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  if ( self->WidgetState == vtkContourWidget::Define )
    {
    if ( self->FollowCursor || self->ContinuousDraw )
      {
      // Have the last node follow the mouse in this case...
      const int numNodes = rep->GetNumberOfNodes();

      // First check if the last node is near the first node, if so, we intend
      // closing the loop.
      if ( numNodes > 1 )
        {
        double displayPos[2];
        int pixelTolerance  = rep->GetPixelTolerance();
        int pixelTolerance2 = pixelTolerance * pixelTolerance;

        rep->GetNthNodeDisplayPosition( 0, displayPos );

        int distance2 = static_cast<int>((X - displayPos[0]) * (X - displayPos[0]) +
                                         (Y - displayPos[1]) * (Y - displayPos[1]));

        const bool mustCloseLoop =
            ( distance2 < pixelTolerance2 && numNodes > 2 ) || 
            ( self->ContinuousDraw && numNodes > pixelTolerance && distance2 < pixelTolerance2 );

        if ( mustCloseLoop != ( rep->GetClosedLoop() == 1 ) )
          {
          if ( rep->GetClosedLoop() )
            {
            // We need to open the closed loop.
            // We do this by adding a node at (X,Y). If by chance the point
            // placer says that (X,Y) is invalid, we'll add it at the location
            // of the first control point (which we know is valid).

            if ( !rep->AddNodeAtDisplayPosition( X, Y ) )
              {
              double closedLoopPoint[3];
              rep->GetNthNodeWorldPosition( 0, closedLoopPoint );
              rep->AddNodeAtDisplayPosition( closedLoopPoint );
              }
            rep->ClosedLoopOff();
            }
          else
            {
            // We need to close the open loop. Delete the node that's following
            // the mouse cursor and close the loop between the previous node and
            // the first node.
            rep->DeleteLastNode();
            rep->ClosedLoopOn();
            }
          }
        else if ( rep->GetClosedLoop() == 0 )
          {
          if( self->ContinuousDraw && self->ContinuousActive )
            {
            rep->AddNodeAtDisplayPosition( X, Y );
            }
          else
            {
            // If we aren't changing the loop topology, simply update the position
            // of the latest node to follow the mouse cursor position (X,Y).
            rep->SetNthNodeDisplayPosition( numNodes-1, X, Y );
            }
          }
        }
      }
    else
      {
      return;
      }
    }

  if ( rep->GetCurrentOperation() == vtkContourRepresentation::Inactive )
    {
    rep->ComputeInteractionState( X, Y );
    rep->ActivateNode( X, Y );
    }
  else
    {
    double pos[2];
    pos[0] = X;
    pos[1] = Y;
    self->WidgetRep->WidgetInteraction( pos );
    self->InvokeEvent( vtkCommand::InteractionEvent, NULL );
    }

  if ( self->WidgetRep->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::EndSelectAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  vtkContourRepresentation *rep =
    reinterpret_cast<vtkContourRepresentation*>(self->WidgetRep);

  if( self->ContinuousDraw )
    {
    self->ContinuousActive = 0;
    }

  // Do nothing if inactive
  if ( rep->GetCurrentOperation() == vtkContourRepresentation::Inactive )
    {
    return;
    }

  rep->SetCurrentOperationToInactive();
  self->EventCallbackCommand->SetAbortFlag(1);
  self->Superclass::EndInteraction();
  self->InvokeEvent(vtkCommand::EndInteractionEvent,NULL);

  // Node picking
  if ( self->AllowNodePicking && self->Interactor->GetControlKey() &&
       self->WidgetState == vtkContourWidget::Manipulate )
    {
    rep->ToggleActiveNodeSelected();
    }

  if ( self->WidgetRep->GetNeedToRender() )
    {
    self->Render();
    self->WidgetRep->NeedToRenderOff();
    }
}

//-------------------------------------------------------------------------
void vtkContourWidget::ResetAction( vtkAbstractWidget *w )
{
  vtkContourWidget *self = reinterpret_cast<vtkContourWidget*>(w);
  self->Initialize( NULL );
}

//----------------------------------------------------------------------
void vtkContourWidget::Initialize( vtkPolyData * pd, int state )
{
  if ( !this->GetEnabled() )
    {
    vtkErrorMacro(<<"Enable widget before initializing");
    }

  if ( this->WidgetRep )
    {
    vtkContourRepresentation *rep = 
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep);

    if ( pd == NULL )
      {
      while( rep->DeleteLastNode() )
        {
        ;
        }
      rep->ClosedLoopOff();
      this->Render();
      rep->NeedToRenderOff();
      rep->VisibilityOff();
      this->WidgetState = vtkContourWidget::Start;
      }
    else
      {
      rep->Initialize( pd );
      this->WidgetState = ( rep->GetClosedLoop() || state == 1 ) ?
        vtkContourWidget::Manipulate : vtkContourWidget::Define;
      }
    }
}

//----------------------------------------------------------------------
void vtkContourWidget::SetAllowNodePicking( int val )
{
  if ( this->AllowNodePicking == val )
    {
    return;
    }
  this->AllowNodePicking = val;
  if ( this->AllowNodePicking )
    {
    vtkContourRepresentation *rep =
      reinterpret_cast<vtkContourRepresentation*>(this->WidgetRep);
    rep->SetShowSelectedNodes( this->AllowNodePicking );
    }
}

//----------------------------------------------------------------------
void vtkContourWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "WidgetState: " << this->WidgetState << endl;
  os << indent << "CurrentHandle: " << this->CurrentHandle << endl;
  os << indent << "AllowNodePicking: " << this->AllowNodePicking << endl;
  os << indent << "FollowCursor: " << (this->FollowCursor ? "On" : "Off") << endl;
  os << indent << "ContinuousDraw: " << (this->ContinuousDraw ? "On" : "Off") << endl;
}
