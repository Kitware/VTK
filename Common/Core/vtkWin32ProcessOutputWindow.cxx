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
#include "vtkWindows.h"

#include <string>

#ifndef _MAX_FNAME
# define _MAX_FNAME 4096
#endif
#ifndef _MAX_PATH
# define _MAX_PATH 4096
#endif

#ifndef PRIdword
#if _LP64
#define PRIdword "u"
#else
#define PRIdword "lu"
#endif
#endif

vtkStandardNewMacro(vtkWin32ProcessOutputWindow);

extern "C" int vtkEncodedArrayWin32OutputWindowProcessWrite(const char* fname);

//----------------------------------------------------------------------------
vtkWin32ProcessOutputWindow::vtkWin32ProcessOutputWindow()
{
  this->OutputPipe = 0;
  this->Broken = 0;
  this->Count = 0;
}

//----------------------------------------------------------------------------
vtkWin32ProcessOutputWindow::~vtkWin32ProcessOutputWindow()
{
  if(this->OutputPipe)
    {
    CloseHandle(this->OutputPipe);
    }
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::DisplayText(const char* text)
{
  // Display the text if the pipe has not been broken.
  if(!this->Broken && text)
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
  // Write the executable as a temporary file.  It will delete itself.
  char exeName[_MAX_FNAME+1] = "";
  char tempDir[_MAX_PATH+1] = "";

  // We will try putting the executable in the system temp directory.
  // Note that the returned path already has a trailing slash.
  DWORD length = GetTempPath(_MAX_PATH+1, tempDir);
  if(length <= 0 || length > _MAX_PATH)
    {
    return 0;
    }

  // Construct the executable name from the process id, pointer to
  // this output window instance, and a count.  This should be unique.
  sprintf(exeName, "vtkWin32OWP_%"PRIdword"_%p_%u.exe",
          GetCurrentProcessId(), this, this->Count++);

  // Allocate a buffer to hold the executable path.
  size_t tdlen = strlen(tempDir);
  char* exeFullPath = (char*)malloc(tdlen + strlen(exeName) + 2);
  if(!exeFullPath)
    {
    return 0;
    }

  // Construct the full path to the executable.
  sprintf(exeFullPath, "%s%s", tempDir, exeName);

  // Try to write the executable to disk.
  if(!vtkEncodedArrayWin32OutputWindowProcessWrite(exeFullPath))
    {
    free(exeFullPath);
    return 0;
    }

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
    DeleteFile(exeFullPath);
    free(exeFullPath);
    return 0;
    }

  // Create the child process.
  if(!CreateProcess(0, exeFullPath, 0, 0, TRUE,
                    NORMAL_PRIORITY_CLASS, 0, 0, &si, &pi))
    {
    DeleteFile(exeFullPath);
    free(exeFullPath);
    CloseHandle(si.hStdInput);
    return 0;
    }

  // We only need to keep the pipe write end.  Close all other handles.
  CloseHandle(si.hStdInput);
  CloseHandle(pi.hThread);
  CloseHandle(pi.hProcess);
  free(exeFullPath);
  return 1;
}

//----------------------------------------------------------------------------
void vtkWin32ProcessOutputWindow::Write(const char* data, size_t length)
{
  if(data && length)
    {
    // Initialize the output pipe the first time.
    if(this->Broken || (!this->OutputPipe && !this->Initialize()))
      {
      this->Broken = 1;
      return;
      }

    // Write the data to the pipe.  If it breaks, close the pipe.
    DWORD nWritten;
    if(!WriteFile(this->OutputPipe, data, (DWORD) length, &nWritten, 0))
      {
      this->Broken = 1;
      CloseHandle(this->OutputPipe);
      this->OutputPipe = 0;
      }
    }
}
