/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32MappedInteractor.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    to Horst Schreiber for developing this MFC code
	     and Karl M. Syring, who removed all MFC dependencies
  
Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
// .NAME vtkWin32MappedInteractor - provide an event driven interface 
// to the renderer
// .SECTION Description
// vtkWin32MappedInteractor is a convenience object that provides event 
// event bindings to common graphics functions. For example, camera
// zoom-in/zoom-out, azimuth, and roll. It is one of the window system
// specific subclasses of vtkRenderWindowInteractor.

// .SECTION see also
// vtkRenderWindowInteractor vtkWin32OglrRenderWindow

// .SECTION Event Bindings
// Mouse bindings: Button 1 - rotate, Button 2 - pan, Button 3 - zoom
// The distance from the center of the renderer viewport determines
// how quickly to rotate, pan and zoom.
// Keystrokes:
//    r - reset camera view
//    w - turn all actors wireframe
//    s - turn all actors surface
//    e - exits


#ifndef __vtkWin32MappedInteractor_h
#define __vtkWin32MappedInteractor_h

#include "vtkWin32Header.h"
#include "vtkRenderWindowInteractor.h"

#define HDIB HANDLE

class VTK_EXPORT vtkWin32MappedInteractor : public vtkRenderWindowInteractor
{
public:
  vtkWin32MappedInteractor();
  ~vtkWin32MappedInteractor();
  static vtkWin32MappedInteractor *New() {return new vtkWin32MappedInteractor;};
  char *GetClassName() {return "vtkWin32MappedInteractor";};
  void PrintSelf(ostream& os, vtkIndent indent);
  
  virtual void Initialize();
  virtual void Start();
  
  //BTX
  
  // Description: 
  // Various methods that a MFCView class can forward
  // to this class to be handled. The methods basically
  // parallel their MFCView counterparts.
  void OnMouseMove(HWND,UINT nFlags, POINT& point);
  void OnRButtonDown(HWND,UINT nFlags, POINT& point);
  void OnRButtonUp(HWND,UINT nFlags, POINT& point);
  void OnLButtonDown(HWND,UINT nFlags, POINT& point);
  void OnLButtonUp(HWND,UINT nFlags, POINT& point);
  void OnSize(HWND,UINT nType, int cx, int cy);
  void OnTimer(HWND,UINT);
  void OnChar(HWND,UINT nChar, UINT nRepCnt, UINT nFlags);

  void UpdateSize(int cx,int cy);

  void Update();
  void DescribePixelFormat(HDC hDC,DWORD,int);
  void Update2(HDC hDC);
  void BitBlt(HDC hDC,int x_position,int y_position);
  HBITMAP GetBitmap();
  HDIB GetDIB();
  void GetBitmapInfo(LPBITMAPINFOHEADER);
  void SetupLogicalPalette(void);
  void DoPalette(HDC hDC);
  HDIB GetDIB(int width, int height, int bitsperpixel);
  BOOL StretchDIB(HDC hDC,int x_position,int y_position, int x_width, int y_width,
							 int width, int height, int bitsperpixel);
  BOOL SaveBMP(LPCTSTR lpszPathName,int width, int height, int bitsperpixel);
  void Initialize(HWND hwnd, RECT *rcBounds,vtkRenderWindow *renw);
#ifdef TIMER
  void StartTiming(int count);
  void StopTiming();
  void OnEnterIdle();
#endif


protected:
  HWND  WindowId;
  UINT  TimerId;
  int   WindowLeft;
  int   WindowTop;
  int   WindowWidth;
  int   WindowHeight;
  HGLRC WindowRC;
  HDC   MemoryDC;
  HWND  WindowHandle;
  HDC   WindowDC;
  HBITMAP WindowBitmap;
  HBITMAP OldBitmap;	
  RECT  WindowRectangle;
  POINT LastPosition;
  unsigned int MiliSeconds;
  vtkRenderWindow *RenderWindow;
  HPALETTE WindowPalette;
  void MakeDirectRenderer(HWND hwnd, RECT *rcBounds,vtkRenderWindow *renw);
  void MakeIndirectRenderer(int,int,int,vtkRenderWindow *);
  void CreateBMPFile(HWND hwnd, LPTSTR pszFile, PBITMAPINFO pbi,HBITMAP hBMP, HDC hDC);
  PBITMAPINFO CreateBitmapInfoStruct(HWND hwnd, HBITMAP hBmp);
  //ETX
};

#endif


