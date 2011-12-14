/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkContextScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkContextKeyEvent.h"

#include "vtkRenderWindowInteractor.h"

#include <cassert>

vtkContextKeyEvent::vtkContextKeyEvent()
{
}

vtkContextKeyEvent::~vtkContextKeyEvent()
{
}

void vtkContextKeyEvent::SetInteractor(vtkRenderWindowInteractor *interactor)
{
  this->Interactor = interactor;
}

vtkRenderWindowInteractor* vtkContextKeyEvent::GetInteractor() const
{
  return this->Interactor.GetPointer();
}

char vtkContextKeyEvent::GetKeyCode() const
{
  if (this->Interactor)
    {
    return this->Interactor->GetKeyCode();
    }
  else
    {
    // This should never happen, perhaps there is a better return value?
    return 0;
    }
}
