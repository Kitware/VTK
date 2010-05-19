/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTensorProbeWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTensorProbeWidget.h"

#include "vtkCallbackCommand.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkCamera.h"
#include "vtkRenderWindow.h"
#include "vtkWidgetEventTranslator.h"
#include "vtkWidgetCallbackMapper.h" 
#include "vtkEvent.h"
#include "vtkWidgetEvent.h"
#include "vtkEllipsoidTensorProbeRepresentation.h"
#include "vtkMath.h"
#include "vtkLine.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"

vtkStandardNewMacro(vtkTensorProbeWidget);

//----------------------------------------------------------------------
vtkTensorProbeWidget::vtkTensorProbeWidget()
{
  // These are the event callbacks supported by this widget
  this->CallbackMapper->SetCallbackMethod(
      vtkCommand::LeftButtonPressEvent,
      vtkWidgetEvent::Select,
      this, vtkTensorProbeWidget::SelectAction);

  this->CallbackMapper->SetCallbackMethod(
      vtkCommand::LeftButtonReleaseEvent,
      vtkWidgetEvent::EndSelect,
      this, vtkTensorProbeWidget::EndSelectAction);

  this->CallbackMapper->SetCallbackMethod(
      vtkCommand::MouseMoveEvent,
      vtkWidgetEvent::Move,
      this, vtkTensorProbeWidget::MoveAction);
  
  this->Selected = 0;
}

//----------------------------------------------------------------------
vtkTensorProbeWidget::~vtkTensorProbeWidget()
{
}

//----------------------------------------------------------------------
void vtkTensorProbeWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkEllipsoidTensorProbeRepresentation::New();
    }
}

//-------------------------------------------------------------------------
void vtkTensorProbeWidget::SelectAction(vtkAbstractWidget *w)
{
  vtkTensorProbeWidget *self = 
    reinterpret_cast<vtkTensorProbeWidget*>(w);

  if ( !self->Selected )
    {
    vtkTensorProbeRepresentation *rep = reinterpret_cast<
          vtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);
  
    if (rep->SelectProbe(pos))
      {
      self->LastEventPosition[0] = pos[0];
      self->LastEventPosition[1] = pos[1];
      self->Selected = 1;
      self->EventCallbackCommand->SetAbortFlag(1);
      }
    }
}

//-------------------------------------------------------------------------
void vtkTensorProbeWidget::EndSelectAction(vtkAbstractWidget *w)
{
  vtkTensorProbeWidget *self = 
    reinterpret_cast<vtkTensorProbeWidget*>(w);

  if ( self->Selected )
    {
    self->Selected = 0;
    self->EventCallbackCommand->SetAbortFlag(1);
    self->LastEventPosition[0] = -1;
    self->LastEventPosition[1] = -1;
    }
}

//-------------------------------------------------------------------------
void vtkTensorProbeWidget::MoveAction(vtkAbstractWidget *w)
{
  vtkTensorProbeWidget *self = 
    reinterpret_cast<vtkTensorProbeWidget*>(w);

  if ( self->Selected )
    {
    vtkTensorProbeRepresentation *rep = reinterpret_cast<
          vtkTensorProbeRepresentation*>(self->WidgetRep);

    int pos[2];
    self->Interactor->GetEventPosition(pos);
  
    double motionVector[2] = { pos[0] - self->LastEventPosition[0],
                               pos[1] - self->LastEventPosition[1] };

    self->LastEventPosition[0] = pos[0];
    self->LastEventPosition[1] = pos[1];

    if (rep->Move( motionVector ))
      {
      self->EventCallbackCommand->SetAbortFlag(1);
      self->Render();
      }
    }
}

//----------------------------------------------------------------------
void vtkTensorProbeWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}

