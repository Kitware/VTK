/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

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

// For placement of origin in the viewer.
#define VTK_IMAGE_VIEWER_UPPER_LEFT 0
#define VTK_IMAGE_VIEWER_LOWER_LEFT 1

class VTK_RENDERING_EXPORT vtkImageViewer : public vtkObject 
{
public:
  static vtkImageViewer *New();
  
  vtkTypeMacro(vtkImageViewer,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get name of rendering window
  char *GetWindowName() {return this->ImageWindow->GetWindowName();};

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
  vtkImageViewer();
  ~vtkImageViewer();
  vtkImageViewer(const vtkImageViewer&);
  void operator=(const vtkImageViewer&);

  vtkImageMapper *ImageMapper;
  vtkImageWindow *ImageWindow;
  vtkImager      *Imager;
  vtkActor2D     *Actor2D;
};

#endif


