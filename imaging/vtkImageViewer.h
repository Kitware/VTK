/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkImageViewer - Display a 2d image.
// .SECTION Description
// vtkImageViewer is a convinience class for displaying a 2d image.  It
// packages up the functionality found in vtkImageWindow, vtkImager,
// vtkActor2D and vtkImageMapper into a single easy to use class.  Behind the
// scenes these four classes are actually used to to provide the required
// functionality. vtkImageViewer is simply a wrapper around them.

// .SECTION See Also
// vtkImageWindow vtkImager vtkImageMapper vtkActor2D

#ifndef __vtkImageViewer_h
#define __vtkImageViewer_h

#include <fstream.h>
#include <stdlib.h>
#include <iostream.h>

#include "vtkImageWindow.h"
#include "vtkStructuredPoints.h"
#include "vtkStructuredPointsToImage.h"

// For placement of origin in the viewer.
#define VTK_IMAGE_VIEWER_UPPER_LEFT 0
#define VTK_IMAGE_VIEWER_LOWER_LEFT 1

class VTK_EXPORT vtkImageViewer : public vtkObject 
{
public:
  vtkImageViewer();
  ~vtkImageViewer();
  static vtkImageViewer *New() {return new vtkImageViewer;};
  
  const char *GetClassName() {return "vtkImageViewer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  char *GetWindowName() {return this->ImageWindow->GetWindowName();};

  // Description:
  // Render the resulting image.
  void Render(void);
  
  // Description:
  // Set/Get the input to the viewer.
  void SetInput(vtkImageCache *in) {this->ImageMapper->SetInput(in);};
  vtkImageCache *GetInput() { return this->ImageMapper->GetInput();};
  void SetInput(vtkStructuredPoints *spts) {this->ImageMapper->SetInput(spts);};
  
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
  void SetDisplayId(void *a) {this->ImageWindow->SetDisplayId(a);};
  void SetWindowId(void *a) {this->ImageWindow->SetWindowId(a);};
  void SetParentId(void *a) {this->ImageWindow->SetParentId(a);};
  
  // Description:
  // By default this is a color viewer.  GrayScaleHintOn will improve the
  // appearance of gray scale images on some systems.
  int GetGrayScaleHint() {return this->ImageWindow->GetGrayScaleHint();};
  void SetGrayScaleHint(int a) {this->ImageWindow->SetGrayScaleHint(a);};
  void GrayScaleHintOn() {this->ImageWindow->GrayScaleHintOn();};
  void GrayScaleHintOff() {this->ImageWindow->GrayScaleHintOff();};

  // Description:
  // Set/Get the position in screen coordinates of the rendering window.
  int *GetPosition() {return this->ImageWindow->GetPosition();};
  void SetPosition(int a,int b) {this->ImageWindow->SetPosition(a,b);};
  virtual void SetPosition(int a[2]);

  // Description:
  // Set/Get the size of the window in screen coordinates.
  int *GetSize() {return this->ImageWindow->GetSize();};
  void SetSize(int a,int b) {this->ImageWindow->SetSize(a,b);};
  virtual void SetSize(int a[2]);
  
  // Description:
  // Get the internal Window Imager and Mapper
  vtkImageWindow *GetImageWindow() {return this->ImageWindow;};
  vtkImageMapper *GetImageMapper() {return this->ImageMapper;};
  vtkImager      *GetImager() {return this->Imager;};
  vtkActor2D     *GetActor2D() {return this->Actor2D;};
  
protected:
  vtkImageMapper *ImageMapper;
  vtkImageWindow *ImageWindow;
  vtkImager      *Imager;
  vtkActor2D     *Actor2D;
};

#endif


