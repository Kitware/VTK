/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkStringOutputWindow - File Specific output window class
// .SECTION Description
// Writes debug/warning/error output to a log file instead of the console.
// To use this class, instantiate it and then call SetInstance(this).
//

#ifndef vtkStringOutputWindow_h
#define vtkStringOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"
#include <sstream>  // for ivar

class VTKCOMMONCORE_EXPORT vtkStringOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkStringOutputWindow, vtkOutputWindow);

  static vtkStringOutputWindow* New();

  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Put the text into the log file.
  // New lines are converted to carriage return new lines.
  virtual void DisplayText(const char*);

  // Description:
  // Get the current output as a string
  std::string GetOutput() { return this->OStream.str(); };

protected:
  vtkStringOutputWindow();
  virtual ~vtkStringOutputWindow();
  void Initialize();

  std::ostringstream OStream;

private:
  vtkStringOutputWindow(const vtkStringOutputWindow&);  // Not implemented.
  void operator=(const vtkStringOutputWindow&);  // Not implemented.
};


#endif
