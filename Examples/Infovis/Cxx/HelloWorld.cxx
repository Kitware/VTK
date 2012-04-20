/*=========================================================================

  Program:   Visualization Toolkit
  Module:    HelloWorld.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example...
//

#include "vtkGraphLayoutView.h"
#include "vtkRandomGraphSource.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"

int main(int, char*[])
{
  vtkRandomGraphSource* source = vtkRandomGraphSource::New();

  vtkGraphLayoutView* view = vtkGraphLayoutView::New();
  view->SetRepresentationFromInputConnection(
    source->GetOutputPort());

  view->ResetCamera();
  view->Render();
  view->GetInteractor()->Start();

  source->Delete();
  view->Delete();

  return 0;
}
