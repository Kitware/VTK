/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageViewer2.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// packages up the functionality found in vtkRenderWindow, vtkImager,
// vtkActor2D and vtkImageMapper into a single easy to use class.  Behind the
// scenes these four classes are actually used to to provide the required
// functionality. vtkImageViewer is simply a wrapper around them.

// .SECTION See Also
// vtkRenderWindow vtkImager vtkImageMapper vtkActor2D

#ifndef __vtkImageViewer2_h
#define __vtkImageViewer2_h

#include "vtkObject.h"
#include "vtkRenderWindow.h"
#include "vtkImageActor.h"
#include "vtkImageMapToWindowLevelColors.h"

class vtkInteractorStyleImage;

// For placement of origin in the viewer.
#define VTK_IMAGE_VIEWER_UPPER_LEFT 0
#define VTK_IMAGE_VIEWER_LOWER_LEFT 1

class VTK_EXPORT vtkImageViewer2 : public vtkObject 
{
public:
  static vtkImageViewer2 *New();
  
  vtkTypeMacro(vtkImageViewer2,vtkObject);
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
  vtkImageViewer2(const vtkImageViewer2&);
  void operator=(const vtkImageViewer2&);

  vtkImageMapToWindowLevelColors  *WindowLevel;
  vtkRenderWindow *RenderWindow;
  vtkRenderer     *Renderer;
  vtkImageActor   *ImageActor;
  int FirstRender;
  vtkRenderWindowInteractor *Interactor;
  vtkInteractorStyleImage *InteractorStyle;
};

#endif


