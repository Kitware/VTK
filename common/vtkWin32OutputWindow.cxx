/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkWin32OutputWindow.h"
#include "vtkObjectFactory.h"

vtkWin32OutputWindow* vtkWin32OutputWindow::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32OutputWindow");
  if(ret)
    {
    return (vtkWin32OutputWindow*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32OutputWindow;
}


LRESULT APIENTRY vtkWin32OutputWindow::WndProc(HWND hWnd, UINT message, 
					       WPARAM wParam, 
					       LPARAM lParam)
{ 
  switch (message) 
    {
    case WM_SIZE:
      {
      int w = LOWORD(lParam);  // width of client area 
      int h = HIWORD(lParam); // height of client area  
      
      MoveWindow(vtkWin32OutputWindow::OutputWindow,
		 0, 0, w, h, true);
      }
      break;
    case WM_DESTROY:
      vtkWin32OutputWindow::OutputWindow = NULL;
      vtkObject::GlobalWarningDisplayOff();
      break;
    case WM_CREATE:
      break;
    }
  return DefWindowProc(hWnd, message, wParam, lParam);
}

 
HWND vtkWin32OutputWindow::OutputWindow = 0;

// Display text in the window, and translate the \n to \r\n.

void vtkWin32OutputWindow::DisplayText(const char* text)
{
  if(!text)
    {
    return;
    }
  
  // Create a buffer big enough to hold the entire text
  char* buffer = new char[strlen(text)+1];
  // Start at the begining
  const char* NewLinePos = text;
  while(NewLinePos)
    {
    int len = 0;
    // Find the next new line in text
    NewLinePos = strchr(text, '\n');
    // if no new line is found then just add the text
    if(NewLinePos == 0)
      {
      vtkWin32OutputWindow::AddText(text);
      }
    // if a new line is found copy it to the buffer
    // and add the buffer with a control new line
    else
      {
      len = NewLinePos - text;
      strncpy(buffer, text, len);
      buffer[len] = 0;
      text = NewLinePos+1;
      vtkWin32OutputWindow::AddText(buffer);
      vtkWin32OutputWindow::AddText("\r\n");
      }
    }
  delete [] buffer;
}


// Add some text to the EDIT control.

void vtkWin32OutputWindow::AddText(const char* text)
{
  if(!Initialize()  || (strlen(text) == 0))
    {
    return;
    }
  
  // move to the end of the text area
  SendMessage( vtkWin32OutputWindow::OutputWindow, EM_SETSEL, 
	       (WPARAM)-1, (LPARAM)-1 );  
  // Append the text to the control
  SendMessage( vtkWin32OutputWindow::OutputWindow, EM_REPLACESEL, 
	       0, (LPARAM)text );
}


// initialize the output window with an EDIT control and
// a container window.

int vtkWin32OutputWindow::Initialize()
{
  // check to see if it is already initialized
  if(vtkWin32OutputWindow::OutputWindow)
    {
    return 1;
    }
  // Initialized the output window
  
  WNDCLASS wndClass;   
  // has the class been registered ?
  if (!GetClassInfo(GetModuleHandle(NULL),"vtkOutputWindow",&wndClass))
    {
    wndClass.style = CS_HREDRAW | CS_VREDRAW;
    wndClass.lpfnWndProc = vtkWin32OutputWindow::WndProc;
    wndClass.cbClsExtra = 0;
    wndClass.hInstance = GetModuleHandle(NULL);
    wndClass.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    wndClass.hCursor = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
    wndClass.lpszMenuName = NULL;
    wndClass.lpszClassName = "vtkOutputWindow";
    // vtk doesn't use these extra 4 bytes, but app writers
    // may want them, so we provide them.
    wndClass.cbWndExtra = 4;
    RegisterClass(&wndClass);
    }

  // create parent container window
  HWND win = CreateWindow(
    "vtkOutputWindow", "vtkOutputWindow",
    WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
    0, 0, 512, 512,
    NULL, NULL, GetModuleHandle(NULL), NULL);
  
  // Now create child window with text display box
  CREATESTRUCT lpParam;
  lpParam.hInstance = GetModuleHandle(NULL);
  lpParam.hMenu = NULL;
  lpParam.hwndParent = win;
  lpParam.cx = 512;
  lpParam.cy = 512;
  lpParam.x = 0;
  lpParam.y = 0;
  lpParam.style = ES_MULTILINE | ES_READONLY | WS_CHILD 
    | ES_AUTOVSCROLL | ES_AUTOHSCROLL | WS_VISIBLE | WS_MAXIMIZE
    | WS_VSCROLL | WS_HSCROLL;
  
  lpParam.lpszName = "Output Control";
  lpParam.lpszClass = "EDIT"; // use the RICHEDIT control widget
  lpParam.dwExStyle = NULL;
  // Create the EDIT window as a child of win
  vtkWin32OutputWindow::OutputWindow = CreateWindow(
    lpParam.lpszClass,  // pointer to registered class name
    "", // pointer to window name
    lpParam.style,        // window style
    lpParam.x,                // horizontal position of window
    lpParam.y,                // vertical position of window
    lpParam.cx,           // window width
    lpParam.cy,          // window height
    lpParam.hwndParent,      // handle to parent or owner window
    NULL,          // handle to menu or child-window identifier
    lpParam.hInstance,     // handle to application instance
    &lpParam        // pointer to window-creation data
    );
  const int maxsize = 5242880;
  
  SendMessage(vtkWin32OutputWindow::OutputWindow, 
	      EM_LIMITTEXT, 5242880, 0L);

  
  // show the top level container window
  ShowWindow(win, SW_SHOW);
  return 1;
}
