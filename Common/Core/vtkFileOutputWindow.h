/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFileOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFileOutputWindow - File Specific output window class
// .SECTION Description
// Writes debug/warning/error output to a log file instead of the console.
// To use this class, instantiate it and then call SetInstance(this).
//

#ifndef __vtkFileOutputWindow_h
#define __vtkFileOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"


class VTKCOMMONCORE_EXPORT vtkFileOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkFileOutputWindow, vtkOutputWindow);

  static vtkFileOutputWindow* New();

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put the text into the log file.
  // New lines are converted to carriage return new lines.
  virtual void DisplayText(const char*);

  // Description:
  // Sets the name for the log file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Turns on buffer flushing for the output
  // to the log file.
  vtkSetMacro(Flush, int);
  vtkGetMacro(Flush, int);
  vtkBooleanMacro(Flush, int);

  // Description:
  // Setting append will cause the log file to be
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
