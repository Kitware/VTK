/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPlaybackWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPlaybackWidget.h"
#include "vtkPlaybackRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkPlaybackWidget);

//-------------------------------------------------------------------------
vtkPlaybackWidget::vtkPlaybackWidget()
{
}

//-------------------------------------------------------------------------
vtkPlaybackWidget::~vtkPlaybackWidget()
{
}

//----------------------------------------------------------------------
void vtkPlaybackWidget::CreateDefaultRepresentation()
{
  if ( ! this->WidgetRep )
    {
    this->WidgetRep = vtkPlaybackRepresentation::New();
    }
}

//-------------------------------------------------------------------------
void vtkPlaybackWidget::SelectRegion(double eventPos[2])
{
  if ( ! this->WidgetRep )
    {
    return;
    }

  double x = eventPos[0];
  if ( x < 0.16667 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->JumpToBeginning();
    }
  else if ( x <= 0.333333 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->BackwardOneFrame();
    }
  else if ( x <= 0.500000 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->Stop();
    }
  else if ( x < 0.666667 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->Play();
    }
  else if ( x <= 0.833333 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->ForwardOneFrame();
    }
  else if ( x <= 1.00000 )
    {
    reinterpret_cast<vtkPlaybackRepresentation*>(this->WidgetRep)->JumpToEnd();
    }
}

//-------------------------------------------------------------------------
void vtkPlaybackWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

}
