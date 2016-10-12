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
/**
 * @class   vtkFileOutputWindow
 * @brief   File Specific output window class
 *
 * Writes debug/warning/error output to a log file instead of the console.
 * To use this class, instantiate it and then call SetInstance(this).
 *
*/

#ifndef vtkFileOutputWindow_h
#define vtkFileOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkOutputWindow.h"


class VTKCOMMONCORE_EXPORT vtkFileOutputWindow : public vtkOutputWindow
{
public:
  vtkTypeMacro(vtkFileOutputWindow, vtkOutputWindow);

  static vtkFileOutputWindow* New();

  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Put the text into the log file.
   * New lines are converted to carriage return new lines.
   */
  void DisplayText(const char*) VTK_OVERRIDE;

  //@{
  /**
   * Sets the name for the log file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Turns on buffer flushing for the output
   * to the log file.
   */
  vtkSetMacro(Flush, int);
  vtkGetMacro(Flush, int);
  vtkBooleanMacro(Flush, int);
  //@}

  //@{
  /**
   * Setting append will cause the log file to be
   * opened in append mode.  Otherwise, if the log file exists,
   * it will be overwritten each time the vtkFileOutputWindow
   * is created.
   */
  vtkSetMacro(Append, int);
  vtkGetMacro(Append, int);
  vtkBooleanMacro(Append, int);
  //@}

protected:
  vtkFileOutputWindow();
  ~vtkFileOutputWindow() VTK_OVERRIDE;
  void Initialize();

  char* FileName;
  ofstream* OStream;
  int Flush;
  int Append;

private:
  vtkFileOutputWindow(const vtkFileOutputWindow&) VTK_DELETE_FUNCTION;
  void operator=(const vtkFileOutputWindow&) VTK_DELETE_FUNCTION;
};


#endif
