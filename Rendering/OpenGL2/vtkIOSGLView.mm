/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkIOSGLView.mm

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLRenderWindow.h"

#import "vtkIOSGLView.h"
#import "vtkIOSRenderWindow.h"
#import "vtkIOSRenderWindowInteractor.h"
#import "vtkCommand.h"


@implementation vtkIOSGLView

//----------------------------------------------------------------------------
// Private
- (void)emptyMethod:(id)sender
{
  (void)sender;
}

//----------------------------------------------------------------------------
- (vtkIOSRenderWindow *)getVTKRenderWindow
{
  return _myVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (void)setVTKRenderWindow:(vtkIOSRenderWindow *)theVTKRenderWindow
{
  _myVTKRenderWindow = theVTKRenderWindow;
}

//----------------------------------------------------------------------------
- (vtkIOSRenderWindowInteractor *)getInteractor
{
  if (_myVTKRenderWindow)
    {
    return (vtkIOSRenderWindowInteractor *)_myVTKRenderWindow->GetInteractor();
    }
  else
    {
    return NULL;
    }
}


@end
