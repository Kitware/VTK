/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.h
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
// vtkImageViewer is a convenience class for displaying a 2d image.  It
// packages up the functionality found in vtkImageWindow, vtkImager,
// vtkActor2D and vtkImageMapper into a single easy to use class.  Behind the
// scenes these four classes are actually used to to provide the required
// functionality. vtkImageViewer is simply a wrapper around them.

// .SECTION See Also
// vtkImageWindow vtkImager vtkImageMapper vtkActor2D

#ifndef __vtkImageViewer_h
#define __vtkImageViewer_h

#include "vtkObject.h"
#include "vtkImageWindow.h"
#include "vtkRenderWindow.h"

class VTK_RENDERING_EXPORT vtkImageViewer : public vtkObject 
{
public:
  static vtkImageViewer *New();
  
  vtkTypeRevisionMacro(vtkImageViewer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  char *GetWindowName() {return this->RenderWindow->GetWindowName();};

  // Description:
  // Render the resulting image.
  virtual void Render(void);
  
  // Description:
  // Set/Get the input to the viewer.
  void SetInput(vtkImageData *in) {this->ImageMapper->SetInput(in);};
  vtkImageData *GetInput() { return this->ImageMapper->GetInput();};
  
  // Description:
  // What is the possible Min/ Max z slices available.
  int GetWholeZMin() {return this->ImageMapper->GetWholeZMin();};
  int GetWholeZMax() {return this->ImageMapper->GetWholeZMax();};
  
  // Description:
  // Set/Get the current Z Slice to display
  int GetZSlice() {return this->ImageMapper->GetZSlice();};
  void SetZSlice(int s) {this->ImageMapper->SetZSlice(s);};
  
  // Description:
  // Sets window/level for mapping pixels to colors.
  float GetColorWindow() {return this->ImageMapper->GetColorWindow();};
  float GetColorLevel() {return this->ImageMapper->GetColorLevel();};
  void SetColorWindow(float s) {this->ImageMapper->SetColorWindow(s);};
  void SetColorLevel(float s) {this->ImageMapper->SetColorLevel(s);};

  // Description:
  // These are here for using a tk window.
  void SetDisplayId(void *a) {this->RenderWindow->SetDisplayId(a);};
  void SetWindowId(void *a) {this->RenderWindow->SetWindowId(a);};
  void SetParentId(void *a) {this->RenderWindow->SetParentId(a);};
  
  // Description:
  // By default this is a color viewer.  GrayScaleHintOn will improve the
  // appearance of gray scale images on some systems.
  int GetGrayScaleHint() 
    {vtkWarningMacro("GetGrayScaleHint deprecated, not required anymore"); return 0;};
  void SetGrayScaleHint(int vtkNotUsed(a)) 
    {vtkWarningMacro("SetGrayScaleHint deprecated, not required anymore");};
  void GrayScaleHintOn()
    {vtkWarningMacro("GrayScaleHintOn deprecated, not required anymore");};
  void GrayScaleHintOff()
    {vtkWarningMacro("GrayScaleHintOff deprecated, not required anymore");};

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
  vtkImageWindow *GetImageWindow() 
    {vtkWarningMacro("GetImageWindow deprecated, use GetRenderWindow instead."); return NULL;};
  vtkImager      *GetImager()
    {vtkWarningMacro("GetImager deprecated, use GetRenderer instead."); return NULL;};

  vtkImageMapper *GetImageMapper() {return this->ImageMapper;};
  vtkActor2D     *GetActor2D() {return this->Actor2D;};
  vtkRenderWindow *GetRenderWindow() {return this->RenderWindow;};
  vtkRenderer     *GetRenderer() {return this->Renderer;};
  
protected:
  vtkImageViewer();
  ~vtkImageViewer();

  vtkRenderWindow *RenderWindow;
  vtkRenderer *Renderer;
  vtkImageMapper *ImageMapper;
  vtkActor2D     *Actor2D;
private:
  vtkImageViewer(const vtkImageViewer&);  // Not implemented.
  void operator=(const vtkImageViewer&);  // Not implemented.
};

#endif


