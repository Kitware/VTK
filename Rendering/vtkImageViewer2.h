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
// .NAME vtkImageViewer2 - Display a 2D image.
// .SECTION Description
// vtkImageViewer2 is a convenience class for displaying a 2D image.  It
// packages up the functionality found in vtkRenderWindow, vtkRenderer,
// vtkImageActor and vtkImageMapToWindowLevelColors into a single easy to use
// class.  This class also creates an image interactor style
// (vtkInteractorStyleImage) that allows zooming and panning of images, and
// supports interactive window/level operations on the image. Note that
// vtkImageViewer2 is simply a wrapper around these classes.
//
// Some notes on using vtkImageViewer2. It is possible to add geometry to a
// scene using the methods:
//
// viewer->SetInput( myImage );
// viewer->GetRenderer()->AddActor( myActor );
//
// This is nice for "drawing" on top of an image.  For instance, you can
// construct a PolyData of "edges" and draw them on top of an image, or
// you can highlight sections of an image, etc.
//
// .SECTION Caveats
// vtkImageViewer2 occasionally causes suprising clipping behavior.  Often
// when the class is used to display slices from a volume, you can only look
// at the first few slices of volume.  After the first few slices, the image
// becomes black (i.e., background color).  This is because vtkImageViewer2
// uses the 3D rendering/texture mapping engine to draw a slice onto a plane.
// It turns out that the location of that plane is based on the
// "z-coordinate" of the slice.  As you adjust the slice that is viewed, the
// plane moves and eventually gets outside the camera clipping range.
//
// This behavior may be used to advantage in some situations.  If you are
// mixing the image with 3D geometry, you may want the z-coordinate of the
// plane to be adjusted as you switch slices.  This way the image can "hide"
// the portions of the geometry that is behind it. There are other ways to do
// this now using the ImagePlaneWidgets but ImageViewer2 is a quick and dirty
// solution, especially if you want to restrict the view to be down the
// z-axis.  vtkImageViewer2 is confusing, however, when just using the viewer
// to look at each slice in the volume. To solve this problem, whenever you
// call SetZSlice() on the viewer, you need to reset the camera clipping
// range as follows:
//
// viewer->SetZSlice( slice );
// viewer->GetRenderer()->ResetCameraClippingRange();

// .SECTION See Also
// vtkRenderWindow vtkRenderer vtkImageActor vtkImageMapToWindowLevelColors

#ifndef __vtkImageViewer2_h
#define __vtkImageViewer2_h

#include "vtkObject.h"

#include "vtkRenderWindow.h" // For inline methods
#include "vtkImageActor.h" // For inline methods
#include "vtkImageMapToWindowLevelColors.h" // For inline methods

class vtkInteractorStyleImage;

class VTK_RENDERING_EXPORT vtkImageViewer2 : public vtkObject 
{
public:
  static vtkImageViewer2 *New();
  
  vtkTypeRevisionMacro(vtkImageViewer2,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the name of rendering window.
  char *GetWindowName() {return this->RenderWindow->GetWindowName();}

  // Description:
  // Render the resulting image.
  virtual void Render(void);
  
  // Description:
  // Set/Get the input image to the viewer.
  void SetInput(vtkImageData *in) {this->WindowLevel->SetInput(in);}
  vtkImageData *GetInput() { return this->WindowLevel->GetInput();}
  
  // Description:
  // Return the minimum and maximum z-slice values.
  int GetWholeZMin() {return this->ImageActor->GetWholeZMin();}
  int GetWholeZMax() {return this->ImageActor->GetWholeZMax();}
  
  // Description:
  // Set/Get the current z-slice to display.
  int GetZSlice() {return this->ImageActor->GetZSlice();}
  void SetZSlice(int s) {this->ImageActor->SetZSlice(s);}
  
  // Description:
  // Set window and level for mapping pixels to colors.
  float GetColorWindow() {return this->WindowLevel->GetWindow();}
  float GetColorLevel() {return this->WindowLevel->GetLevel();}
  void SetColorWindow(float s) {this->WindowLevel->SetWindow(s);}
  void SetColorLevel(float s) {this->WindowLevel->SetLevel(s);}

  // Description:
  // These are here when using a Tk window.
  void SetDisplayId(void *a) {this->RenderWindow->SetDisplayId(a);}
  void SetWindowId(void *a) {this->RenderWindow->SetWindowId(a);}
  void SetParentId(void *a) {this->RenderWindow->SetParentId(a);}
  
  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  int *GetPosition() {return this->RenderWindow->GetPosition();}
  void SetPosition(int a,int b) {this->RenderWindow->SetPosition(a,b);}
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  int *GetSize() {return this->RenderWindow->GetSize();}
  void SetSize(int a,int b) {this->RenderWindow->SetSize(a,b);}
  virtual void SetSize(int a[2]);
  
  // Description:
  // Get the internal render window, renderer, image actor, and
  // image map instances.
  vtkGetObjectMacro(RenderWindow,vtkRenderWindow);
  vtkGetObjectMacro(Renderer, vtkRenderer);
  vtkGetObjectMacro(ImageActor,vtkImageActor);
  vtkGetObjectMacro(WindowLevel,vtkImageMapToWindowLevelColors);
  
  // Description:
  // Create and attach an interactor for the internal render window.
  void SetupInteractor(vtkRenderWindowInteractor *);
  
  // Description:  
  // Create a window in memory instead of on the screen. This may not
  // be supported for every type of window and on some windows you may
  // need to invoke this prior to the first render.
  void SetOffScreenRendering(int);
  int GetOffScreenRendering();
  void OffScreenRenderingOn();
  void OffScreenRenderingOff();

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


