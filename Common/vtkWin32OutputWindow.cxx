/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32OutputWindow.cxx
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
#include "vtkWin32OutputWindow.h"
#include "vtkObjectFactory.h"

HWND vtkWin32OutputWindow::OutputWindow = 0;

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

// Display text in the window, and translate the \n to \r\n.

void vtkWin32OutputWindow::DisplayText(const char* someText)
{
  if(!someText)
    {
    return;
    }
  if(this->PromptUser)
    {
    this->PromptText(someText);
    return;
    }
  
  // Create a buffer big enough to hold the entire text
  char* buffer = new char[strlen(someText)+1];
  // Start at the begining
  const char* NewLinePos = someText;
  while(NewLinePos)
    {
    int len = 0;
    // Find the next new line in text
    NewLinePos = strchr(someText, '\n');
    // if no new line is found then just add the text
    if(NewLinePos == 0)
      {
      vtkWin32OutputWindow::AddText(someText);
      }
    // if a new line is found copy it to the buffer
    // and add the buffer with a control new line
    else
      {
      len = NewLinePos - someText;
      strncpy(buffer, someText, len);
      buffer[len] = 0;
      someText = NewLinePos+1;
      vtkWin32OutputWindow::AddText(buffer);
      vtkWin32OutputWindow::AddText("\r\n");
      }
    }
  delete [] buffer;
}


// Add some text to the EDIT control.

void vtkWin32OutputWindow::AddText(const char* someText)
{
  if(!Initialize()  || (strlen(someText) == 0))
    {
    return;
    }
  
  // move to the end of the text area
  SendMessage( vtkWin32OutputWindow::OutputWindow, EM_SETSEL, 
	       (WPARAM)-1, (LPARAM)-1 );  
  // Append the text to the control
  SendMessage( vtkWin32OutputWindow::OutputWindow, EM_REPLACESEL, 
	       0, (LPARAM)someText );
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
  lpParam.dwExStyle = 0;
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


void vtkWin32OutputWindow::PromptText(const char* someText)
{
  ostrstream vtkmsg;
  vtkmsg << someText << "\nPress Cancel to suppress any further messages." 
         << ends;
  if (MessageBox(NULL, vtkmsg.str(), "Error",
		 MB_ICONERROR | MB_OKCANCEL) == IDCANCEL) 
    { 
    vtkObject::GlobalWarningDisplayOff(); 
    }
  vtkmsg.rdbuf()->freeze(0);
}
