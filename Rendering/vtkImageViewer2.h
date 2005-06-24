/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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
// vtkImageViewer2 uses the 3D rendering and texture mapping engine
// to draw an image on a plane.  This allows for rapid rendering,
// zooming, and panning. The image is placed in the 3D scene at a
// depth based on the z-coordinate of the particular image slice. Each
// call to SetZSlice() changes the image data (slice) displayed AND
// changes the depth of the displayed slice in the 3D scene. This can
// be controlled by the AutoResetCameraClippingRange ivar.
//
// It is possible to mix images and geometry, using the methods:
//
// viewer->SetInput( myImage );
// viewer->GetRenderer()->AddActor( myActor );
//
// This can be used to annotate an image with a PolyData of "edges" or
// or highlight sections of an image or display a 3D isosurface
// with a slice from the volume, etc. Any portions of your geometry
// that are in front of the displayed slice will be visible; any
// portions of your geometry that are behind the displayed slice will
// be obscured. A more general framework (with respect to viewing
// direction) for achieving this effect is provided by the
// vtkImagePlaneWidget .
//

// .SECTION See Also
// vtkRenderWindow vtkRenderer vtkImageActor vtkImageMapToWindowLevelColors

#ifndef __vtkImageViewer2_h
#define __vtkImageViewer2_h

#include "vtkObject.h"

#include "vtkRenderWindow.h" // For inline methods
#include "vtkImageActor.h" // For inline methods
#include "vtkImageMapToWindowLevelColors.h" // For inline methods
#include "vtkImageData.h" // makes things a bit easier

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
  vtkImageData *GetInput()
    { return vtkImageData::SafeDownCast(this->WindowLevel->GetInput());}
  virtual void SetInputConnection(vtkAlgorithmOutput* input) {
    this->WindowLevel->SetInputConnection(input);};
  
  // Description:
  // Return the minimum and maximum z-slice values.
  int GetWholeZMin() {return this->ImageActor->GetWholeZMin();}
  int GetWholeZMax() {return this->ImageActor->GetWholeZMax();}
  
  // Description:
  vtkGetMacro(AutoResetCameraClippingRange,int);
  vtkSetMacro(AutoResetCameraClippingRange,int);
  vtkBooleanMacro(AutoResetCameraClippingRange,int);
  
  // Description:
  // Set/Get the current z-slice to display.
  int GetZSlice() {return this->ImageActor->GetZSlice();}
  void SetZSlice(int s);
  
  // Description:
  // Set window and level for mapping pixels to colors.
  double GetColorWindow() {return this->WindowLevel->GetWindow();}
  double GetColorLevel() {return this->WindowLevel->GetLevel();}
  void SetColorWindow(double s) {this->WindowLevel->SetWindow(s);}
  void SetColorLevel(double s) {this->WindowLevel->SetLevel(s);}

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
  // Set your own renderwindow
  virtual void SetRenderWindow(vtkRenderWindow *arg);
  virtual void SetRenderer(vtkRenderer *arg);

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

  virtual void InstallPipeline();
  virtual void UnInstallPipeline();

  vtkImageMapToWindowLevelColors  *WindowLevel;
  vtkRenderWindow *RenderWindow;
  vtkRenderer     *Renderer;
  vtkImageActor   *ImageActor;
  int FirstRender;
  int AutoResetCameraClippingRange;
  vtkRenderWindowInteractor *Interactor;
  vtkInteractorStyleImage *InteractorStyle;

private:
  vtkImageViewer2(const vtkImageViewer2&);  // Not implemented.
  void operator=(const vtkImageViewer2&);  // Not implemented.
};

#endif


