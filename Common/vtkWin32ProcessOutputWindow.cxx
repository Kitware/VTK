/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWin32ProcessOutputWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWin32ProcessOutputWindow.h"
#include "vtkObjectFactory.h"

vtkCxxRevisionMacro(vtkWin32ProcessOutputWindow, "1.2");
vtkStandardNewMacro(vtkWin32ProcessOutputWindow);

//----------------------------------------------------------------------------
vtkWin32ProcessOutputWindow::vtkWin32ProcessOutputWindow()
{
  this->OutputPipe = 0;
  this->Executable = 0;
  this->Broken = 0;
}

//----------------------------------------------------------------------------
vtkWin32ProcessOutputWindow::~vtkWin32ProcessOutputWindow()
{
  this->SetExecutable(0);
  if(this->OutputPipe)
    {
    CloseHandle(this->OutputPipe);
    }
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::DisplayText(const char* text)
{
  // Display the text if the pipe has not been broken.
  if(!this->Broken && this->Executable && text)
    {
    const char* begin = text;
    while(const char* end = strchr(begin, '\n'))
      {
      this->Write(begin, end-begin);
      this->Write("\r\n", 2);
      begin = end+1;
      }
    this->Write(begin, strlen(begin));
    }
}

//----------------------------------------------------------------------------
int vtkWin32ProcessOutputWindow::Initialize()
{
  // Create a process and a pipe connected to its stdin.
  STARTUPINFO si;
  PROCESS_INFORMATION pi;
  ZeroMemory(&si, sizeof(si));
  si.cb = sizeof(si);
  si.dwFlags |= STARTF_USESHOWWINDOW;
  si.dwFlags |= STARTF_USESTDHANDLES;
  si.wShowWindow = SW_SHOWDEFAULT;

  // Create a pipe with an inherited read end.
  if(!CreatePipe(&si.hStdInput, &this->OutputPipe, 0, 0) ||
     !DuplicateHandle(GetCurrentProcess(), si.hStdInput,
                      GetCurrentProcess(), &si.hStdInput,
                      0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE))
    {
    return 0;
    }

  // Create the child process.
  if(!CreateProcess(0, this->Executable, 0, 0, TRUE,
                    NORMAL_PRIORITY_CLASS, 0, 0, &si, &pi))
    {
    CloseHandle(si.hStdInput);
    return 0;
    }

  // We only need to keep the pipe write end.  Close all other handles.
  CloseHandle(si.hStdInput);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  return 1;
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::Write(const char* data, int length)
{
  if(data && length)
    {
    // Initialize the output pipe the first time.
    if(this->Broken || !this->OutputPipe && !this->Initialize())
      {
      this->Broken = 1;
      return;
      }

    // Write the data to the pipe.  If it breaks, close the pipe.
    DWORD nWritten;
    if(!WriteFile(this->OutputPipe, data, length, &nWritten, 0))
      {
      this->Broken = 1;
      CloseHandle(this->OutputPipe);
      this->OutputPipe = 0;
      }
    }
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Executable: " 
     << (this->Executable ? this->Executable : "(none)") << "\n";
}

