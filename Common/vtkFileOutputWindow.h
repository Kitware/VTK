/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFileOutputWindow.h
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
// .NAME vtkFileOutputWindow - File Specific output window class
// .SECTION Description
// Writes debug/warning/error output to a log file instead of the console.
// To use this class, instantiate it and then call SetInstance(this).
// 

#ifndef __vtkFileOutputWindow_h
#define __vtkFileOutputWindow_h

#include "vtkOutputWindow.h"


class VTK_COMMON_EXPORT vtkFileOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkFileOutputWindow, vtkOutputWindow);

  static vtkFileOutputWindow* New();

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:  Put the text into the log file
  // New lines are converted to carriage return new lines.
  virtual void DisplayText(const char*);

  // Description: Sets the name for the log file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description: Turns on buffer flushing for the output 
  // to the log file.
  vtkSetMacro(Flush, int);
  vtkGetMacro(Flush, int);
  vtkBooleanMacro(Flush, int);
	
  // Description: Setting append will cause the log file to be 
  // opened in append mode.  Otherwise, if the log file exists,
  // it will be overwritten each time the vtkFileOutputWindow 
  // is created.
  vtkSetMacro(Append, int);
  vtkGetMacro(Append, int);
  vtkBooleanMacro(Append, int);

 protected:
  vtkFileOutputWindow(); 
  virtual ~vtkFileOutputWindow(); 
  void Initialize();
  char* FileName;
  ofstream* OStream;
  int Flush;
  int Append;
private:
  vtkFileOutputWindow(const vtkFileOutputWindow&);  // Not implemented.
  void operator=(const vtkFileOutputWindow&);  // Not implemented.
};



#endif
