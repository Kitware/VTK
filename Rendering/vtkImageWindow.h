/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkImageWindow - a 2D display window
// .SECTION Description
// vtkImageWindow contains 2D rendering in vtk. Typically a vtkImageWindow
// has some vtkImagers within it.  The imagers in turn display images, Text
// etc. The concept is very similar to that of a RenderWindow for 3d.

// .SECTION See Also
// vtkImager vtkWindow vtkRenderWindow

#ifndef __vtkImageWindow_h
#define __vtkImageWindow_h

#include "vtkWindow.h"
#include "vtkActor2D.h"
#include "vtkImager.h"
#include "vtkImageMapper.h"
#include "vtkImagerCollection.h"
#include "vtkWindow.h"


class VTK_EXPORT vtkImageWindow : public vtkWindow
{
public:
  // Description:
  // Creates a vtkImageWindow with 
  // background erasing disabled and gray scale hint off
  static vtkImageWindow *New();

  void PrintSelf(ostream& os, vtkIndent indent);
  vtkTypeMacro(vtkImageWindow,vtkWindow);

  // Description:
  // Set the position of the window on the screen
  virtual void SetPosition(int x, int y) = 0;
  virtual void SetPosition(int a[2]) { this->SetPosition(a[0],a[1]); };
 
  // Description:
  // Get the position of the window on the screen
  virtual int* GetPosition() = 0;
  virtual void GetPosition(int* x, int* y);

  // Description:
  // Sets the size of a window in pixels.
  virtual void SetSize(int , int ) = 0;
  virtual void SetSize(int a[2]) { this->SetSize(a[0], a[1]); };
  
  // Description:
  // Returns the size of a window in pixels
  virtual int* GetSize() = 0;
  virtual void GetSize(int *x, int *y);

  // Description:
  // These are here for using a tk window.
  virtual void SetDisplayId(void *) = 0;
  virtual void SetWindowId(void *) = 0;
  virtual void SetParentId(void *) = 0;
  virtual void *GetGenericDisplayId() = 0;
  virtual void *GetGenericWindowId() = 0;
  virtual void *GetGenericParentId() = 0;
  virtual void *GetGenericContext()   = 0;
  virtual void *GetGenericDrawable() {return NULL;};

  // Description:
  // Swap the front and back buffers.  This function
  // is used to implement double buffering.  The user
  // shouldn't need to call this function.  To enable
  // double buffering, invoke DoubleBufferOn
  virtual void SwapBuffers() = 0;

  // Description:
  // Flush the buffer and swap if necessary.
  virtual void Frame() = 0;
  
  // Description:
  // useful for scripting languages
  virtual void SetWindowInfo(char *) 
      { vtkErrorMacro(<<"vtkImageWindow::SetWindowInfo - Not implemented"); };
  virtual void SetParentInfo(char *)
      { vtkErrorMacro(<<"vtkImageWindow::SetParentInfo - Not implemented"); };

  // Description:
  // By default this is a color viewer.  GrayScaleHintOn will improve the
  // appearance of gray scale images on some systems.
  vtkSetMacro(GrayScaleHint, int);
  vtkGetMacro(GrayScaleHint, int);
  vtkBooleanMacro(GrayScaleHint, int);

  // Description:
  // Add an imager to the window's list of imagers
  // to be rendered.
  void AddImager(vtkImager* im);

  // Description:
  // Return the collection of imagers for this window.
  vtkImagerCollection *GetImagers() {return this->Imagers;};

  // Description:
  // Remove an imager from the window
  void RemoveImager(vtkImager* im);
  
  // Description:
  // Draw the contents of the window
  virtual void Render();

  // Description:
  // Erase the window contents 
  virtual void EraseWindow();

  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();

  // Description:
  // Open/Write/Close a PPM file
  virtual  int OpenPPMImageFile();
  virtual void WritePPMImageFile();
  virtual void ClosePPMImageFile();

  // Description:
  // Set/Get the FileName used for saving images. See the SaveImageAsPPM 
  // method.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Make this window current. Overridden in subclasses to do, for
  // example, glXMakeCurrent or wglMakeCurrent
  virtual void MakeCurrent() {};

protected:
  vtkImageWindow();
  ~vtkImageWindow();
  vtkImageWindow(const vtkImageWindow&);
  void operator=(const vtkImageWindow&);

  vtkImagerCollection *Imagers;
  int WindowCreated;
  int GrayScaleHint;
  virtual void MakeDefaultWindow() = 0;
  char *FileName;
  FILE* PPMImageFilePtr;

};


#endif




