/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer2.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkImageViewer - Display a 2d image.
// .SECTION Description
// vtkImageViewer2 is a convenience class for displaying a 2d image.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// vtkImageActor and vtkImageMapToWindowLevelColors into a single easy to use
// class.  Behind the scenes these four classes are actually used to to
// provide the required functionality. vtkImageViewer2 is simply a wrapper
// around them.

// .SECTION See Also
// vtkRenderWindow vtkRenderer vtkImageActor vtkImageMapToWindowLevelColors

#ifndef __vtkImageViewer2_h
#define __vtkImageViewer2_h

#include "vtkObject.h"
#include "vtkRenderWindow.h"
#include "vtkImageActor.h"
#include "vtkImageMapToWindowLevelColors.h"

class vtkInteractorStyleImage;

class VTK_RENDERING_EXPORT vtkImageViewer2 : public vtkObject 
{
public:
  static vtkImageViewer2 *New();
  
  vtkTypeRevisionMacro(vtkImageViewer2,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  char *GetWindowName() {return this->RenderWindow->GetWindowName();};

  // Description:
  // Render the resulting image.
  virtual void Render(void);
  
  // Description:
  // Set/Get the input to the viewer.
  void SetInput(vtkImageData *in) {this->WindowLevel->SetInput(in);};
  vtkImageData *GetInput() { return this->WindowLevel->GetInput();};
  
  // Description:
  // What is the possible Min/ Max z slices available.
  int GetWholeZMin() {return this->ImageActor->GetWholeZMin();};
  int GetWholeZMax() {return this->ImageActor->GetWholeZMax();};
  
  // Description:
  // Set/Get the current Z Slice to display
  int GetZSlice() {return this->ImageActor->GetZSlice();};
  void SetZSlice(int s) {this->ImageActor->SetZSlice(s);};
  
  // Description:
  // Sets window/level for mapping pixels to colors.
  float GetColorWindow() {return this->WindowLevel->GetWindow();};
  float GetColorLevel() {return this->WindowLevel->GetLevel();};
  void SetColorWindow(float s) {this->WindowLevel->SetWindow(s);};
  void SetColorLevel(float s) {this->WindowLevel->SetLevel(s);};

  // Description:
  // These are here for using a tk window.
  void SetDisplayId(void *a) {this->RenderWindow->SetDisplayId(a);};
  void SetWindowId(void *a) {this->RenderWindow->SetWindowId(a);};
  void SetParentId(void *a) {this->RenderWindow->SetParentId(a);};
  
  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  int *GetPosition() {return this->RenderWindow->GetPosition();};
  void SetPosition(int a,int b) {this->RenderWindow->SetPosition(a,b);};
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  int *GetSize() {return this->RenderWindow->GetSize();};
  void SetSize(int a,int b) {this->RenderWindow->SetSize(a,b);};
  virtual void SetSize(int a[2]);
  
  // Description:
  // Get the internal Window Imager and Mapper
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  vtkGetObjectMacro(WindowLevel,vtkImageMapToWindowLevelColors);
  
  // Description:
  // Create and attach an interactor for this window
  void SetupInteractor(vtkRenderWindowInteractor *);

protected:
  vtkImageViewer2();
  ~vtkImageViewer2();

  vtkImageMapToWindowLevelColors  *WindowLevel;
  vtkRenderWindow *RenderWindow;
  vtkRenderer     *Renderer;
  vtkImageActor   *ImageActor;
  int FirstRender;
  vtkRenderWindowInteractor *Interactor;
  vtkInteractorStyleImage *InteractorStyle;
private:
  vtkImageViewer2(const vtkImageViewer2&);  // Not implemented.
  void operator=(const vtkImageViewer2&);  // Not implemented.
};

#endif


