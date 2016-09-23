/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCameraWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCameraWidget.h"
#include "vtkCameraRepresentation.h"
#include "vtkCameraInterpolator.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkCameraWidget);

//-------------------------------------------------------------------------
vtkCameraWidget::vtkCameraWidget()
{
}

//-------------------------------------------------------------------------
vtkCameraWidget::~vtkCameraWidget()
{
}

//----------------------------------------------------------------------
void vtkCameraWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
  {
    this->WidgetRep = vtkCameraRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void vtkCameraWidget::SelectRegion(double eventPos[2])
{
  if ( ! this->WidgetRep )
  {
    return;
  }

  double x = eventPos[0];
  if ( x < 0.3333 )
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->AddCameraToPath();
  }
  else if ( x < 0.666667 )
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->
      AnimatePath(this->Interactor);
  }
  else if ( x < 1.0 )
  {
    reinterpret_cast<vtkCameraRepresentation*>(this->WidgetRep)->InitializePath();
  }

  this->Superclass::SelectRegion(eventPos);
}

//-------------------------------------------------------------------------
void vtkCameraWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
