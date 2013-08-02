/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOutputWindow.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkOutputWindow - base class for writing debug output to a console
// .SECTION Description
// This class is used to encapsulate all text output, so that it will work
// with operating systems that have a stdout and stderr, and ones that
// do not.  (i.e windows does not).  Sub-classes can be provided which can
// redirect the output to a window.

#ifndef __vtkOutputWindow_h
#define __vtkOutputWindow_h

#include "vtkCommonCoreModule.h" // For export macro
#include "vtkObject.h"

//BTX
class VTKCOMMONCORE_EXPORT vtkOutputWindowCleanup
{
public:
  vtkOutputWindowCleanup();
  ~vtkOutputWindowCleanup();

private:
  vtkOutputWindowCleanup(const vtkOutputWindowCleanup& other); // no copy constructor
  vtkOutputWindowCleanup& operator=(const vtkOutputWindowCleanup& rhs); // no copy assignment
};
//ETX

class VTKCOMMONCORE_EXPORT vtkOutputWindow : public vtkObject
{
public:
// Methods from vtkObject
  vtkTypeMacro(vtkOutputWindow,vtkObject);
  // Description:
  // Print ObjectFactor to stream.
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // This is a singleton pattern New.  There will only be ONE
  // reference to a vtkOutputWindow object per process.  Clients that
  // call this must call Delete on the object so that the reference
  // counting will work.   The single instance will be unreferenced when
  // the program exits.
  static vtkOutputWindow* New();
  // Description:
  // Return the singleton instance with no reference counting.
  static vtkOutputWindow* GetInstance();
  // Description:
  // Supply a user defined output window. Call ->Delete() on the supplied
  // instance after setting it.
  static void SetInstance(vtkOutputWindow *instance);
  // Description:
  // Display the text. Four virtual methods exist, depending on the type of
  // message to display. This allows redirection or reformatting of the
  // messages. The default implementation uses DisplayText for all.
  virtual void DisplayText(const char*);
  virtual void DisplayErrorText(const char*);
  virtual void DisplayWarningText(const char*);
  virtual void DisplayGenericWarningText(const char*);

  virtual void DisplayDebugText(const char*);
  // Description:
  // If PromptUser is set to true then each time a line of text
  // is displayed, the user is asked if they want to keep getting
  // messages.
  vtkBooleanMacro(PromptUser,int);
  vtkSetMacro(PromptUser, int);
//BTX
  // use this as a way of memory management when the
  // program exits the SmartPointer will be deleted which
  // will delete the Instance singleton
  static vtkOutputWindowCleanup Cleanup;
//ETX
protected:
  vtkOutputWindow();
  virtual ~vtkOutputWindow();
  int PromptUser;
private:
  static vtkOutputWindow* Instance;
private:
  vtkOutputWindow(const vtkOutputWindow&);  // Not implemented.
  void operator=(const vtkOutputWindow&);  // Not implemented.
};

#endif
