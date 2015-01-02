/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ExternalVTKWidget.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME ExternalVTKWidget - Use VTK rendering in an external window/application
// .SECTION Description
// ExternalVTKWidget provides an easy way to render VTK objects in an external
// environment using the VTK rendering framework without drawing a new window.

#ifndef __ExternalVTKWidget_h
#define __ExternalVTKWidget_h

#include "vtkExternalOpenGLRenderer.h"
#include "vtkExternalOpenGLRenderWindow.h"
//#include "vtkGenericRenderWindowInteractor.h"
#include "vtkObject.h"
#include "vtkRenderingExternalModule.h" // For export macro

// Forward Declarations
class vtkRenderer;
class vtkRenderWindow;
//class vtkGenericRenderWindowInteractor;

class VTKRENDERINGEXTERNAL_EXPORT ExternalVTKWidget :
  public vtkObject
{
public:
  static ExternalVTKWidget *New();
  vtkTypeMacro(ExternalVTKWidget, vtkObject);
  void PrintSelf(ostream &os, vtkIndent indent);

  // Get the render window
  vtkExternalOpenGLRenderWindow* GetRenderWindow(void);
  void SetRenderWindow(vtkExternalOpenGLRenderWindow* renWin);

  // Get the renderer
  vtkExternalOpenGLRenderer* GetRenderer();

//  // Get the interactor
//  vtkGenericRenderWindowInteractor* GetInteractor();

protected:
  ExternalVTKWidget();
  ~ExternalVTKWidget();

  vtkExternalOpenGLRenderer* Renderer;
  vtkExternalOpenGLRenderWindow* RenderWindow;
//  vtkGenericRenderWindowInteractor* Interactor;

private:
  ExternalVTKWidget(const ExternalVTKWidget&); // Not implemented
  void operator=(const ExternalVTKWidget&); // Not implemented
};

#endif //__ExternalVTKWidget_h
