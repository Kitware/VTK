/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ImageWindow.h
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
// .NAME vtkWin32ImageWindow - (obsolete)2D display window for Windows
// .SECTION Description
// vtkWin32ImageWindow is a concrete subclass of vtkImageWindow.
// It handles 2D rendering under windows.

// .SECTION See Also
// vtkImageWindow

#ifndef __vtkWin32ImageWindow_h
#define __vtkWin32ImageWindow_h


#include        "vtkImageWindow.h"

#ifndef VTK_REMOVE_LEGACY_CODE
class VTK_RENDERING_EXPORT vtkWin32ImageWindow : public vtkImageWindow 
{
public:
  HINSTANCE ApplicationInstance;
  HPALETTE  Palette;
  HDC       DeviceContext;
  HWND      WindowId;
  HWND      ParentId;

  static vtkWin32ImageWindow *New();
  vtkTypeRevisionMacro(vtkWin32ImageWindow,vtkImageWindow);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Swap the front and back buffers. Normally not called by the user.
  // Double buffering is not supported in this class.
  void SwapBuffers();
  
  // Description:
  // Flush and swap buffers if necessary. Double buffering is not
  // supported in this class.
  void Frame();

  // output to the viewer.
  vtkWin32ImageWindow *GetOutput(){return this;};
  
  // Description:
  // Set this ImageWindow's window id to a pre-existing window.
  void SetWindowInfo(char *);

  // Description:
  // Sets the HWND id of the window that WILL BE created.
  void SetParentInfo(char *);

  //BTX

  // Description:
  // Set/Get the window id and parent window id.
  HWND GetWindowId(); 
  void SetWindowId(void* id) {this->WindowId = (HWND) id;};
  void SetParentId(void* id) {this->ParentId = (HWND) id;};
  void SetWindowId(HWND);
  void SetParentId(HWND);

  void SetDeviceContext(void* dc) {this->DeviceContext = (HDC) dc;};
  void SetDeviceContext(HDC);
  void SetDisplayId(void *foo) {vtkDebugMacro(<<"SetDisplayID not implemented");};

  void *GetGenericDisplayId() 
        {vtkDebugMacro(<<"Display ID not implemented in Win32."); return (void*) NULL;};
  void *GetGenericWindowId() {return (void*) this->WindowId;};
  void *GetGenericParentId() {return (void*) this->ParentId;};
  void *GetGenericContext() {return (void*) this->DeviceContext;};
  //ETX

  // Description:
  // Set/Get the current size of the window.
  void   SetSize(int,int);
  int   *GetSize();

  // Description:
  // Set/Get the position in screen coordinates of the window.
  int   *GetPosition();
  void   SetPosition(int,int);

  // Description:
  // Set the desired background color for the window.
  void SetBackgroundColor(float r, float g, float b);

  // Description:
  // Erase the window. Normally nor called by the user.
  void EraseWindow();

  unsigned char *GetDIBPtr();
  unsigned char *GetPixelData(int x1, int y1, int x2, int y2, int);
  
  // Description:
  // Creates a Win32 window or sets up an existing window.
  void MakeDefaultWindow();  

  // Description:
  // These methods can be used by MFC applications 
  // to support print preview and printing, or more
  // general rendering into memory. 
  void SetupMemoryRendering(int x, int y, HDC prn);
  void ResumeScreenRendering();
  HDC GetMemoryDC();
  unsigned char *GetMemoryData(){return this->MemoryData;};

protected:
  vtkWin32ImageWindow();
  ~vtkWin32ImageWindow();

  // the following is used to support rendering into memory
  BITMAPINFO MemoryDataHeader;
  HBITMAP MemoryBuffer;
  unsigned char *MemoryData;    // the data in the DIBSection
  HDC MemoryHdc;
  int ScreenMapped;
  int ScreenWindowSize[2];
  HDC ScreenDeviceContext;

  int OwnWindow; // do we create this window ?

  unsigned char *DIBPtr;        // the data in the DIBSection
  int SwapFlag;
  HDC CompatHdc;
  HDC OldHdc;
  HBITMAP BackBuffer;
  BITMAPINFO DataHeader;
private:
  vtkWin32ImageWindow(const vtkWin32ImageWindow&);  // Not implemented.
  void operator=(const vtkWin32ImageWindow&);  // Not implemented.
};

#endif

#endif
