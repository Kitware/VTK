/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWindow.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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
// .NAME vtkImageWindow - 2D display window
// .SECTION Description
// vtkImageWindow contains 2D rendering in vtk. 
// Typically a vtkImageWindow has some vtkImagers within it.
// The imagers in turn display images, Text etc.

// .SECTION See Also
// vtkImager vtkWindow

#ifndef __vtkImageWindow_h
#define __vtkImageWindow_h

#include "vtkWindow.h"
#include "vtkActor2D.h"
#include "vtkImageCache.h"
#include "vtkImager.h"
#include "vtkImageMapper.h"
#include "vtkImagerCollection.h"
#include "vtkWindow.h"

#define TOP_LEFT_ORIGIN 0

class VTK_EXPORT vtkImageWindow : public vtkWindow
{
public:
  vtkImageWindow();
  ~vtkImageWindow();

  void PrintSelf(ostream& os, vtkIndent indent);
  const char *GetClassName() {return "vtkImageWindow";};
  static vtkImageWindow *New();

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

  // useful for scripting languages
  virtual void SetWindowInfo(char *) 
      { vtkErrorMacro(<<"vtkImageWindow::SetWindowInfo - Not implemented"); };

  vtkSetMacro(GrayScaleHint, int);
  vtkGetMacro(GrayScaleHint, int);
  vtkBooleanMacro(GrayScaleHint, int);

  // Description:
  // Add an imager to the window's list of imagers
  // to be rendered.
  void AddImager(vtkImager* im);

  // Description:
  // Remove an imager from the window
  void RemoveImager(vtkImager* im);
  
  // Description:
  // Draw the contents of the window
  void Render();

  // Description:
  // Erase the window contents 
  virtual void EraseWindow() = 0;

  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();

  // Description:
  // Open a PPM file
  virtual  int OpenPPMImageFile();

  // Description:
  // Write a PPM file
  virtual void WritePPMImageFile();

  // Description:
  // Close the PPM file
  virtual void ClosePPMImageFile();


  // Description:
  // Set/Get the FileName used for saving images. See the SaveImageAsPPM 
  // method.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkImagerCollection Imagers;
  int WindowCreated;
  int GrayScaleHint;
  virtual void MakeDefaultWindow() = 0;
  char *FileName;
  FILE* PPMImageFilePtr;

};


#endif




