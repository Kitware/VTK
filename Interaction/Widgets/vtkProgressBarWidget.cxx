/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProgressBarWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkProgressBarWidget.h"
#include "vtkProgressBarRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkProgressBarWidget);

//-------------------------------------------------------------------------
vtkProgressBarWidget::vtkProgressBarWidget()
{
  this->Selectable = 0;
}

//-------------------------------------------------------------------------
vtkProgressBarWidget::~vtkProgressBarWidget()
{
}

//----------------------------------------------------------------------
void vtkProgressBarWidget::CreateDefaultRepresentation()
{
  if (!this->WidgetRep)
  {
    this->WidgetRep = vtkProgressBarRepresentation::New();
  }
}

//-------------------------------------------------------------------------
void vtkProgressBarWidget::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
