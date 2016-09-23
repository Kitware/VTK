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

#include "vtkCallbackCommand.h"
#include "vtkCoordinate.h"
#include "vtkObjectFactory.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkScalarBarActor.h"
#include "vtkScalarBarRepresentation.h"
#include "vtkWidgetCallbackMapper.h"
#include "vtkWidgetEvent.h"

vtkStandardNewMacro(vtkScalarBarWidget);

//-------------------------------------------------------------------------
vtkScalarBarWidget::vtkScalarBarWidget()
{
  this->Selectable = 0;
  this->Repositionable = 1;

  // Override the subclasses callback to handle the Repositionable flag.
  this->CallbackMapper->SetCallbackMethod(vtkCommand::MouseMoveEvent,
                                          vtkWidgetEvent::Move,
                                          this, vtkScalarBarWidget::MoveAction);
}

//-------------------------------------------------------------------------
vtkScalarBarWidget::~vtkScalarBarWidget()
{
}

//-----------------------------------------------------------------------------
void vtkScalarBarWidget::SetRepresentation(vtkScalarBarRepresentation *rep)
{
  this->SetWidgetRepresentation(rep);
}

//-----------------------------------------------------------------------------
void vtkScalarBarWidget::SetScalarBarActor(vtkScalarBarActor *actor)
{
  vtkScalarBarRepresentation *rep = this->GetScalarBarRepresentation();
  if (!rep)
  {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
  }

  if (rep->GetScalarBarActor() != actor)
  {
    rep->SetScalarBarActor(actor);
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
vtkScalarBarActor *vtkScalarBarWidget::GetScalarBarActor()
{
  vtkScalarBarRepresentation *rep = this->GetScalarBarRepresentation();
  if (!rep)
  {
    this->CreateDefaultRepresentation();
    rep = this->GetScalarBarRepresentation();
  }

  return rep->GetScalarBarActor();
}

//-----------------------------------------------------------------------------
void vtkScalarBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    vtkScalarBarRepresentation *rep = vtkScalarBarRepresentation::New();
    this->SetRepresentation(rep);
    rep->Delete();
  }
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::SetCursor(int cState)
{
  if (   !this->Repositionable && !this->Selectable
      && cState == vtkBorderRepresentation::Inside)
  {
    // Don't have a special cursor for the inside if we cannot reposition.
    this->RequestCursorShape(VTK_CURSOR_DEFAULT);
  }
  else
  {
    this->Superclass::SetCursor(cState);
  }
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::MoveAction(vtkAbstractWidget *w)
{
  // The the superclass handle most stuff.
  vtkScalarBarWidget::Superclass::MoveAction(w);

  vtkScalarBarWidget *self = reinterpret_cast<vtkScalarBarWidget*>(w);
  vtkScalarBarRepresentation *representation = self->GetScalarBarRepresentation();

  // Handle the case where we suppress widget translation.
  if (   !self->Repositionable
      && (   representation->GetInteractionState()
          == vtkBorderRepresentation::Inside ) )
  {
    representation->MovingOff();
  }
}

//-------------------------------------------------------------------------
void vtkScalarBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Repositionable: " << this->Repositionable << endl;
}
