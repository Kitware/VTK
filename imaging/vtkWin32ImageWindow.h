/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ImageWindow.h
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
// .NAME vtkWin32ImageWindow - 2D display window for Windows
// .SECTION Description
// vtkWin32ImageWindow is a concrete subclass of vtkImageWindow.
// It handles 2D rendering under windows.

// .SECTION See Also
// vtkImageWindow

#ifndef __vtkWin32ImageWindow_h
#define __vtkWin32ImageWindow_h


#include 	"vtkImageWindow.h"

class VTK_EXPORT vtkWin32ImageWindow : public vtkImageWindow 
{
public:
  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HDC       DeviceContext;
  HWND      WindowId;
  HWND      ParentId;

  // ###
  void SwapBuffers();

  vtkWin32ImageWindow();
  ~vtkWin32ImageWindow();
  static vtkWin32ImageWindow *New() {return new vtkWin32ImageWindow;};
  const char *GetClassName() {return "vtkWin32ImageWindow";};

  void PrintSelf(ostream& os, vtkIndent indent);

  // output to the viewer.
  vtkWin32ImageWindow *GetOutput(){return this;};
  
  //BTX
  void SetDisplayId(void *foo) {vtkDebugMacro(<<"SetDisplayID not implemented");};
  HWND GetWindowId(); 
  void SetWindowId(void* id) {this->WindowId = (HWND) id;};
  void SetParentId(void* id) {this->ParentId = (HWND) id;};
  void SetDeviceContext(void* dc) {this->DeviceContext = (HDC) dc;};
  void SetWindowId(HWND);
  void SetParentId(HWND);
  void SetDeviceContext(HDC);

  void *GetGenericDisplayId() 
        {vtkDebugMacro(<<"Display ID not implemented in Win32."); return (void*) NULL;};
  void *GetGenericWindowId() {return (void*) this->WindowId;};
  void *GetGenericParentId() {return (void*) this->ParentId;};
  void *GetGenericContext() {return (void*) this->DeviceContext;};
 

  //ETX

  void   SetSize(int,int);
  int   *GetSize();
  int   *GetPosition();
  void   SetPosition(int,int);
  void PenLineTo(int x,int y);
  void PenMoveTo(int x,int y);
  void SetPenColor(float r,float g,float b);

  // Pen for 2D graphics line drawing
  HPEN Pen;

  void SetBackgroundColor(float r, float g, float b);
  void EraseWindow();

  // Pen color for the graphics;
  COLORREF PenColor;

  // ###
  unsigned char *GetDIBPtr();
  unsigned char *GetPixelData(int x1, int y1, int x2, int y2, int);

protected:
  int OwnWindow; // do we create this window ?
  void MakeDefaultWindow();  
  // ###
  unsigned char *DIBPtr;	// the data in the DIBSection
  int SwapFlag;
  HDC CompatHdc;
  HDC OldHdc;
  HBITMAP BackBuffer;
  BITMAPINFO DataHeader;
};

#endif
