/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageWin32Viewer.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


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
// .NAME vtkImageWin32Viewer - Display a 2d image in an Win32Window.
// .SECTION Description
// vtkImageWin32Viewer displays a 2d image in an Win32 window.

#ifndef __vtkImageWin32Viewer_h
#define __vtkImageWin32Viewer_h


#include 	"vtkImageViewer.h"

class VTK_EXPORT vtkImageWin32Viewer : public vtkImageViewer 
{
public:
  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HDC       DeviceContext;
  HWND      WindowId;
  HWND      ParentId;

  vtkImageWin32Viewer();
  ~vtkImageWin32Viewer();
  char *GetClassName() {return "vtkImageWin32Viewer";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkImageWin32Viewer *GetOutput(){return this;};

  void Render(void);
  
  void SetWindow(int win);
  int GetWindow();
  
  // Description:
  // Gets the number of colors in the pseudo color map.
  vtkGetMacro(NumberOfColors,int);
  
  // Description:
  // Gets the windows depth. For the templated function.
  vtkGetMacro(VisualDepth,int);
  
  // Description:
  // Gets the windows visual class. For the templated function.
  vtkGetMacro(VisualClass,int);
  
  // Description:
  // Window/level information used by templated function.
  float GetColorShift();
  float GetColorScale();

  //BTX
  HWND      GetWindowId();
  void      SetWindowId(void *foo) {this->SetWindowId((HWND)foo);};
  void      SetWindowId(HWND);
  void      SetParentId(void *foo) {this->SetParentId((HWND)foo);};
  void      SetParentId(HWND);
  void      SetDeviceContext(HDC);
  //ETX

  void SetSize(int,int);
  int *GetSize();

protected:
  int OwnWindow; // do we create this window ?
  void MakeDefaultWindow();
  
};

#endif


